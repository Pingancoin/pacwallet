# Pingancoin Qt Wallet

This directory contains the native `C++/Qt` desktop front end for Pingancoin
Wallet.

## Architecture

- desktop UI: Qt Widgets
- local wallet core: bundled `pacwallet serve`
- chain backend: configured remote `pacd` RPC endpoint

The Qt app is the primary desktop-wallet interface. It starts the local wallet
service automatically, talks to it over `127.0.0.1`, and stops it when the app
quits. The local service stores wallet data and forwards chain operations to the
configured remote RPC endpoint.

## Current macOS Release Scope

- first-run create and restore flow
- overview with wallet state, balance, and node health
- receive screen with address list, QR loading, copy helpers, and QR export
- send form with spendable balance display and confirmation prompt
- transaction list with filters, search, and detail inspector
- multisig preview with local signer export and result export
- settings for language, upstream endpoint, backup, restore, encryption,
  passphrase change, private-key import, and wallet path shortcuts

## Build

Requirements:

- Qt 6 Widgets
- Qt 6 Network
- CMake
- C++17 compiler
- Go 1.25 or newer

Development build:

```bash
qt-cmake -S qt -B qt/build
cmake --build qt/build -j2
```

macOS release build from the repository root:

```bash
VERSION=1.0.0 ./scripts/build-macos-qt-release.sh
```

The release script produces a bundled `pacwallet-qt.app` and a zip archive.

## Runtime Notes

The app uses the local wallet service API, including:

- `GET /api/overview`
- `GET /api/tx/<txid>`
- `POST /api/wallet/create`
- `POST /api/wallet/restore`
- `POST /api/addresses`
- `POST /api/send`
- `POST /api/multisig/preview`
- `GET /receive/qr/<address>`

The Qt app should not require users to start a separate wallet backend manually.
