# nohang-tr

A cross-desktop system tray companion for `nohang` that visualizes memory
pressure. The tray icon changes color based on Linux PSI and
`/proc/meminfo` readings so you can see when `nohang` actions may trigger.
Built with Qt 6, CMake and Ninja; tests use Catch2.

## Features

- Live tray indicator of memory pressure
- Color palette reflects warning and critical thresholds
- Tooltip displays current readings alongside configured targets

## Dependencies

- Linux with PSI (`/proc/pressure`) available
- `nohang` for configuration parsing (install separately from the AUR)
- Qt 6 Widgets, CMake, Ninja
- Vulkan headers (e.g., `libvulkan-dev`)
- Catch2 for tests (optional; tests are skipped if absent)
- gcovr for coverage reports

See `apt-packages.txt` for packages installed on Ubuntu CI.

## Installation and updates

Run `./install.sh` to install dependencies and build the project:

The provided scripts assume an Arch Linux environment and use `pacman` for
package management without upgrading the whole system. They check whether
`nohang` is already installed and, if missing, instruct you to obtain it from
the [AUR](https://aur.archlinux.org/packages/nohang). The `apt-packages.txt`
file remains for compatibility with other tooling.

```bash
./install.sh
```

Fetch the latest changes and rebuild with:

```bash
./update.sh
```


## Building and testing

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build/tests
```

To collect coverage data:

```bash
cmake -S . -B build -G Ninja -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build/tests
gcovr -r . --exclude build -e src/main.cpp
```

## Running

```bash
./build/nohang-tr
```

## Configuration

Copy `config/nohang-tr.example.toml` to your configuration directory and
adjust thresholds and palette as needed:

```bash
cp config/nohang-tr.example.toml ~/.config/nohang-tr/nohang-tr.toml
```

`XDG_CONFIG_HOME` is respected when set; otherwise `$HOME/.config` is used.
The file defines PSI and available memory thresholds and the icons used for
each state.

