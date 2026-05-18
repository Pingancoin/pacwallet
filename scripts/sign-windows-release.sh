#!/usr/bin/env bash
set -euo pipefail

RELEASE_DIR="${1:?usage: sign-windows-release.sh <release-dir>}"
TIMESTAMP_URL="${TIMESTAMP_URL:-http://timestamp.digicert.com}"

if [[ ! -d "$RELEASE_DIR" ]]; then
  echo "release directory not found: $RELEASE_DIR" >&2
  exit 1
fi

mapfile -t TARGETS < <(find "$RELEASE_DIR" -maxdepth 1 -type f \( -name '*.exe' -o -name '*.msi' \) | sort)
if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "no signable targets found in $RELEASE_DIR" >&2
  exit 1
fi

if command -v osslsigncode >/dev/null 2>&1; then
  : "${SIGN_PFX_PATH:?set SIGN_PFX_PATH to your code signing .pfx file}"
  : "${SIGN_PFX_PASSWORD:?set SIGN_PFX_PASSWORD to the .pfx password}"
  for target in "${TARGETS[@]}"; do
    signed="${target%.exe}-signed.exe"
    signed="${signed%.msi}-signed.msi"
    osslsigncode sign \
      -pkcs12 "$SIGN_PFX_PATH" \
      -pass "$SIGN_PFX_PASSWORD" \
      -n "Pingancoin Wallet" \
      -t "$TIMESTAMP_URL" \
      -in "$target" \
      -out "$signed"
    mv "$signed" "$target"
    echo "signed $target"
  done
  exit 0
fi

cat <<EOF
No signing tool found on this machine.

You can sign the release on a Windows signer with signtool.exe or on Unix with osslsigncode.

Expected release targets:
$(printf '  %s\n' "${TARGETS[@]}")

If you use osslsigncode later:
  SIGN_PFX_PATH=/path/to/codesign.pfx \\
  SIGN_PFX_PASSWORD=your-password \\
  ./scripts/sign-windows-release.sh "$RELEASE_DIR"
EOF
