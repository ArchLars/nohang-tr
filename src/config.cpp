#include "config.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <cstdlib>
#include <filesystem>

bool AppConfig::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream ts(&f);
    QString section;
    while (!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        if (line.startsWith('[') && line.endsWith(']')) {
            section = line.mid(1, line.size() - 2);
            continue;
        }
        int eq = line.indexOf('=');
        if (eq == -1)
            continue;
        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();
        int comment = value.indexOf('#');
        if (comment != -1)
            value = value.left(comment).trimmed();
        if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
            value = value.mid(1, value.size() - 2);

        bool ok = false;
        if (section == "psi") {
            double v = value.toDouble(&ok);
            if (ok) {
                if (key == "avg10_warn")
                    psi.avg10_warn = v;
                else if (key == "avg10_warn_exit")
                    psi.avg10_warn_exit = v;
                else if (key == "avg10_crit")
                    psi.avg10_crit = v;
                else if (key == "avg10_crit_exit")
                    psi.avg10_crit_exit = v;
            }
        } else if (section == "psi.trigger") {
            auto parts = value.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                long stall = parts[0].toLong(&ok);
                bool ok2 = false;
                long window = parts[1].toLong(&ok2);
                if (ok && ok2) {
                    Psi::Trigger trig{stall, window};
                    if (key == "some")
                        psi.trigger.some = trig;
                    else if (key == "full")
                        psi.trigger.full = trig;
                }
            }
        } else if (section == "mem") {
            long v = value.toLong(&ok);
            if (ok) {
                if (key == "available_warn_kib")
                    mem.available_warn_kib = v;
                else if (key == "available_warn_exit_kib")
                    mem.available_warn_exit_kib = v;
                else if (key == "available_crit_kib")
                    mem.available_crit_kib = v;
                else if (key == "available_crit_exit_kib")
                    mem.available_crit_exit_kib = v;
            }
        } else if (section == "ui.palette") {
            if (key == "green")
                palette.green = value;
            else if (key == "yellow")
                palette.yellow = value;
            else if (key == "orange")
                palette.orange = value;
            else if (key == "red")
                palette.red = value;
        } else if (section == "sample") {
            int v = value.toInt(&ok);
            if (ok && key == "interval_ms")
                sample_interval_ms = v;
        }
    }
    return true;
}

QString resolveConfigPath(const QString& cliPath) {
    if (!cliPath.isEmpty())
        return cliPath;
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::filesystem::path base;
    if (xdg && *xdg) {
        base = xdg;
    } else {
        const char* home = std::getenv("HOME");
        if (home && *home)
            base = std::filesystem::path(home) / ".config";
        else
            base = std::filesystem::path(".config");
    }
    base /= "nohang-tr";
    base /= "nohang-tr.toml";
    return QString::fromStdString(base.string());
}

