#pragma once
#include <QSystemTrayIcon>
#include <QTimer>
#ifdef HAVE_AYATANA_APPINDICATOR3
struct _AppIndicator;
using AppIndicator = _AppIndicator;
struct _GtkWidget;
using GtkWidget = _GtkWidget;
#endif
#include "config.h"
#include "system_probe.h"
#include <memory>
#include <optional>

/**
 * @brief Maintains a system tray icon reflecting memory pressure.
 *
 * Tray owns a SystemProbe, periodically samples system memory pressure and
 * updates the tray icon color and tooltip accordingly.
 */
class Tray : public QObject {
  Q_OBJECT
public:
  /**
   * @brief Construct a Tray instance.
   * @param parent Qt parent object.
   * @param probe Optional probe implementation.
   * @param configPath Path to configuration file.
   */
  explicit Tray(QObject *parent = nullptr,
                std::unique_ptr<SystemProbe> probe = nullptr,
                const QString &configPath = QString());

  /** Show the tray icon and start periodic updates. */
  void show();

  /**
   * @brief Color-coded memory pressure states.
   */
  enum class State {
    Green,  ///< Normal memory pressure.
    Yellow, ///< Mild pressure.
    Orange, ///< High pressure.
    Red     ///< Critical pressure.
  };

  /**
   * @brief Build tooltip text from a probe sample and configuration.
   */
  static QString buildTooltip(const ProbeSample &s, const AppConfig &cfg,
                              State state);

  /**
   * @brief Decide next state based on a sample and previous state.
   * @param prevSomeAvg10 Previous PSI some avg10 value to compute rate.
   */
  static State decide(const ProbeSample &s, const AppConfig &cfg, State prev,
                      std::optional<double> prevSomeAvg10 = std::nullopt);

  /**
   * @brief Determine whether to use Ayatana AppIndicator integration.
   */
  static bool shouldUseAppIndicator();

private:
  void refresh();
#ifdef HAVE_AYATANA_APPINDICATOR3
  AppIndicator *indicator_ = nullptr;
  GtkWidget *menuIndicator_ = nullptr;
#endif
  QSystemTrayIcon icon_;
  QTimer timer_;
  AppConfig cfg_;
  std::unique_ptr<SystemProbe> probe_;
  State state_ = State::Green;
  std::optional<double> prevSomeAvg10_;
  QString tooltipCache_;
  std::optional<ProbeSample> tooltipSample_;
};
