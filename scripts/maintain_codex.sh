#!/usr/bin/env bash
# Runs when the cached Codex container is resumed.
# Keep it quick. Update indices and ensure deps are still installed.
set -euo pipefail

echo "[maintain] starting"
export DEBIAN_FRONTEND=noninteractive
sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

# Refresh apt metadata in case the cache is old
sudo apt-get update -y -o Acquire::Retries=3

# Reconcile any missing packages, same list as setup
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

missing=()
for p in "${APT_PKGS[@]}"; do
  if ! dpkg -s "$p" >/dev/null 2>&1; then
    missing+=("$p")
  fi
done

if (( ${#missing[@]} )); then
  echo "[maintain] installing: ${missing[*]}"
  sudo apt-get install -y --no-install-recommends "${missing[@]}"
else
  echo "[maintain] all packages present"
fi

# Touch the build directory so the agent can immediately build and test
if [ -d build ]; then
  echo "[maintain] re-configure build"
  cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache || true
fi

ccache -s || true
echo "[maintain] done"
