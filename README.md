# nohang-tr

A DE-agnostic system tray for `nohang`, showing live memory pressure and when actions may trigger. Qt 6, CMake, Ninja. TDD with Catch2.

## Dependencies

- Linux with PSI (`/proc/pressure`) available
- `nohang` installed
- Qt 6 Widgets, CMake, Ninja
- libayatana-appindicator3 (for GNOME compatibility)
- Catch2 for tests
- gcovr for coverage reports

Install on Ubuntu CI (see apt-packages.txt). On GNOME, AppIndicator support may be required. 

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build/tests

# With coverage
cmake -S . -B build -G Ninja -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build/tests
gcovr -r . --exclude build -e src/main.cpp
```

## RUN
```bash
./build/nohang-tr
```

Config

See config/nohang-tr.example.toml. It defines thresholds and colors that the tray uses to compute states against current /proc values. PSI and /proc/meminfo are read to estimate risk and show a color shield.
