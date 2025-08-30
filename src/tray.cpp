#include "tray.h"
#include <QMenu>
#include <QIcon>
#include <QFile>
#include <QAction>
#include <QCoreApplication>

Tray::Tray(QObject* parent, std::unique_ptr<SystemProbe> probe, const QString& configPath)
    : QObject(parent), probe_(probe ? std::move(probe) : std::make_unique<SystemProbe>()) {
    if (!configPath.isEmpty())
        cfg_.load(configPath);
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

Tray::State Tray::decide(const ProbeSample& s, const AppConfig& cfg, State prev) {
    auto rank = [](State st) { return static_cast<int>(st); };
    const int p = rank(prev);

    const long memCritThr = (p >= rank(State::Red)) ? cfg.mem.available_crit_exit_kib
                                                    : cfg.mem.available_crit_kib;
    const double psiCritThr = (p >= rank(State::Red)) ? cfg.psi.avg10_crit_exit
                                                      : cfg.psi.avg10_crit;
    if ((s.mem_available_kib && *s.mem_available_kib <= memCritThr) ||
        s.some.avg10 >= psiCritThr)
        return State::Red;

    const long memWarnThr = (p >= rank(State::Orange)) ? cfg.mem.available_warn_exit_kib
                                                       : cfg.mem.available_warn_kib;
    if (s.mem_available_kib && *s.mem_available_kib <= memWarnThr)
        return State::Orange;

    const double psiWarnThr = (p >= rank(State::Yellow)) ? cfg.psi.avg10_warn_exit
                                                         : cfg.psi.avg10_warn;
    if (s.some.avg10 >= psiWarnThr)
        return State::Yellow;

    return State::Green;
}

void Tray::refresh() {
    auto sOpt = probe_->sample();
    if (!sOpt) return;
    const auto& s = *sOpt;
    icon_.setToolTip(buildTooltip(s));
    state_ = decide(s, cfg_, state_);
    switch (state_) {
        case State::Green:  icon_.setIcon(QIcon(cfg_.palette.green));  break;
        case State::Yellow: icon_.setIcon(QIcon(cfg_.palette.yellow)); break;
        case State::Orange: icon_.setIcon(QIcon(cfg_.palette.orange)); break;
        case State::Red:    icon_.setIcon(QIcon(cfg_.palette.red));    break;
    }
}
