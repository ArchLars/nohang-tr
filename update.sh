#!/usr/bin/env bash
# Pulls latest changes and rebuilds modified targets.
set -euo pipefail

sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

echo "[update] updating system packages"
sudo pacman -Syu --noconfirm
if [ -f pacman-packages.txt ]; then
  xargs -a pacman-packages.txt -r sudo pacman -S --needed --noconfirm
fi

echo "[update] pulling latest changes"
git pull --ff-only

echo "[update] configuring"
cmake -S . -B build -G Ninja

echo "[update] building"
cmake --build build

echo "[update] done"
