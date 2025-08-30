#!/usr/bin/env bash
# Removes installed nohang-tr files.
set -euo pipefail

sudo() { command sudo -n "$@" 2>/dev/null || "$@"; }

prefix=${1:-/usr/local}

echo "[uninstall] removing binary"
sudo rm -f "$prefix/bin/nohang-tr"

echo "[uninstall] removing desktop entry"
sudo rm -f "$prefix/share/applications/nohang-tr.desktop"

echo "[uninstall] removing icons"
for icon in \
  shield-green.svg \
  shield-yellow.svg \
  shield-orange.svg \
  shield-red.svg \
  shield-black.svg
 do
  sudo rm -f "$prefix/share/icons/hicolor/scalable/apps/$icon"
 done

echo "[uninstall] removing example config"
sudo rm -f "$prefix/share/nohang-tr/nohang-tr.example.toml"
# Attempt to remove directory if empty
sudo rmdir "$prefix/share/nohang-tr" 2>/dev/null || true

echo "[uninstall] done"
