#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RELEASE_DIR="${1:-$ROOT/dist/pacwallet-windows-amd64}"
VERSION="${VERSION:-0.1.0-dev}"
ISS="$ROOT/packaging/windows/pacwallet-installer.iss"
ISCC_BIN="${ISCC_BIN:-iscc}"

if [[ ! -d "$RELEASE_DIR" ]]; then
  echo "release directory not found: $RELEASE_DIR" >&2
  exit 1
fi

if command -v "$ISCC_BIN" >/dev/null 2>&1; then
  "$ISCC_BIN" "//DMyAppVersion=$VERSION" "//DSourceReleaseDir=$RELEASE_DIR" "$ISS"
  exit 0
fi

cat <<EOF
Inno Setup compiler not found.

Installer script is ready at:
  $ISS

On a Windows build machine, compile it with:
  iscc /DMyAppVersion=$VERSION /DSourceReleaseDir="$RELEASE_DIR" "$ISS"

If Inno Setup is not on PATH, set:
  ISCC_BIN="C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe"
  ./scripts/build-windows-installer.sh "$RELEASE_DIR"
EOF
