#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-$ROOT/dist/pacwallet-windows-amd64}"

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "building Windows release into $OUT_DIR"

pushd "$ROOT" >/dev/null
GOOS=windows GOARCH=amd64 go build -o "$OUT_DIR/pacwallet.exe" ./cmd/pacwallet
GOOS=windows GOARCH=amd64 go build -o "$OUT_DIR/pacwallet-desktop.exe" ./cmd/pacwallet-desktop
popd >/dev/null

cp "$ROOT/README.md" "$OUT_DIR/README.md"

cat >"$OUT_DIR/run-pacwallet-desktop.bat" <<'EOF'
@echo off
setlocal
set PAC_RPC_URL=%PAC_RPC_URL%
if "%PAC_RPC_URL%"=="" set PAC_RPC_URL=http://127.0.0.1:9509

set PAC_NETWORK=%PAC_NETWORK%
if "%PAC_NETWORK%"=="" set PAC_NETWORK=mainnet

start "" "%~dp0pacwallet-desktop.exe" --network %PAC_NETWORK% --rpc %PAC_RPC_URL% --browser edge
endlocal
EOF

cat >"$OUT_DIR/run-pacwallet-web.bat" <<'EOF'
@echo off
setlocal
set PAC_RPC_URL=%PAC_RPC_URL%
if "%PAC_RPC_URL%"=="" set PAC_RPC_URL=http://127.0.0.1:9509

set PAC_NETWORK=%PAC_NETWORK%
if "%PAC_NETWORK%"=="" set PAC_NETWORK=mainnet

set PAC_LISTEN=%PAC_LISTEN%
if "%PAC_LISTEN%"=="" set PAC_LISTEN=127.0.0.1:19709

"%~dp0pacwallet.exe" serve --network %PAC_NETWORK% --rpc %PAC_RPC_URL% --listen %PAC_LISTEN%
endlocal
EOF

cat >"$OUT_DIR/WINDOWS_RELEASE_NOTES.txt" <<'EOF'
Pingancoin Wallet Windows Release

Files:
- pacwallet.exe: CLI and web wallet service
- pacwallet-desktop.exe: desktop launcher that opens the wallet in an Edge or Chrome app window
- run-pacwallet-desktop.bat: convenience launcher for desktop mode
- run-pacwallet-web.bat: convenience launcher for browser-hosted mode

Recommended setup:
1. Start pacd with RPC enabled.
2. Double-click run-pacwallet-desktop.bat.
3. On first run, create or restore wallet.json.

Environment overrides:
- PAC_RPC_URL=http://127.0.0.1:9509
- PAC_NETWORK=mainnet
- PAC_LISTEN=127.0.0.1:19709
EOF

echo "release contents:"
ls -lh "$OUT_DIR"
