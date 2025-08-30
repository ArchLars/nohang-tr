#include "tray.h"
#include <QMenu>
#include <QIcon>
#include <QFile>
#include <QAction>
#include <QCoreApplication>
#include <algorithm>

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

QString Tray::buildTooltip(const ProbeSample& s, const AppConfig& cfg) {
    auto makeBar = [](double ratio) {
        ratio = std::clamp(ratio, 0.0, 1.0);
        int filled = static_cast<int>(ratio * 10);
        return QString("%1%2").arg(QString(filled, '#'), QString(10 - filled, '-'));
    };

    QString tip;
    if (s.mem_available_kib) {
        tip += QString("MemAvailable: %1 KiB\n").arg(*s.mem_available_kib);
        auto addMem = [&](const char* label, long thr) {
            double ratio = static_cast<double>(*s.mem_available_kib) / thr;
            double pct = ratio * 100.0;
            tip += QString(" %1 %2 KiB [%3] %4%\n")
                .arg(label)
                .arg(thr)
                .arg(makeBar(ratio))
                .arg(pct, 0, 'f', 0);
        };
        addMem("warn", cfg.mem.available_warn_kib);
        addMem("crit", cfg.mem.available_crit_kib);
    } else {
        tip += QStringLiteral("MemAvailable: n/a\n");
    }

    tip += QString("PSI some avg10: %1\n").arg(s.some.avg10, 0, 'f', 2);
    auto addPsi = [&](const char* label, double thr) {
        double ratio = s.some.avg10 / thr;
        double pct = ratio * 100.0;
        tip += QString(" %1 %2 [%3] %4%\n")
            .arg(label)
            .arg(thr, 0, 'f', 2)
            .arg(makeBar(ratio))
            .arg(pct, 0, 'f', 0);
    };
    addPsi("warn", cfg.psi.avg10_warn);
    addPsi("crit", cfg.psi.avg10_crit);

    if (cfg.psi.trigger.some) {
        const auto& t = *cfg.psi.trigger.some;
        tip += QString(" trigger some: %1us/%2us\n").arg(t.stall_us).arg(t.window_us);
    }
    if (cfg.psi.trigger.full) {
        const auto& t = *cfg.psi.trigger.full;
        tip += QString(" trigger full: %1us/%2us\n").arg(t.stall_us).arg(t.window_us);
    }

    tip += QString("interval: %1 ms").arg(cfg.sample_interval_ms);
    return tip;
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
    icon_.setToolTip(buildTooltip(s, cfg_));
    state_ = decide(s, cfg_, state_);
    switch (state_) {
        case State::Green:  icon_.setIcon(QIcon(cfg_.palette.green));  break;
        case State::Yellow: icon_.setIcon(QIcon(cfg_.palette.yellow)); break;
        case State::Orange: icon_.setIcon(QIcon(cfg_.palette.orange)); break;
        case State::Red:    icon_.setIcon(QIcon(cfg_.palette.red));    break;
    }
}
