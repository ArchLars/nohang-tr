#include "tray.h"
#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QIcon>
#include <QMenu>
#include <algorithm>
#include <vector>

Tray::Tray(QObject *parent, std::unique_ptr<SystemProbe> probe,
           const QString &configPath)
    : QObject(parent),
      probe_(probe ? std::move(probe) : std::make_unique<SystemProbe>()) {
  if (!configPath.isEmpty())
    cfg_.load(configPath);
  auto *menu = new QMenu();
  auto *quit = menu->addAction("Quit");
  connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);
  icon_.setContextMenu(menu);
  timer_.setInterval(cfg_.sample_interval_ms);
  connect(&timer_, &QTimer::timeout, this, &Tray::refresh);

  std::vector<SystemProbe::Trigger> triggers;
  if (cfg_.psi.trigger.some) {
    const auto &t = *cfg_.psi.trigger.some;
    triggers.push_back({SystemProbe::PsiType::Some, t.stall_us, t.window_us});
  }
  if (cfg_.psi.trigger.full) {
    const auto &t = *cfg_.psi.trigger.full;
    triggers.push_back({SystemProbe::PsiType::Full, t.stall_us, t.window_us});
  }
  if (!triggers.empty())
    probe_->enableTriggers(triggers);
}

void Tray::show() {
  icon_.setIcon(QIcon(cfg_.palette.green)); // initial
  icon_.setVisible(true);
  timer_.start();
}

QString Tray::buildTooltip(const ProbeSample &s, const AppConfig &cfg) {
  auto formatKib = [](long kib) {
    double mib = kib / 1024.0;
    if (mib >= 1024.0) {
      double gib = mib / 1024.0;
      return QString("%1 GiB").arg(gib, 0, 'f', 1);
    }
    return QString("%1 MiB").arg(mib, 0, 'f', 1);
  };
  auto makeBar = [](double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    int filled = static_cast<int>(ratio * 10);
    QChar full = QChar(0x2588);  // '█'
    QChar empty = QChar(0x2591); // '░'
    return QString("%1%2").arg(QString(filled, full),
                               QString(10 - filled, empty));
  };

  QString tip;
  if (s.mem_available_kib) {
    tip += QString("MemAvailable: %1\n").arg(formatKib(*s.mem_available_kib));
    auto addMem = [&](const char *label, long thr) {
      double ratio = 1.0 - static_cast<double>(*s.mem_available_kib) / thr;
      ratio = std::clamp(ratio, 0.0, 1.0);
      double pct = ratio * 100.0;
      tip += QString(" %1 %2 [%3] %4%\n")
                 .arg(label)
                 .arg(formatKib(thr))
                 .arg(makeBar(ratio))
                 .arg(pct, 0, 'f', 0);
    };
    addMem("warn", cfg.mem.available_warn_kib);
    addMem("crit", cfg.mem.available_crit_kib);
  } else {
    tip += QStringLiteral("MemAvailable: n/a\n");
  }

  tip += QString("PSI some avg10: %1 (warn %2, crit %3)\n")
             .arg(s.some.avg10, 0, 'f', 2)
             .arg(cfg.psi.avg10_warn, 0, 'f', 2)
             .arg(cfg.psi.avg10_crit, 0, 'f', 2);

  tip += QString("PSI full avg10: %1\n").arg(s.full.avg10, 0, 'f', 2);

  if (cfg.psi.trigger.some) {
    const auto &t = *cfg.psi.trigger.some;
    tip +=
        QString(" trigger some: %1us/%2us\n").arg(t.stall_us).arg(t.window_us);
  }
  if (cfg.psi.trigger.full) {
    const auto &t = *cfg.psi.trigger.full;
    tip +=
        QString(" trigger full: %1us/%2us\n").arg(t.stall_us).arg(t.window_us);
  }

  tip += QString("interval: %1 ms\n").arg(cfg.sample_interval_ms);
  tip +=
      QString("Config: %1")
          .arg(cfg.source == AppConfig::Source::Nohang ? "nohang" : "default");
  return tip;
}

Tray::State Tray::decide(const ProbeSample &s, const AppConfig &cfg,
                         State prev) {
  auto rank = [](State st) { return static_cast<int>(st); };
  const int p = rank(prev);

  const long memCritThr = (p >= rank(State::Red))
                              ? cfg.mem.available_crit_exit_kib
                              : cfg.mem.available_crit_kib;
  const double psiCritThr =
      (p >= rank(State::Red)) ? cfg.psi.avg10_crit_exit : cfg.psi.avg10_crit;
  if ((s.mem_available_kib && *s.mem_available_kib <= memCritThr) ||
      s.some.avg10 >= psiCritThr)
    return State::Red;

  const long memWarnThr = (p >= rank(State::Orange))
                              ? cfg.mem.available_warn_exit_kib
                              : cfg.mem.available_warn_kib;
  if (s.mem_available_kib && *s.mem_available_kib <= memWarnThr)
    return State::Orange;

  const long memWarnMarginThr = cfg.mem.available_warn_exit_kib;
  if (s.mem_available_kib && *s.mem_available_kib <= memWarnMarginThr)
    return State::Yellow;

  const double psiWarnThr =
      (p >= rank(State::Yellow)) ? cfg.psi.avg10_warn_exit : cfg.psi.avg10_warn;
  if (s.some.avg10 >= psiWarnThr)
    return State::Yellow;

  return State::Green;
}

void Tray::refresh() {
  auto sOpt = probe_->sample();
  if (!sOpt)
    return;
  const auto &s = *sOpt;
  icon_.setToolTip(buildTooltip(s, cfg_));
  state_ = decide(s, cfg_, state_);
  switch (state_) {
  case State::Green:
    icon_.setIcon(QIcon(cfg_.palette.green));
    break;
  case State::Yellow:
    icon_.setIcon(QIcon(cfg_.palette.yellow));
    break;
  case State::Orange:
    icon_.setIcon(QIcon(cfg_.palette.orange));
    break;
  case State::Red:
    icon_.setIcon(QIcon(cfg_.palette.red));
    break;
  }
}
