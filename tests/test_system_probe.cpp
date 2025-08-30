#include <catch2/catch_all.hpp>
#include "system_probe.h"

TEST_CASE("parse PSI memory line") {
    std::string line = "some avg10=1.23 avg60=4.56 total=789";
    auto parsed = SystemProbe::parsePsiMemoryLine(line);
    REQUIRE(parsed);
    CHECK(parsed->first == Catch::Approx(1.23));
    CHECK(parsed->second == Catch::Approx(4.56));
}

TEST_CASE("parse PSI memory invalid line returns nullopt") {
    auto parsed = SystemProbe::parsePsiMemoryLine("foo");
    REQUIRE_FALSE(parsed);
}

TEST_CASE("sample provides non-negative values") {
    SystemProbe probe;
    auto s = probe.sample();
    REQUIRE(s.mem_available_kib >= 0);
    REQUIRE(s.psi_mem_avg10 >= 0.0);
    REQUIRE(s.psi_mem_avg60 >= 0.0);
}
