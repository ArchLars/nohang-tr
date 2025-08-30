#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
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

TEST_CASE("load PSI trigger thresholds from config") {
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
}
