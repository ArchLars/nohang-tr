#pragma once
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>
#include "config.h"
#include "system_probe.h"

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
    explicit Tray(QObject* parent=nullptr, std::unique_ptr<SystemProbe> probe=nullptr,
                  const QString& configPath=QString());

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
     * @brief Build tooltip text from a probe sample.
     */
    static QString buildTooltip(const ProbeSample& s);

    /**
     * @brief Decide next state based on a sample and previous state.
     */
    static State decide(const ProbeSample& s, const AppConfig& cfg, State prev);

private:
    void refresh();

    QSystemTrayIcon icon_;
    QTimer timer_;
    AppConfig cfg_;
    std::unique_ptr<SystemProbe> probe_;
    State state_ = State::Green;
};
