#!/usr/bin/env bash
# Installs dependencies, builds, and installs nohang-tr.
set -euo pipefail

sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

echo "[install] installing dependencies"
sudo pacman -Syu --noconfirm
if [ -f pacman-packages.txt ]; then
  xargs -a pacman-packages.txt -r sudo pacman -S --needed --noconfirm
fi

if ! command -v nohang >/dev/null; then
  echo "[install] nohang not found. Please install it from the AUR: https://aur.archlinux.org/packages/nohang"
fi

echo "[install] configuring"
cmake -S . -B build -G Ninja

echo "[install] building"
cmake --build build
echo "[install] installing"
prefix=${1:-/usr/local}
sudo cmake --install build --prefix "$prefix"
echo "[install] done"
