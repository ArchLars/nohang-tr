#!/usr/bin/env bash
# Pulls latest changes and rebuilds modified targets.
set -euo pipefail

echo "[update] pulling latest changes"
git pull --ff-only

echo "[update] configuring"
cmake -S . -B build -G Ninja

echo "[update] building"
cmake --build build

echo "[update] done"
