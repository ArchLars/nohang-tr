#pragma once
#include <QString>

struct AppConfig {
    struct {
        double avg10_warn = 0.5;
        double avg10_warn_exit = 0.5;
        double avg10_crit = 1.0;
        double avg10_crit_exit = 1.0;
    } psi;

    struct {
        long available_warn_kib = 512 * 1024;
        long available_warn_exit_kib = 512 * 1024;
        long available_crit_kib = 256 * 1024;
        long available_crit_exit_kib = 256 * 1024;
    } mem;

    struct {
        QString green = "res/icons/shield-green.svg";
        QString yellow = "res/icons/shield-yellow.svg";
        QString orange = "res/icons/shield-orange.svg";
        QString red = "res/icons/shield-red.svg";
    } palette;

    int sample_interval_ms = 2000;
    /**
     * Load configuration values from a TOML file.
     *
     * The file is parsed for keys used by AppConfig. Missing keys
     * keep their default values. Returns false if the file cannot be
     * read.
     */
    bool load(const QString& path);
};

/**
 * Determine the configuration file path.
 *
 * When \a cliPath is provided it is returned directly. Otherwise the path is
 * resolved according to the XDG Base Directory specification using
 * `XDG_CONFIG_HOME` and falling back to `$HOME/.config/nohang-tr/nohang-tr.toml`.
 */
QString resolveConfigPath(const QString& cliPath = {});
