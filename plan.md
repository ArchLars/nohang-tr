# Plan

Goal: tray app that visualizes memory pressure and predicted `nohang` actions using simple color states and a tooltip that explains why.

Components:
- `system_probe` reads `/proc/meminfo` and `/proc/pressure/memory`. It returns a struct with MemAvailable KiB, PSI avg10 and avg60, and simple derived metrics.
- `config` loads TOML with thresholds like `psi.avg10_warn`, `mem.available_crit_kib`, and a `palette` mapping states to icons.
- `tray` owns QSystemTrayIcon, picks an icon color based on current sample and the config, updates tooltip with current numbers and predicted trigger windows. It does not depend on a desktop environment. For GNOME, libayatana-appindicator is linked to allow bridge compatibility. Qt falls back to the native tray where present. :contentReference[oaicite:6]{index=6}

Update loop:
- QTimer every 2 seconds reads `system_probe`.
- `tray` computes state: green, yellow, orange, red. State machine has hysteresis to avoid flicker. For now, stub uses single threshold comparisons.

TDD:
- Unit tests cover config parsing and state computation on synthetic inputs.
- CI runs tests only on merged PRs to `main`, builds with Ninja, caches APT packages for speed. :contentReference[oaicite:7]{index=7}

Phase 1 – Basic Function:
- Run `unminimize` on minimal systems to ensure man pages and documentation are present.
- Integrate `nohang` configuration parsing to align tray behavior with system settings.
- Sample memory pressure via `system_probe` to feed data into the tray.
- Display a basic tray icon with color states and a tooltip.
- Add initial tests covering probe sampling, config integration, and UI state.

Future:
- Parse `nohang` config for actual trigger points and show them.
- Popup details window with rolling PSI graph.
- User overrides through a settings dialog.
