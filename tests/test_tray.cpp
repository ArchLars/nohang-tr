#include <QApplication>
#include <QDir>
#include <QIcon>
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#define private public
#include "tray.h"
#undef private

namespace {
std::unique_ptr<QApplication> app = [] {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  int argc = 0;
  char *argv[] = {(char *)"test", nullptr};
  return std::make_unique<QApplication>(argc, argv);
}();

QString resourcePath(const QString &relative) {
  QDir root(QCoreApplication::applicationDirPath());
  root.cd("..");
  root.cd("..");
  return root.filePath(relative);
}

void applyPalette(Tray &tray) {
  tray.cfg_.palette.green = resourcePath("res/icons/shield-green.svg");
  tray.cfg_.palette.yellow = resourcePath("res/icons/shield-yellow.svg");
  tray.cfg_.palette.orange = resourcePath("res/icons/shield-orange.svg");
  tray.cfg_.palette.red = resourcePath("res/icons/shield-red.svg");
  tray.cfg_.palette.black = resourcePath("res/icons/shield-black.svg");
}

struct StubProbe : SystemProbe {
  ProbeSample s;
  explicit StubProbe(const ProbeSample &sample) : s(sample) {}
  std::optional<ProbeSample> sample() const override { return s; }
};

struct NullProbe : SystemProbe {
  std::optional<ProbeSample> sample() const override { return std::nullopt; }
};
} // namespace

TEST_CASE("icons load") {
  std::vector<QString> icons = {
      resourcePath("res/icons/shield-green.svg"),
      resourcePath("res/icons/shield-yellow.svg"),
      resourcePath("res/icons/shield-orange.svg"),
      resourcePath("res/icons/shield-red.svg"),
      resourcePath("res/icons/shield-black.svg")};
  for (const auto &path : icons) {
    INFO(path.toStdString());
    CHECK(!QIcon(path).isNull());
  }
}

TEST_CASE("buildTooltip formats values") {
  ProbeSample s;
  s.mem_available_kib = 1234;
  s.mem_total_kib = 2 * 1024 * 1024;
  s.mem_free_kib = 1024;
  s.swap_free_kib = 1234;
  s.cached_kib = 2048;
  s.some.avg10 = 0.5;
  s.some.avg60 = 1.5;
  s.full.avg10 = 1.5;
  AppConfig cfg;
  auto tooltip =
      Tray::buildTooltip(s, cfg, Tray::State::Green).toStdString();
  REQUIRE(tooltip.find("MemAvailable: 1.2 MiB / 2.0 GiB") !=
          std::string::npos);
  REQUIRE(tooltip.find("MemFree: 1.0 MiB") != std::string::npos);
  REQUIRE(tooltip.find("SwapFree: 1.2 MiB") != std::string::npos);
  REQUIRE(tooltip.find("Cached: 2.0 MiB") != std::string::npos);
  REQUIRE(tooltip.find("PSI some avg10: 0.50 (warn 0.50, crit 1.00)") !=
          std::string::npos);
  REQUIRE(tooltip.find("PSI full avg10: 1.50") != std::string::npos);
}

TEST_CASE("buildTooltip indicates config source") {
  ProbeSample s; // defaults ok
  AppConfig cfg;
  auto tip = Tray::buildTooltip(s, cfg, Tray::State::Green).toStdString();
  REQUIRE(tip.find("Config: default") != std::string::npos);

  cfg.source = AppConfig::Source::Nohang;
  tip = Tray::buildTooltip(s, cfg, Tray::State::Green).toStdString();
  REQUIRE(tip.find("Config: nohang") != std::string::npos);
}

TEST_CASE("buildTooltip uses lowest memory indicator") {
  AppConfig cfg;
  ProbeSample s;
  s.mem_available_kib = cfg.mem.available_warn_kib * 2;
  s.mem_free_kib = cfg.mem.available_warn_kib * 2;
  s.swap_free_kib = cfg.swap.free_warn_kib * 2;
  s.cached_kib = cfg.mem.available_warn_kib * 2;

  auto tip =
      Tray::buildTooltip(s, cfg, Tray::State::Green).toStdString();
  REQUIRE(tip.find("Pressure:") != std::string::npos);
  REQUIRE(tip.find("0%") != std::string::npos);
  REQUIRE(tip.find("style='color:green'") != std::string::npos);

  s.mem_free_kib = 0; // forces bar to 100%
  tip = Tray::buildTooltip(s, cfg, Tray::State::Red).toStdString();
  REQUIRE(tip.find("100%") != std::string::npos);
  REQUIRE(tip.find("style='color:red'>██████████") != std::string::npos);
}

