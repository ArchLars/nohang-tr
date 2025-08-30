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
    ProbeSample sample() const override { return s; }
};
} // namespace

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
        s.psi_mem_avg10 = cfg.psi.avg10_warn - 0.1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s));
    }

    SECTION("yellow") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib + 1;
        s.psi_mem_avg10 = cfg.psi.avg10_warn + 0.1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s));
    }

    SECTION("orange") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_warn_kib - 1;
        s.psi_mem_avg10 = 0.0;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s));
    }

    SECTION("red") {
        ProbeSample s;
        s.mem_available_kib = cfg.mem.available_crit_kib - 1;
        Tray tray(nullptr, std::make_unique<StubProbe>(s));
        applyPalette(tray);
        tray.refresh();
        CHECK(tray.icon_.toolTip() == Tray::buildTooltip(s));
    }
}
