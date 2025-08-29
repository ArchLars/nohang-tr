#include <QApplication>
#include "tray.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning("System tray not available");
        return 1;
    }
    Tray tray;
    tray.show();
    return app.exec();
}
