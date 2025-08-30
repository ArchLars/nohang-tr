#!/usr/bin/env bash
# Installs dependencies and builds nohang-tr.
set -euo pipefail

sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

echo "[install] installing dependencies"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update -y -o Acquire::Retries=3
if [ -f apt-packages.txt ]; then
  xargs -a apt-packages.txt -r sudo apt-get install -y --no-install-recommends
fi

echo "[install] configuring"
cmake -S . -B build -G Ninja

echo "[install] building"
cmake --build build

echo "[install] done"
