# nohang-tr

A DE-agnostic system tray for `nohang`, showing live memory pressure and when actions may trigger. Qt 6, CMake, Ninja. TDD with Catch2.

## Dependencies

- Linux with PSI (`/proc/pressure`) available
- `nohang` installed and running (optional for dev)
- Qt 6 Widgets, CMake, Ninja
- libayatana-appindicator3 (for GNOME compatibility)
- Catch2 for tests

Install on Ubuntu CI (see apt-packages.txt). On GNOME, AppIndicator support may be required. :contentReference[oaicite:4]{index=4}

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
```

## RUN
```bash
./build/nohang-tr
```