TEST_CASE("decide returns expected state") {
  AppConfig cfg;
  ProbeSample s;
  s.mem_available_kib = cfg.mem.available_crit_kib - 1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Red);
  s.mem_available_kib = cfg.mem.available_warn_kib - 1;
  s.some.avg10 = 0.0;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Orange);
  s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
  s.some.avg10 = cfg.psi.avg10_warn + 0.1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Yellow);
  s.some.avg10 = cfg.psi.avg10_warn - 0.1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Green);
}

TEST_CASE("decide warns before reaching memory threshold") {
  AppConfig cfg;
  ProbeSample s;
  s.some.avg10 = 0.0;
  s.mem_available_kib =
      cfg.mem.available_warn_exit_kib - 1; // within margin above warn
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Yellow);
}

TEST_CASE("decide accounts for swap thresholds") {
  AppConfig cfg;
  ProbeSample s;
  s.mem_available_kib = cfg.mem.available_warn_kib * 2;
  s.some.avg10 = 0.0;
  s.swap_free_kib = cfg.swap.free_crit_kib - 1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Red);
  s.swap_free_kib = cfg.swap.free_warn_kib - 1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Orange);
  s.swap_free_kib = cfg.swap.free_warn_exit_kib - 1;
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green) == Tray::State::Yellow);
}

TEST_CASE("decide preempts on rapid PSI increase") {
  AppConfig cfg;
  ProbeSample s;
  s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
  s.some.avg10 = 0.4; // below warn threshold
  // With previous avg10 0.1 and 2s interval, rate = 0.15 > 0.1 threshold
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green, 0.1) ==
          Tray::State::Yellow);
  s.some.avg10 = 0.15; // rate = 0.025 < threshold
  REQUIRE(Tray::decide(s, cfg, Tray::State::Green, 0.1) ==
          Tray::State::Green);
}

TEST_CASE("hysteresis prevents state flapping") {
  AppConfig cfg;

  SECTION("psi warn") {
    cfg.psi.avg10_warn = 0.5;
    cfg.psi.avg10_warn_exit = 0.4;
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
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
    s.mem_available_kib =
        cfg.mem.available_warn_kib + 512; // between enter and exit
    state = Tray::decide(s, cfg, state);
    REQUIRE(state == Tray::State::Orange);
    s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
    state = Tray::decide(s, cfg, state);
    REQUIRE(state == Tray::State::Green);
    s.mem_available_kib =
        cfg.mem.available_warn_exit_kib - 1; // between thresholds
    state = Tray::decide(s, cfg, state);
    REQUIRE(state == Tray::State::Yellow);
  }
}

TEST_CASE("Tray enables configured PSI triggers") {
  namespace fs = std::filesystem;
  fs::path dir = fs::temp_directory_path() / "tray_triggers";
  fs::remove_all(dir);
  fs::create_directories(dir);
  fs::path mem = dir / "meminfo";
  fs::path psi = dir / "pressure";
  {
    std::ofstream(mem.string());
  }
  {
    std::ofstream(psi.string());
  }
  fs::path cfg = dir / "config.toml";
  {
    std::ofstream out(cfg);
    out << "[psi.trigger]\n";
    out << "some = 10 100\n";
    out << "full = 20 200\n";
  }
  auto probe = std::make_unique<SystemProbe>(mem.string(), psi.string());
  Tray tray(nullptr, std::move(probe), QString::fromStdString(cfg.string()));
  std::ifstream in(psi);
  std::string line1, line2;
  std::getline(in, line1);
  std::getline(in, line2);
  REQUIRE(line1 == "some 10 100");
  REQUIRE(line2 == "full 20 200");
}

TEST_CASE("Tray skips enabling PSI triggers when none configured") {
  namespace fs = std::filesystem;
  fs::path dir = fs::temp_directory_path() / "tray_no_triggers";
  fs::remove_all(dir);
  fs::create_directories(dir);
  fs::path mem = dir / "meminfo";
  fs::path psi = dir / "pressure";
  {
    std::ofstream(mem.string());
  }
  {
    std::ofstream(psi.string());
  }
  fs::path cfg = dir / "config.toml";
  {
    std::ofstream(cfg.string());
  }
  auto probe = std::make_unique<SystemProbe>(mem.string(), psi.string());
  {
    Tray tray(nullptr, std::move(probe), QString::fromStdString(cfg.string()));
  }
  std::ifstream in(psi);
  std::string line;
  REQUIRE_FALSE(std::getline(in, line));
}

