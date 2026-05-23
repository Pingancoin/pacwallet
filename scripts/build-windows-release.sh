#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-$ROOT/dist/pacwallet-windows-amd64}"
VERSION="${VERSION:-0.1.0-dev}"
COMMIT="${COMMIT:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)}"
BUILD_TIME="${BUILD_TIME:-$(date -u +"%Y-%m-%dT%H:%M:%SZ")}"
ARCHIVE_PATH="${ARCHIVE_PATH:-${OUT_DIR}.zip}"
DEFAULT_RPC_PRIMARY="${DEFAULT_RPC_PRIMARY:-https://rpc.pingancoin.org/rpc}"
DEFAULT_RPC_SECONDARY="${DEFAULT_RPC_SECONDARY:-http://180.184.43.187/rpc}"

LDFLAGS="-X github.com/Pingancoin/pacwallet/internal/buildinfo.Version=${VERSION} -X github.com/Pingancoin/pacwallet/internal/buildinfo.Commit=${COMMIT} -X github.com/Pingancoin/pacwallet/internal/buildinfo.BuildTime=${BUILD_TIME}"

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "building Windows release into $OUT_DIR"
echo "version: $VERSION"
echo "commit: $COMMIT"
echo "build_time: $BUILD_TIME"

pushd "$ROOT" >/dev/null
GOOS=windows GOARCH=amd64 go build -ldflags "$LDFLAGS" -o "$OUT_DIR/pacwallet.exe" ./cmd/pacwallet
GOOS=windows GOARCH=amd64 go build -ldflags "$LDFLAGS" -o "$OUT_DIR/pacwallet-desktop.exe" ./cmd/pacwallet-desktop
popd >/dev/null

cp "$ROOT/README.md" "$OUT_DIR/README.md"
mkdir -p "$OUT_DIR/branding" "$OUT_DIR/packaging/windows"
cp "$ROOT/assets/branding/pingancoin/"* "$OUT_DIR/branding/"
cp "$ROOT/packaging/windows/pacwallet-installer.iss" "$OUT_DIR/packaging/windows/pacwallet-installer.iss"
cp "$ROOT/scripts/build-windows-installer.sh" "$OUT_DIR/build-windows-installer.sh"
cp "$ROOT/scripts/sign-windows-release.sh" "$OUT_DIR/sign-windows-release.sh"

cat >"$OUT_DIR/build-installer.bat" <<'EOF'
@echo off
setlocal
set RELEASE_DIR=%~dp0
set VERSION=%VERSION%
if "%VERSION%"=="" set VERSION=0.1.0-dev

if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
  "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" /DMyAppVersion=%VERSION% /DSourceReleaseDir="%RELEASE_DIR:~0,-1%" "%~dp0packaging\windows\pacwallet-installer.iss"
  exit /b %ERRORLEVEL%
)

if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
  "%ProgramFiles%\Inno Setup 6\ISCC.exe" /DMyAppVersion=%VERSION% /DSourceReleaseDir="%RELEASE_DIR:~0,-1%" "%~dp0packaging\windows\pacwallet-installer.iss"
  exit /b %ERRORLEVEL%
)

echo Inno Setup 6 not found. Install it, then rerun this script.
exit /b 1
EOF

cat >"$OUT_DIR/sign-release.bat" <<'EOF'
@echo off
setlocal
if "%SIGN_PFX_PATH%"=="" (
  echo Set SIGN_PFX_PATH to your code signing .pfx file.
  exit /b 1
)
if "%SIGN_PFX_PASSWORD%"=="" (
  echo Set SIGN_PFX_PASSWORD to your code signing password.
  exit /b 1
)

for %%F in ("%~dp0*.exe" "%~dp0*.msi") do (
  if exist "%%~fF" (
    signtool sign /f "%SIGN_PFX_PATH%" /p "%SIGN_PFX_PASSWORD%" /tr http://timestamp.digicert.com /td sha256 /fd sha256 "%%~fF"
  )
)
EOF

cat >"$OUT_DIR/pacwallet-desktop.json" <<'EOF'
{
  "network": "mainnet",
  "wallet_dir": "",
  "rpc_url": "__DEFAULT_RPC_PRIMARY__",
  "listen": "127.0.0.1:19709",
  "browser": "edge",
  "title": "Pingancoin Wallet",
  "upstreams_template": "upstreams.mainnet.template.json"
}
EOF
python3 - <<PY
from pathlib import Path
p = Path("$OUT_DIR/pacwallet-desktop.json")
p.write_text(p.read_text().replace("__DEFAULT_RPC_PRIMARY__", "$DEFAULT_RPC_PRIMARY"))
PY

cat >"$OUT_DIR/upstreams.mainnet.template.json" <<'EOF'
{
  "active_id": "server1-rpc",
  "profiles": [
    {
      "id": "server1-rpc",
      "name": "Server 1 RPC",
      "url": "__DEFAULT_RPC_PRIMARY__",
      "source": "official"
    },
    {
      "id": "server2-rpc",
      "name": "Server 2 RPC",
      "url": "__DEFAULT_RPC_SECONDARY__",
      "source": "official"
    }
  ]
}
EOF
python3 - <<PY
from pathlib import Path
p = Path("$OUT_DIR/upstreams.mainnet.template.json")
text = p.read_text()
text = text.replace("__DEFAULT_RPC_PRIMARY__", "$DEFAULT_RPC_PRIMARY")
text = text.replace("__DEFAULT_RPC_SECONDARY__", "$DEFAULT_RPC_SECONDARY")
p.write_text(text)
PY

