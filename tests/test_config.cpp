#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QDir>
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

TEST_CASE("load values from example config") {
    AppConfig cfg;
    REQUIRE(cfg.load(resourcePath("config/nohang-tr.example.toml")));
    CHECK(cfg.psi.avg10_warn == Catch::Approx(0.50));
    CHECK(cfg.psi.avg10_crit == Catch::Approx(1.00));
    CHECK(cfg.mem.available_warn_kib == 524288);
    CHECK(cfg.mem.available_crit_kib == 262144);
    CHECK(cfg.palette.green.endsWith("shield-green.svg"));
    CHECK(cfg.palette.red.endsWith("shield-red.svg"));
    CHECK(cfg.sample_interval_ms == 2000);
}
