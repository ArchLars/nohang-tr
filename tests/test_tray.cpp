#include <catch2/catch_all.hpp>
#include "tray.h"

TEST_CASE("buildTooltip formats values") {
    ProbeSample s;
    s.mem_available_kib = 1234;
    s.psi_mem_avg10 = 0.5;
    s.psi_mem_avg60 = 1.5;
    auto tooltip = Tray::buildTooltip(s).toStdString();
    REQUIRE(tooltip.find("MemAvailable: 1234 KiB") != std::string::npos);
    REQUIRE(tooltip.find("PSI mem avg10: 0.50") != std::string::npos);
    REQUIRE(tooltip.find("PSI mem avg60: 1.50") != std::string::npos);
}

TEST_CASE("decide returns expected state") {
    AppConfig cfg;
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_crit_kib - 1;
    REQUIRE(Tray::decide(s, cfg) == Tray::State::Red);
    s.mem_available_kib = cfg.mem.available_warn_kib - 1;
    s.psi_mem_avg10 = 0.0;
    REQUIRE(Tray::decide(s, cfg) == Tray::State::Orange);
    s.mem_available_kib = cfg.mem.available_warn_kib + 1;
    s.psi_mem_avg10 = cfg.psi.avg10_warn + 0.1;
    REQUIRE(Tray::decide(s, cfg) == Tray::State::Yellow);
    s.psi_mem_avg10 = cfg.psi.avg10_warn - 0.1;
    REQUIRE(Tray::decide(s, cfg) == Tray::State::Green);
}
