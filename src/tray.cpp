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
    return QString("MemAvailable: %1 KiB\nPSI mem avg10: %2\nPSI mem avg60: %3")
        .arg(s.mem_available_kib)
        .arg(s.psi_mem_avg10, 0, 'f', 2)
        .arg(s.psi_mem_avg60, 0, 'f', 2);
}

Tray::State Tray::decide(const ProbeSample& s, const AppConfig& cfg) {
    if (s.mem_available_kib <= cfg.mem.available_crit_kib || s.psi_mem_avg10 >= cfg.psi.avg10_crit)
        return State::Red;
    if (s.mem_available_kib <= cfg.mem.available_warn_kib)
        return State::Orange;
    if (s.psi_mem_avg10 >= cfg.psi.avg10_warn)
        return State::Yellow;
    return State::Green;
}

void Tray::refresh() {
    auto s = probe_->sample();
    icon_.setToolTip(buildTooltip(s));
    switch (decide(s, cfg_)) {
        case State::Green:  icon_.setIcon(QIcon(cfg_.palette.green));  break;
        case State::Yellow: icon_.setIcon(QIcon(cfg_.palette.yellow)); break;
        case State::Orange: icon_.setIcon(QIcon(cfg_.palette.orange)); break;
        case State::Red:    icon_.setIcon(QIcon(cfg_.palette.red));    break;
    }
}
