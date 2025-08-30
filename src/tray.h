#pragma once
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>
#include "config.h"
#include "system_probe.h"

class Tray : public QObject {
    Q_OBJECT
public:
    explicit Tray(QObject* parent=nullptr, std::unique_ptr<SystemProbe> probe=nullptr,
                  const QString& configPath=QString());
    void show();

    enum class State { Green, Yellow, Orange, Red };
    static QString buildTooltip(const ProbeSample& s);
    static State decide(const ProbeSample& s, const AppConfig& cfg, State prev);

private:
    void refresh();

    QSystemTrayIcon icon_;
    QTimer timer_;
    AppConfig cfg_;
    std::unique_ptr<SystemProbe> probe_;
    State state_ = State::Green;
};
