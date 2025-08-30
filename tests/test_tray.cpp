#include <catch2/catch_all.hpp>
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <memory>
#define private public
#include "tray.h"
#undef private

namespace {
std::unique_ptr<QApplication> app = [] {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 0;
    char* argv[] = { (char*)"test", nullptr };
    return std::make_unique<QApplication>(argc, argv);
}();

QString resourcePath(const QString& relative) {
    QDir root(QCoreApplication::applicationDirPath());
    root.cd("..");
    root.cd("..");
    return root.filePath(relative);
}

void applyPalette(Tray& tray) {
    tray.cfg_.palette.green = resourcePath("res/icons/shield-green.svg");
    tray.cfg_.palette.yellow = resourcePath("res/icons/shield-yellow.svg");
    tray.cfg_.palette.orange = resourcePath("res/icons/shield-orange.svg");
    tray.cfg_.palette.red = resourcePath("res/icons/shield-red.svg");
}

struct StubProbe : SystemProbe {
    ProbeSample s;
    explicit StubProbe(const ProbeSample& sample) : s(sample) {}
    std::optional<ProbeSample> sample() const override { return s; }
};
} // namespace

TEST_CASE("buildTooltip formats values") {
    ProbeSample s;
    s.mem_available_kib = 1234;
    s.some.avg10 = 0.5;
    s.some.avg60 = 1.5;
    s.full.avg10 = 1.5;
    AppConfig cfg;
    auto tooltip = Tray::buildTooltip(s, cfg).toStdString();
    REQUIRE(tooltip.find("MemAvailable: 1.2 MiB") != std::string::npos);
    REQUIRE(tooltip.find("warn 512.0 MiB") != std::string::npos);
    REQUIRE(tooltip.find("crit 256.0 MiB") != std::string::npos);
    REQUIRE(tooltip.find("PSI some avg10: 0.50 (warn 0.50, crit 1.00)") != std::string::npos);
    REQUIRE(tooltip.find("PSI full avg10: 1.50") != std::string::npos);
}

TEST_CASE("buildTooltip clamps percentage at 100") {
    AppConfig cfg;
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_kib * 2;
    s.some.avg10 = cfg.psi.avg10_warn * 2;
    auto tooltip = Tray::buildTooltip(s, cfg).toStdString();
    size_t pos = tooltip.find("100%");
    REQUIRE(pos != std::string::npos);
    pos = tooltip.find("100%", pos + 1);
    REQUIRE(pos != std::string::npos);
}

TEST_CASE("decide returns expected state") {
    AppConfig cfg;
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_crit_kib - 1;
    REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Red);
    s.mem_available_kib = cfg.mem.available_warn_kib - 1;
    s.some.avg10 = 0.0;
    REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Orange);
    s.mem_available_kib = cfg.mem.available_warn_kib + 1;
    s.some.avg10 = cfg.psi.avg10_warn + 0.1;
    REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Yellow);
    s.some.avg10 = cfg.psi.avg10_warn - 0.1;
    REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Green);
}

TEST_CASE("hysteresis prevents state flapping") {
    AppConfig cfg;

    SECTION("psi warn") {
        cfg.psi.avg10_warn = 0.5;
        cfg.psi.avg10_warn_exit = 0.4;
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib + 1;
        s.some.avg10 = 0.51;
        auto state = Tray::decide(s, cfg, Tray::State::Green);
        REQUIRE(state == Tray::State::Yellow);
        s.some.avg10 = 0.45; // between enter and exit
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Yellow);
        s.some.avg10 = 0.39; // below exit
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Green);
        s.some.avg10 = 0.45; // between enter and exit
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Green);
    }

    SECTION("mem warn") {
        cfg.mem.available_warn_kib = 512 * 1024;
        cfg.mem.available_warn_exit_kib = cfg.mem.available_warn_kib + 1024;
        ProbeSample s;
        s.some.avg10 = 0.0;
        s.mem_available_kib = cfg.mem.available_warn_kib - 1;
        auto state = Tray::decide(s, cfg, Tray::State::Green);
        REQUIRE(state == Tray::State::Orange);
        s.mem_available_kib = cfg.mem.available_warn_kib + 512; // between enter and exit
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Orange);
        s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Green);
        s.mem_available_kib = cfg.mem.available_warn_exit_kib - 1; // between thresholds
        state = Tray::decide(s, cfg, state);
        REQUIRE(state == Tray::State::Green);
    }
}

TEST_CASE("Tray show makes icon visible and starts timer") {
    ProbeSample s; // defaults ok
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    tray.show();
    CHECK(tray.icon_.isVisible());
    CHECK(tray.timer_.isActive());
}

TEST_CASE("refresh sets icon color for each state") {
    AppConfig cfg;

    SECTION("green") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib + 1;
        s.some.avg10 = cfg.psi.avg10_warn - 0.1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s, tray.cfg_));
    }

    SECTION("yellow") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib + 1;
        s.some.avg10 = cfg.psi.avg10_warn + 0.1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s, tray.cfg_));
    }

    SECTION("orange") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib - 1;
        s.some.avg10 = 0.0;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s, tray.cfg_));
    }

    SECTION("red") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_crit_kib - 1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s, tray.cfg_));
    }
}
