#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RELEASE_DIR="${1:-$ROOT/dist/pacwallet-windows-amd64}"
VERSION="${VERSION:-0.1.0-dev}"
ISS="$ROOT/packaging/windows/pacwallet-installer.iss"

if [[ ! -d "$RELEASE_DIR" ]]; then
  echo "release directory not found: $RELEASE_DIR" >&2
  exit 1
fi

if command -v iscc >/dev/null 2>&1; then
  iscc "//DMyAppVersion=$VERSION" "//DSourceReleaseDir=$RELEASE_DIR" "$ISS"
  exit 0
fi

cat <<EOF
Inno Setup compiler not found.

Installer script is ready at:
  $ISS

On a Windows build machine, compile it with:
  iscc /DMyAppVersion=$VERSION /DSourceReleaseDir="$RELEASE_DIR" "$ISS"
EOF
