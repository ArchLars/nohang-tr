#pragma once
#include <QString>
#include <optional>

struct AppConfig {
  /**
   * @brief Where configuration values were sourced from.
   */
  enum class Source {
    Default, ///< Built-in defaults.
    Nohang   ///< Parsed from nohang.conf.
  };

  Source source = Source::Default;
  struct Psi {
    struct Trigger {
      long stall_us = 0;
      long window_us = 0;
    };
    double avg10_warn = 0.5;
    double avg10_warn_exit = 0.4; // 20% below warn
    double avg10_crit = 1.0;
    double avg10_crit_exit = 0.8;  // 20% below crit
    double avg10_deriv_warn = 0.1; ///< avg10 rise/sec triggering yellow
    struct {
      std::optional<Trigger> some;
      std::optional<Trigger> full;
    } trigger;
  } psi;

  struct {
    long available_warn_kib = 512 * 1024;
    long available_warn_exit_kib =
        512 * 1024 * 6 / 5; // 20% above warn, Yellow margin
    long available_crit_kib = 256 * 1024;
    long available_crit_exit_kib = 256 * 1024 * 6 / 5; // 20% above crit
  } mem;

  struct {
    long free_warn_kib = 512 * 1024;
    long free_warn_exit_kib = 512 * 1024 * 6 / 5; // 20% above warn
    long free_crit_kib = 256 * 1024;
    long free_crit_exit_kib = 256 * 1024 * 6 / 5; // 20% above crit
  } swap;

  struct {
    // Icon theme names or file paths for tray colors.
    QString green = "shield-green";
    QString yellow = "shield-yellow";
    QString orange = "shield-orange";
    QString red = "shield-red";
    QString black = "shield-black";
  } palette;

  int sample_interval_ms = 2000;
  /**
   * Load configuration values from a TOML file.
   *
   * The file is parsed for keys used by AppConfig. Missing keys
   * keep their default values. Returns false if the file cannot be
   * read.
   */
  bool load(const QString &path);
};

/**
 * Determine the configuration file path.
 *
 * When \a cliPath is provided it is returned directly. Otherwise the path is
 * resolved according to the XDG Base Directory specification using
 * `XDG_CONFIG_HOME` and falling back to
 * `$HOME/.config/nohang-tr/nohang-tr.toml`.
 */
QString resolveConfigPath(const QString &cliPath = {});
