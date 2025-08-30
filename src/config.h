#pragma once
#include <QString>

struct AppConfig {
    struct {
        double avg10_warn = 0.5;
        double avg10_crit = 1.0;
    } psi;

    struct {
        long available_warn_kib = 512 * 1024;
        long available_crit_kib = 256 * 1024;
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
