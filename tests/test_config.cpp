#include <catch2/catch_all.hpp>
#include "config.h"

TEST_CASE("default config values load") {
    AppConfig cfg;
    REQUIRE(cfg.psi.avg10_warn > 0.0);
    REQUIRE(cfg.mem.available_warn_kib > 0);
}
