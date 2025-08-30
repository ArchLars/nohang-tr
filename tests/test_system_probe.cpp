#include <catch2/catch_all.hpp>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
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

TEST_CASE("sample reads from provided paths") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "psi_sample";
    fs::create_directories(dir);
    fs::path mem = dir / "meminfo";
    fs::path psi = dir / "pressure";
    {
        std::ofstream out(mem);
        out << "MemAvailable: 123 kB\n";
    }
    {
        std::ofstream out(psi);
        out << "some avg10=1 avg60=2 avg300=3 total=4\n";
        out << "full avg10=5 avg60=6 avg300=7 total=8\n";
    }
    SystemProbe probe(mem.string(), psi.string());
    auto s = probe.sample();
    REQUIRE(s);
    REQUIRE(s->mem_available_kib);
    CHECK(*s->mem_available_kib == 123);
    CHECK(s->some.total == 4);
    CHECK(s->full.total == 8);
}

TEST_CASE("enableTriggers writes thresholds") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "psi_triggers";
    fs::create_directories(dir);
    fs::path mem = dir / "meminfo";
    fs::path psi = dir / "pressure";
    fs::path trig = dir / "trig";
    {
        std::ofstream out(mem);
        out << "MemAvailable: 0 kB\n";
    }
    {
        std::ofstream out(psi);
        out << "some avg10=0 avg60=0 avg300=0 total=0\n";
        out << "full avg10=0 avg60=0 avg300=0 total=0\n";
    }
    {
        std::ofstream out(trig);
    }
    SystemProbe probe(mem.string(), psi.string());
    SystemProbe::Trigger t{SystemProbe::PsiType::Some, 10, 100};
    REQUIRE(probe.enableTriggers(trig.string(), {t}));
    std::ifstream in(trig);
    std::string line;
    std::getline(in, line);
    CHECK(line == "some 10 100");
}

TEST_CASE("sample logs missing PSI file") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "psi_missing";
    fs::create_directories(dir);
    fs::path mem = dir / "meminfo";
    {
        std::ofstream out(mem);
        out << "MemAvailable: 1 kB\n";
    }
    fs::path psi = dir / "pressure";
    SystemProbe probe(mem.string(), psi.string());
    std::ostringstream err;
    auto* old = std::cerr.rdbuf(err.rdbuf());
    auto s = probe.sample();
    std::cerr.rdbuf(old);
    REQUIRE_FALSE(s);
    CHECK(err.str().find("not found") != std::string::npos);
}

TEST_CASE("sample logs incomplete PSI data") {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "psi_incomplete";
    fs::create_directories(dir);
    fs::path mem = dir / "meminfo";
    {
        std::ofstream out(mem);
        out << "MemAvailable: 1 kB\n";
    }
    fs::path psi = dir / "pressure";
    {
        std::ofstream out(psi);
        out << "some avg10=1 avg60=2 avg300=3 total=4\n"; // missing full line
    }
    SystemProbe probe(mem.string(), psi.string());
    std::ostringstream err;
    auto* old = std::cerr.rdbuf(err.rdbuf());
    auto s = probe.sample();
    std::cerr.rdbuf(old);
    REQUIRE_FALSE(s);
    CHECK(err.str().find("incomplete") != std::string::npos);
}
