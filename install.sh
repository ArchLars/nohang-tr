#!/usr/bin/env bash
# Installs dependencies and builds nohang-tr.
set -euo pipefail

sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

echo "[install] installing dependencies"
sudo pacman -Syu --noconfirm
if [ -f apt-packages.txt ]; then
  xargs -a apt-packages.txt -r sudo pacman -S --needed --noconfirm
fi

echo "[install] configuring"
cmake -S . -B build -G Ninja

echo "[install] building"
cmake --build build

echo "[install] done"