cat >"$OUT_DIR/release.json" <<EOF
{
  "product": "Pingancoin Wallet",
  "version": "${VERSION}",
  "commit": "${COMMIT}",
  "build_time": "${BUILD_TIME}",
  "platform": "windows-amd64",
  "artifacts": [
    "pacwallet.exe",
    "pacwallet-desktop.exe",
    "pacwallet-desktop.json",
    "upstreams.mainnet.template.json",
    "branding/",
    "packaging/windows/pacwallet-installer.iss",
    "build-windows-installer.sh",
    "sign-windows-release.sh",
    "build-installer.bat",
    "sign-release.bat",
    "run-pacwallet-desktop.bat",
    "run-pacwallet-web.bat",
    "WINDOWS_RELEASE_NOTES.txt",
    "README.md"
  ]
}
EOF

cat >"$OUT_DIR/run-pacwallet-desktop.bat" <<'EOF'
@echo off
setlocal
if exist "%~dp0pacwallet-desktop.json" (
  start "" "%~dp0pacwallet-desktop.exe" --config "%~dp0pacwallet-desktop.json"
) else (
  start "" "%~dp0pacwallet-desktop.exe" --network mainnet --rpc __DEFAULT_RPC_PRIMARY__ --browser edge
)
endlocal
EOF
python3 - <<PY
from pathlib import Path
p = Path("$OUT_DIR/run-pacwallet-desktop.bat")
p.write_text(p.read_text().replace("__DEFAULT_RPC_PRIMARY__", "$DEFAULT_RPC_PRIMARY"))
PY

cat >"$OUT_DIR/run-pacwallet-web.bat" <<'EOF'
@echo off
setlocal
set PAC_RPC_URL=%PAC_RPC_URL%
if "%PAC_RPC_URL%"=="" set PAC_RPC_URL=__DEFAULT_RPC_PRIMARY__

set PAC_NETWORK=%PAC_NETWORK%
if "%PAC_NETWORK%"=="" set PAC_NETWORK=mainnet

set PAC_LISTEN=%PAC_LISTEN%
if "%PAC_LISTEN%"=="" set PAC_LISTEN=127.0.0.1:19709

"%~dp0pacwallet.exe" serve --network %PAC_NETWORK% --rpc %PAC_RPC_URL% --listen %PAC_LISTEN%
endlocal
EOF
python3 - <<PY
from pathlib import Path
p = Path("$OUT_DIR/run-pacwallet-web.bat")
p.write_text(p.read_text().replace("__DEFAULT_RPC_PRIMARY__", "$DEFAULT_RPC_PRIMARY"))
PY

cat >"$OUT_DIR/WINDOWS_RELEASE_NOTES.txt" <<'EOF'
Pingancoin Wallet Windows Release

Files:
- pacwallet.exe: CLI and web wallet service
- pacwallet-desktop.exe: desktop launcher that opens the wallet in an Edge or Chrome app window
- pacwallet-desktop.json: default desktop startup config
- upstreams.mainnet.template.json: template official RPC endpoint profiles
- release.json: machine-readable version manifest
- branding\*: app icons and website icon assets
- packaging\windows\pacwallet-installer.iss: installer template for Inno Setup
- build-windows-installer.sh: helper for compiling the installer on a Windows build box
- sign-windows-release.sh: helper for code signing release binaries later
- build-installer.bat: Windows-native installer build helper
- sign-release.bat: Windows-native signing helper for exe and installer files
- run-pacwallet-desktop.bat: convenience launcher for desktop mode
- run-pacwallet-web.bat: convenience launcher for browser-hosted mode

Recommended setup:
1. Double-click run-pacwallet-desktop.bat.
2. On first run, create or restore wallet.json.
3. The wallet file is stored locally under %USERPROFILE%\.pacwallet\mainnet\wallet.json.
4. The desktop launcher talks to the official node RPC upstream configured in pacwallet-desktop.json.
5. Run build-installer.bat on a Windows packaging machine with Inno Setup 6 installed.
6. Sign the exe and installer with your code-signing certificate.

Environment overrides:
- PAC_RPC_URL=__DEFAULT_RPC_PRIMARY__
- PAC_NETWORK=mainnet
- PAC_LISTEN=127.0.0.1:19709
EOF
python3 - <<PY
from pathlib import Path
p = Path("$OUT_DIR/WINDOWS_RELEASE_NOTES.txt")
p.write_text(p.read_text().replace("__DEFAULT_RPC_PRIMARY__", "$DEFAULT_RPC_PRIMARY"))
PY

if command -v ditto >/dev/null 2>&1; then
  rm -f "$ARCHIVE_PATH"
  ditto -c -k --sequesterRsrc --keepParent "$OUT_DIR" "$ARCHIVE_PATH"
fi

echo "release contents:"
ls -lh "$OUT_DIR"
if [[ -f "$ARCHIVE_PATH" ]]; then
  echo "release archive:"
  ls -lh "$ARCHIVE_PATH"
fi
