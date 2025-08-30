#include <QApplication>
#include <QCommandLineParser>
#include "config.h"
#include "tray.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption configOpt({"c", "config"}, "Path to configuration file", "path");
    parser.addOption(configOpt);
    parser.addHelpOption();
    parser.process(app);

    QString configPath = resolveConfigPath(parser.value(configOpt));

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning("System tray not available");
        return 1;
    }
    Tray tray(nullptr, nullptr, configPath);
    tray.show();
    return app.exec();
}
