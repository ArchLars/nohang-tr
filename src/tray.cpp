#include "tray.h"
#include <QMenu>
#include <QIcon>
#include <QFile>
#include <QAction>
#include <QCoreApplication>

Tray::Tray(QObject* parent, std::unique_ptr<SystemProbe> probe)
    : QObject(parent), probe_(probe ? std::move(probe) : std::make_unique<SystemProbe>()) {
    cfg_.load("config/nohang-tr.example.toml"); // stub path
    auto *menu = new QMenu();
    auto *quit = menu->addAction("Quit");
    connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);
    icon_.setContextMenu(menu);
    timer_.setInterval(cfg_.sample_interval_ms);
    connect(&timer_, &QTimer::timeout, this, &Tray::refresh);
}

void Tray::show() {
    icon_.setIcon(QIcon(cfg_.palette.green)); // initial
    icon_.setVisible(true);
    timer_.start();
}

QString Tray::buildTooltip(const ProbeSample& s) {
    const QString memAvail = s.mem_available_kib ? QString::number(*s.mem_available_kib)
                                                : QStringLiteral("n/a");
    return QString("MemAvailable: %1 KiB\nPSI some avg10: %2\nPSI some avg60: %3")
        .arg(memAvail)
        .arg(s.some.avg10, 0, 'f', 2)
        .arg(s.some.avg60, 0, 'f', 2);
}

Tray::State Tray::decide(const ProbeSample& s, const AppConfig& cfg) {
    if ((s.mem_available_kib && *s.mem_available_kib <= cfg.mem.available_crit_kib) ||
        s.some.avg10 >= cfg.psi.avg10_crit)
        return State::Red;
    if (s.mem_available_kib && *s.mem_available_kib <= cfg.mem.available_warn_kib)
        return State::Orange;
    if (s.some.avg10 >= cfg.psi.avg10_warn)
        return State::Yellow;
    return State::Green;
}

void Tray::refresh() {
    auto sOpt = probe_->sample();
    if (!sOpt) return;
    const auto& s = *sOpt;
    icon_.setToolTip(buildTooltip(s));
    switch (decide(s, cfg_)) {
        case State::Green:  icon_.setIcon(QIcon(cfg_.palette.green));  break;
        case State::Yellow: icon_.setIcon(QIcon(cfg_.palette.yellow)); break;
        case State::Orange: icon_.setIcon(QIcon(cfg_.palette.orange)); break;
        case State::Red:    icon_.setIcon(QIcon(cfg_.palette.red));    break;
    }
}
