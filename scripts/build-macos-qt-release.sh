#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-$ROOT/dist/pacwallet-macos-qt}"
BUILD_DIR="${BUILD_DIR:-$ROOT/qt/build-release}"
VERSION="${VERSION:-0.2.0-dev}"
COMMIT="${COMMIT:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)}"
BUILD_TIME="${BUILD_TIME:-$(date -u +"%Y-%m-%dT%H:%M:%SZ")}"
ARCHIVE_PATH="${ARCHIVE_PATH:-${OUT_DIR}.zip}"
QT_CMAKE="${QT_CMAKE:-qt-cmake}"
MACDEPLOYQT="${MACDEPLOYQT:-macdeployqt}"

rm -rf "$OUT_DIR" "$BUILD_DIR"
mkdir -p "$OUT_DIR"

echo "building macOS Qt release into $OUT_DIR"
echo "version: $VERSION"
echo "commit: $COMMIT"
echo "build_time: $BUILD_TIME"

pushd "$ROOT" >/dev/null
"$QT_CMAKE" -S qt -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j2
popd >/dev/null

APP_BUNDLE="$BUILD_DIR/pacwallet-qt.app"
if [[ ! -d "$APP_BUNDLE" ]]; then
  echo "missing app bundle: $APP_BUNDLE" >&2
  exit 1
fi

cp -R "$APP_BUNDLE" "$OUT_DIR/"
GOOS=darwin GOARCH=arm64 go build -ldflags "-X github.com/Pingancoin/pacwallet/internal/buildinfo.Version=${VERSION} -X github.com/Pingancoin/pacwallet/internal/buildinfo.Commit=${COMMIT} -X github.com/Pingancoin/pacwallet/internal/buildinfo.BuildTime=${BUILD_TIME}" -o "$OUT_DIR/pacwallet-qt.app/Contents/MacOS/pacwallet" ./cmd/pacwallet
"$MACDEPLOYQT" "$OUT_DIR/pacwallet-qt.app" -verbose=1

cp "$ROOT/README.md" "$OUT_DIR/README.md"
mkdir -p "$OUT_DIR/branding"
cp "$ROOT/assets/branding/pingancoin/"* "$OUT_DIR/branding/"

cat >"$OUT_DIR/release.json" <<EOF
{
  "product": "Pingancoin Wallet Qt",
  "version": "${VERSION}",
  "commit": "${COMMIT}",
  "build_time": "${BUILD_TIME}",
  "platform": "macos-native-qt",
  "artifacts": [
    "pacwallet-qt.app",
    "pacwallet-qt.app/Contents/MacOS/pacwallet",
    "branding/",
    "README.md"
  ]
}
EOF

rm -f "$ARCHIVE_PATH"
ditto -c -k --sequesterRsrc --keepParent "$OUT_DIR" "$ARCHIVE_PATH"

echo "release contents:"
ls -lh "$OUT_DIR"
echo "release archive:"
ls -lh "$ARCHIVE_PATH"
