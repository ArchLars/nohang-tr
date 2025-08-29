#pragma once
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>
#include "config.h"
#include "system_probe.h"

class Tray : public QObject {
    Q_OBJECT
public:
    explicit Tray(QObject* parent=nullptr);
    void show();

private:
    void refresh();
    QString buildTooltip(const ProbeSample& s) const;
    enum class State { Green, Yellow, Orange, Red };
    State decide(const ProbeSample& s) const;

    QSystemTrayIcon icon_;
    QTimer timer_;
    AppConfig cfg_;
    std::unique_ptr<SystemProbe> probe_;
};
