#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <cstdlib>
#include "config.h"

namespace {
QString resourcePath(const QString& relative) {
    QDir root(QCoreApplication::applicationDirPath());
    root.cd("..");
    root.cd("..");
    return root.filePath(relative);
}
} // namespace

TEST_CASE("default config values load") {
    AppConfig cfg;
    REQUIRE(cfg.psi.avg10_warn > 0.0);
    REQUIRE(cfg.mem.available_warn_kib > 0);
}

TEST_CASE("default exit thresholds derive from entry thresholds") {
    AppConfig cfg;
    CHECK(cfg.mem.available_warn_exit_kib == cfg.mem.available_warn_kib * 6 / 5);
    CHECK(cfg.mem.available_crit_exit_kib == cfg.mem.available_crit_kib * 6 / 5);
    CHECK(cfg.psi.avg10_warn_exit == Catch::Approx(cfg.psi.avg10_warn * 0.8));
    CHECK(cfg.psi.avg10_crit_exit == Catch::Approx(cfg.psi.avg10_crit * 0.8));
}

TEST_CASE("load values from example config") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QDir().mkpath(dir.filePath("nohang"));
    QFile(dir.filePath("nohang/nohang.conf")).open(QIODevice::WriteOnly); // empty file
    QByteArray oldXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", dir.path().toLocal8Bit());

    AppConfig cfg;
    REQUIRE(cfg.load(resourcePath("config/nohang-tr.example.toml")));
    CHECK(cfg.psi.avg10_warn == Catch::Approx(0.50));
    CHECK(cfg.psi.avg10_crit == Catch::Approx(1.00));
    CHECK(cfg.mem.available_warn_kib == 524288);
    CHECK(cfg.mem.available_crit_kib == 262144);
    CHECK(cfg.palette.green.endsWith("shield-green.svg"));
    CHECK(cfg.palette.red.endsWith("shield-red.svg"));
    CHECK(cfg.sample_interval_ms == 2000);

    if (oldXdg.isNull())
        ::unsetenv("XDG_CONFIG_HOME");
    else
        ::setenv("XDG_CONFIG_HOME", oldXdg.constData(), 1);
}

TEST_CASE("load PSI trigger thresholds from config") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QDir().mkpath(dir.filePath("nohang"));
    QFile(dir.filePath("nohang/nohang.conf")).open(QIODevice::WriteOnly);
    QByteArray oldXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", dir.path().toLocal8Bit());

    QTemporaryFile tmp;
    REQUIRE(tmp.open());
    QTextStream ts(&tmp);
    ts << "[psi.trigger]\n";
    ts << "some = \"10 100\"\n";
    ts << "full = \"20 200\"\n";
    ts.flush();
    AppConfig cfg;
    REQUIRE(cfg.load(tmp.fileName()));
    REQUIRE(cfg.psi.trigger.some);
    CHECK(cfg.psi.trigger.some->stall_us == 10);
    CHECK(cfg.psi.trigger.some->window_us == 100);
    REQUIRE(cfg.psi.trigger.full);
    CHECK(cfg.psi.trigger.full->stall_us == 20);
    CHECK(cfg.psi.trigger.full->window_us == 200);

    if (oldXdg.isNull())
        ::unsetenv("XDG_CONFIG_HOME");
    else
        ::setenv("XDG_CONFIG_HOME", oldXdg.constData(), 1);
}

TEST_CASE("load thresholds from nohang config") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString nhDir = dir.filePath("nohang");
    QDir().mkpath(nhDir);
    QFile nhConf(nhDir + "/nohang.conf");
    REQUIRE(nhConf.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream ts(&nhConf);
        ts << "warning_threshold_min_mem = 512 M\n";
        ts << "soft_threshold_min_mem = 256 M\n";
        ts << "hard_threshold_min_mem = 128 M\n";
        ts << "warning_threshold_max_psi = 10\n";
        ts << "soft_threshold_max_psi = 20\n";
        ts << "hard_threshold_max_psi = 30\n";
        ts.flush();
    }
    nhConf.close();

    QByteArray oldXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", dir.path().toLocal8Bit());

    QTemporaryFile appConf;
    REQUIRE(appConf.open());
    AppConfig cfg;
    REQUIRE(cfg.load(appConf.fileName()));

    CHECK(cfg.mem.available_warn_kib == 512 * 1024);
    CHECK(cfg.mem.available_warn_exit_kib == 512 * 1024 * 6 / 5);
    CHECK(cfg.mem.available_crit_kib == 256 * 1024);
    CHECK(cfg.mem.available_crit_exit_kib == 128 * 1024);
    CHECK(cfg.psi.avg10_warn == Catch::Approx(0.10));
    CHECK(cfg.psi.avg10_warn_exit == Catch::Approx(0.08));
    CHECK(cfg.psi.avg10_crit == Catch::Approx(0.20));
    CHECK(cfg.psi.avg10_crit_exit == Catch::Approx(0.30));

    if (oldXdg.isNull())
        ::unsetenv("XDG_CONFIG_HOME");
    else
        ::setenv("XDG_CONFIG_HOME", oldXdg.constData(), 1);
}
