#include <catch2/catch_all.hpp>
#include <sstream>
#include "system_probe.h"

TEST_CASE("parse MemAvailable returns value") {
    std::istringstream ss("MemTotal: 1 kB\nMemAvailable: 123 kB\n");
    auto v = parseMemAvailable(ss);
    REQUIRE(v);
    CHECK(*v == 123);
}

TEST_CASE("parse MemAvailable missing returns nullopt") {
    std::istringstream ss("MemTotal: 1 kB\n");
    auto v = parseMemAvailable(ss);
    REQUIRE_FALSE(v);
}

TEST_CASE("parse PSI memory some line") {
    std::string line = "some avg10=1.23 avg60=4.56 avg300=7.89 total=789";
    auto parsed = SystemProbe::parsePsiMemoryLine(line);
    REQUIRE(parsed);
    CHECK(parsed->first == SystemProbe::PsiType::Some);
    CHECK(parsed->second.avg10 == Catch::Approx(1.23));
    CHECK(parsed->second.avg60 == Catch::Approx(4.56));
    CHECK(parsed->second.avg300 == Catch::Approx(7.89));
    CHECK(parsed->second.total == 789);
}

TEST_CASE("parse PSI memory full line") {
    std::string line = "full avg10=0.1 avg60=0.2 avg300=0.3 total=10";
    auto parsed = SystemProbe::parsePsiMemoryLine(line);
    REQUIRE(parsed);
    CHECK(parsed->first == SystemProbe::PsiType::Full);
    CHECK(parsed->second.avg10 == Catch::Approx(0.1));
    CHECK(parsed->second.avg60 == Catch::Approx(0.2));
    CHECK(parsed->second.avg300 == Catch::Approx(0.3));
    CHECK(parsed->second.total == 10);
}

TEST_CASE("parse PSI memory invalid line returns nullopt") {
    auto parsed = SystemProbe::parsePsiMemoryLine("foo");
    REQUIRE_FALSE(parsed);
}

TEST_CASE("sample provides non-negative values") {
    SystemProbe probe;
    auto sOpt = probe.sample();
    if (sOpt) {
        const auto& s = *sOpt;
        if (s.mem_available_kib)
            REQUIRE(*s.mem_available_kib >= 0);
        REQUIRE(s.some.avg10 >= 0.0);
        REQUIRE(s.some.avg60 >= 0.0);
        REQUIRE(s.some.avg300 >= 0.0);
        REQUIRE(s.some.total >= 0);
        REQUIRE(s.full.avg10 >= 0.0);
        REQUIRE(s.full.avg60 >= 0.0);
        REQUIRE(s.full.avg300 >= 0.0);
        REQUIRE(s.full.total >= 0);
    } else {
        SUCCEED("PSI data unavailable");
    }
}
