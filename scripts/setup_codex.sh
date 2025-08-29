#!/usr/bin/env bash
# Sets up the Codex CI container so it can build and test nohang-tr.
# Runs before the container is cached.
set -euo pipefail

echo "[setup] starting"

# Codex uses a Debian or Ubuntu base inside its universal image.
# Install only what we need for C++ Qt tray builds and tests.
# Keep it idempotent and fast.
APT_PKGS=(
  build-essential
  cmake
  ninja-build
  pkg-config
  qt6-base-dev
  libayatana-appindicator3-dev
  libglib2.0-dev
  libdbus-1-dev
  catch2
  ccache
  git
)

export DEBIAN_FRONTEND=noninteractive
export APT_LISTCHANGES_FRONTEND=none

# Use apt retries, quiet output, and no recommends
sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

echo "[setup] apt update"
sudo apt-get update -y -o Acquire::Retries=3

# Install only missing packages
missing=()
for p in "${APT_PKGS[@]}"; do
  if ! dpkg -s "$p" >/dev/null 2>&1; then
    missing+=("$p")
  fi
done

if (( ${#missing[@]} )); then
  echo "[setup] installing: ${missing[*]}"
  sudo apt-get install -y --no-install-recommends "${missing[@]}"
else
  echo "[setup] all packages present"
fi

# Configure ccache location inside the workspace to benefit from Codex container caching
export CCACHE_DIR="${CCACHE_DIR:-/workspace/.ccache}"
mkdir -p "$CCACHE_DIR"
ccache --max-size=2G || true
echo "export CCACHE_DIR=${CCACHE_DIR}" >> "${HOME}/.bashrc"

# Pre-configure CMake cache so the first agent build is quicker
if [ -f CMakeLists.txt ]; then
  echo "[setup] cmake configure"
  cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache || true
fi

# Helpful env for Qt headless usage in CI-like environments
grep -q 'QT_QPA_PLATFORM' "${HOME}/.bashrc" 2>/dev/null || \
  echo 'export QT_QPA_PLATFORM=offscreen' >> "${HOME}/.bashrc"

# Document what this setup expects so Codex can infer test commands from AGENTS.md
echo "[setup] done"
