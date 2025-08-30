#include "tray.h"
#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QIcon>
#include <QMenu>
#include <QProcessEnvironment>
#include <algorithm>
#include <cmath>
#include <vector>
#ifdef HAVE_AYATANA_APPINDICATOR3
#undef signals
#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#endif

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

bool Tray::shouldUseAppIndicator() {
#ifdef HAVE_AYATANA_APPINDICATOR3
  const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
  return desktop.contains("GNOME", Qt::CaseInsensitive);
#else
  return false;
#endif
}

void Tray::show() {
  icon_.setIcon(QIcon(cfg_.palette.black)); // initial
#ifdef HAVE_AYATANA_APPINDICATOR3
  if (shouldUseAppIndicator()) {
    if (!indicator_) {
      int argc = 0;
      char **argv = nullptr;
      if (gtk_init_check(&argc, &argv)) { // GCOVR_EXCL_START
        indicator_ = app_indicator_new(
            "nohang-tr", cfg_.palette.black.toUtf8().constData(),
            APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
        app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *quit = gtk_menu_item_new_with_label("Quit");
        g_signal_connect(quit, "activate",
                         G_CALLBACK(+[](GtkMenuItem *, gpointer) {
                           QCoreApplication::quit();
                         }),
                         nullptr);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
        gtk_widget_show_all(menu);
        app_indicator_set_menu(indicator_, GTK_MENU(menu));
        menuIndicator_ = menu;
      } // GCOVR_EXCL_STOP
    }
  } else {
    icon_.setVisible(true);
  }
#else
  icon_.setVisible(true);
#endif
  timer_.start();
}

QString Tray::buildTooltip(const ProbeSample &s, const AppConfig &cfg,
                           State state) {
  auto formatKib = [](long kib) {
    double mib = kib / 1024.0;
    if (mib >= 1024.0) {
      double gib = mib / 1024.0;
      return QString("%1 GiB").arg(gib, 0, 'f', 1);
    }
    return QString("%1 MiB").arg(mib, 0, 'f', 1);
  };
  auto colorFor = [](State st) {
    switch (st) {
    case State::Green:
      return "green";
    case State::Yellow:
      return "yellow";
    case State::Orange:
      return "orange";
    case State::Red:
      return "red";
    }
    return "white";
  };
  auto makeBar = [&](double ratio) {
    ratio = std::clamp(ratio, 0.0, 1.0);
    int filled = static_cast<int>(ratio * 10);
    QChar full = QChar(0x2588);  // '█'
    QChar empty = QChar(0x2591); // '░'
    QString filledStr(filled, full);
    QString emptyStr(10 - filled, empty);
    return QString("<span style='color:%1'>%2</span><span "
                   "style='color:gray'>%3</span>")
        .arg(colorFor(state))
        .arg(filledStr)
        .arg(emptyStr);
  };

  QString tip;
  if (s.mem_available_kib) {
    if (s.mem_total_kib) {
      tip += QString("MemAvailable: %1 / %2\n")
                 .arg(formatKib(*s.mem_available_kib))
                 .arg(formatKib(*s.mem_total_kib));
    } else {
      tip += QString("MemAvailable: %1\n").arg(formatKib(*s.mem_available_kib));
    }
  } else {
    tip += QStringLiteral("MemAvailable: n/a\n");
  }

  if (s.mem_free_kib)
    tip += QString("MemFree: %1\n").arg(formatKib(*s.mem_free_kib));
  else
    tip += QStringLiteral("MemFree: n/a\n");

  if (s.swap_free_kib)
    tip += QString("SwapFree: %1\n").arg(formatKib(*s.swap_free_kib));
  else
    tip += QStringLiteral("SwapFree: n/a\n");

  if (s.cached_kib)
    tip += QString("Cached: %1\n").arg(formatKib(*s.cached_kib));
  else
    tip += QStringLiteral("Cached: n/a\n");

  std::vector<double> ratios;
  if (s.mem_available_kib)
    ratios.push_back(static_cast<double>(*s.mem_available_kib) /
                     cfg.mem.available_warn_kib);
  if (s.mem_free_kib)
    ratios.push_back(static_cast<double>(*s.mem_free_kib) /
                     cfg.mem.available_warn_kib);
  if (s.cached_kib)
    ratios.push_back(static_cast<double>(*s.cached_kib) /
                     cfg.mem.available_warn_kib);
  if (s.swap_free_kib)
    ratios.push_back(static_cast<double>(*s.swap_free_kib) /
                     cfg.swap.free_warn_kib);
  if (!ratios.empty()) {
    double minRatio = *std::min_element(ratios.begin(), ratios.end());
    double usage = 1.0 - std::clamp(minRatio, 0.0, 1.0);
    tip += QString("Pressure: [%1] %2%\n")
               .arg(makeBar(usage))
               .arg(usage * 100.0, 0, 'f', 0);
  } else {
    tip += QStringLiteral("Pressure: n/a\n");
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

Tray::State Tray::decide(const ProbeSample &s, const AppConfig &cfg, State prev,
                         std::optional<double> prevSomeAvg10) {
  auto rank = [](State st) { return static_cast<int>(st); };
  const int p = rank(prev);

  const long memCritThr = (p >= rank(State::Red))
                              ? cfg.mem.available_crit_exit_kib
                              : cfg.mem.available_crit_kib;
  const long swapCritThr = (p >= rank(State::Red)) ? cfg.swap.free_crit_exit_kib
                                                   : cfg.swap.free_crit_kib;
  const double psiCritThr =
      (p >= rank(State::Red)) ? cfg.psi.avg10_crit_exit : cfg.psi.avg10_crit;
  if ((s.mem_available_kib && *s.mem_available_kib <= memCritThr) ||
      (s.swap_free_kib && *s.swap_free_kib <= swapCritThr) ||
      s.some.avg10 >= psiCritThr)
    return State::Red;

  const long memWarnThr = (p >= rank(State::Orange))
                              ? cfg.mem.available_warn_exit_kib
                              : cfg.mem.available_warn_kib;
  const long swapWarnThr = (p >= rank(State::Orange))
                               ? cfg.swap.free_warn_exit_kib
                               : cfg.swap.free_warn_kib;
  if ((s.mem_available_kib && *s.mem_available_kib <= memWarnThr) ||
      (s.swap_free_kib && *s.swap_free_kib <= swapWarnThr))
    return State::Orange;

  const long memWarnMarginThr = cfg.mem.available_warn_exit_kib;
  const long swapWarnMarginThr = cfg.swap.free_warn_exit_kib;
  if ((s.mem_available_kib && *s.mem_available_kib <= memWarnMarginThr) ||
      (s.swap_free_kib && *s.swap_free_kib <= swapWarnMarginThr))
    return State::Yellow;

  if (prevSomeAvg10) {
    double rate = (s.some.avg10 - *prevSomeAvg10) /
                  (static_cast<double>(cfg.sample_interval_ms) / 1000.0);
    if (rate >= cfg.psi.avg10_deriv_warn)
      return State::Yellow;
  }

  const double psiWarnThr =
      (p >= rank(State::Yellow)) ? cfg.psi.avg10_warn_exit : cfg.psi.avg10_warn;
  if (s.some.avg10 >= psiWarnThr)
    return State::Yellow;

  return State::Green;
}

void Tray::refresh() {
  auto sOpt = probe_->sample();
  if (!sOpt) {
    icon_.setIcon(QIcon(cfg_.palette.black));
    return;
  }
  const auto &s = *sOpt;
  auto nextState = decide(s, cfg_, state_, prevSomeAvg10_);
  bool updateTip = true;
  if (tooltipSample_) {
    auto diffPct = [](double a, double b) {
      return (a == 0.0) ? std::abs(b) : std::abs(a - b) / std::abs(a);
    };
    auto crosses = [](double prev, double cur, double thr) {
      return (prev <= thr && cur > thr) || (prev > thr && cur <= thr);
    };
    const auto &prev = *tooltipSample_;
    updateTip = false;

    if (prev.mem_available_kib != s.mem_available_kib) {
      if (!prev.mem_available_kib || !s.mem_available_kib)
        updateTip = true;
      else {
        double oldVal = static_cast<double>(*prev.mem_available_kib);
        double curVal = static_cast<double>(*s.mem_available_kib);
        if (diffPct(oldVal, curVal) > 0.05)
          updateTip = true;
        if (crosses(oldVal, curVal,
                    static_cast<double>(cfg_.mem.available_warn_kib)) ||
            crosses(oldVal, curVal,
                    static_cast<double>(cfg_.mem.available_crit_kib)))
          updateTip = true;
      }
    }

    if (!updateTip) {
      double oldSome = prev.some.avg10;
      double curSome = s.some.avg10;
      if (diffPct(oldSome, curSome) > 0.05 ||
          crosses(oldSome, curSome, cfg_.psi.avg10_warn) ||
          crosses(oldSome, curSome, cfg_.psi.avg10_crit))
        updateTip = true;
    }

    if (!updateTip) {
      double oldFull = prev.full.avg10;
      double curFull = s.full.avg10;
      if (diffPct(oldFull, curFull) > 0.05)
        updateTip = true;
    }
  }

  if (updateTip) {
    tooltipCache_ = buildTooltip(s, cfg_, nextState);
    tooltipSample_ = s;
  }
  icon_.setToolTip(tooltipCache_);
  state_ = nextState;
  prevSomeAvg10_ = s.some.avg10;
  QString iconPath = cfg_.palette.black;
  switch (state_) {
  case State::Green:
    iconPath = cfg_.palette.green;
    break;
  case State::Yellow:
    iconPath = cfg_.palette.yellow;
    break;
  case State::Orange:
    iconPath = cfg_.palette.orange;
    break;
  case State::Red:
    iconPath = cfg_.palette.red;
    break;
  }
  icon_.setIcon(QIcon(iconPath));
#ifdef HAVE_AYATANA_APPINDICATOR3
  if (indicator_) {
    app_indicator_set_title(indicator_, tooltipCache_.toUtf8().constData());
    app_indicator_set_icon_full(indicator_, iconPath.toUtf8().constData(),
                                tooltipCache_.toUtf8().constData());
  }
#endif
}