TEST_CASE("Tray show makes icon visible and starts timer") {
  ProbeSample s; // defaults ok
  Tray tray(nullptr, std::make_unique<StubProbe>(s));
  applyPalette(tray);
  tray.show();
  CHECK(tray.icon_.isVisible());
  CHECK(tray.timer_.isActive());
  auto actual = tray.icon_.icon().pixmap(16, 16).toImage();
  auto expected = QIcon(tray.cfg_.palette.black).pixmap(16, 16).toImage();
  bool same = (actual == expected);
  CHECK(same);
}

TEST_CASE("refresh uses black icon when probe fails") {
  Tray tray(nullptr, std::make_unique<NullProbe>());
  applyPalette(tray);
  tray.refresh();
  auto actual = tray.icon_.icon().pixmap(16, 16).toImage();
  auto expected = QIcon(tray.cfg_.palette.black).pixmap(16, 16).toImage();
  bool same = (actual == expected);
  CHECK(same);
}

TEST_CASE("refresh sets icon color for each state") {
  AppConfig cfg;

  SECTION("green") {
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
    s.some.avg10 = cfg.psi.avg10_warn - 0.1;
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    auto state = Tray::decide(s, tray.cfg_, Tray::State::Green);
    tray.refresh();
    CHECK(tray.icon_.toolTip() ==
          Tray::buildTooltip(s, tray.cfg_, state));
  }

  SECTION("yellow psi") {
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_exit_kib + 1;
    s.some.avg10 = cfg.psi.avg10_warn + 0.1;
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    auto state = Tray::decide(s, tray.cfg_, Tray::State::Green);
    tray.refresh();
    CHECK(tray.icon_.toolTip() ==
          Tray::buildTooltip(s, tray.cfg_, state));
  }

  SECTION("yellow mem") {
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_exit_kib - 1;
    s.some.avg10 = cfg.psi.avg10_warn - 0.1;
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    auto state = Tray::decide(s, tray.cfg_, Tray::State::Green);
    tray.refresh();
    CHECK(tray.icon_.toolTip() ==
          Tray::buildTooltip(s, tray.cfg_, state));
  }

  SECTION("orange") {
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_warn_kib - 1;
    s.some.avg10 = 0.0;
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    auto state = Tray::decide(s, tray.cfg_, Tray::State::Green);
    tray.refresh();
    CHECK(tray.icon_.toolTip() ==
          Tray::buildTooltip(s, tray.cfg_, state));
  }

  SECTION("red") {
    ProbeSample s;
    s.mem_available_kib = cfg.mem.available_crit_kib - 1;
    Tray tray(nullptr, std::make_unique<StubProbe>(s));
    applyPalette(tray);
    auto state = Tray::decide(s, tray.cfg_, Tray::State::Green);
    tray.refresh();
    CHECK(tray.icon_.toolTip() ==
          Tray::buildTooltip(s, tray.cfg_, state));
  }
}

TEST_CASE("refresh caches tooltip until significant change") {
  AppConfig cfg;
  ProbeSample s;
  s.mem_available_kib = cfg.mem.available_warn_kib * 2;
  s.some.avg10 = 0.1;
  auto *probe = new StubProbe(s);
  Tray tray(nullptr, std::unique_ptr<SystemProbe>(probe));
  applyPalette(tray);

  tray.refresh();
  auto initial = tray.icon_.toolTip();

  // minor change (<5%), stays above thresholds
  probe->s.mem_available_kib = *s.mem_available_kib - *s.mem_available_kib * 3 / 100;
  probe->s.some.avg10 = 0.103;
  tray.refresh();
  CHECK(tray.icon_.toolTip() == initial);

  // major change (>5%), still above warn threshold
  probe->s.mem_available_kib = *s.mem_available_kib * 8 / 10;
  probe->s.some.avg10 = 0.2;
  tray.refresh();
  CHECK(tray.icon_.toolTip() != initial);
}

TEST_CASE("refresh updates tooltip on threshold crossing") {
  AppConfig cfg;
  ProbeSample s;
  long base = cfg.mem.available_warn_kib + cfg.mem.available_warn_kib / 50; // 2% above warn
  s.mem_available_kib = base;
  s.some.avg10 = 0.0;
  auto *probe = new StubProbe(s);
  Tray tray(nullptr, std::unique_ptr<SystemProbe>(probe));
  applyPalette(tray);

  tray.refresh();
  auto tipAbove = tray.icon_.toolTip();

  // small change (<5%) but crosses warn threshold
  probe->s.mem_available_kib = cfg.mem.available_warn_kib - cfg.mem.available_warn_kib / 50;
  tray.refresh();
  CHECK(tray.icon_.toolTip() != tipAbove);
}
