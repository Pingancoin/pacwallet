# Pingancoin Native Wallet (`pacwallet-qt`)

This directory contains the native `C++/Qt` desktop wallet front end for Pingancoin.

The architecture follows:

- native desktop UI: `Qt Widgets`
- wallet backend: existing `pacwallet serve`
- chain/node backend: existing `pacd` RPC

## Goals

This is the path for a real native desktop wallet rather than a browser-hosted shell.
It is meant to feel closer to a traditional cryptocurrency desktop client while still
reusing the existing Go wallet core and RPC surface.

## Current Scope

The initial Qt app includes:

- native main window and sidebar navigation
- overview dashboard
- first-run create / restore flow
- overview dashboard with wallet state and UTXO inventory
- receive screen with address list, QR loading, copy helpers, and QR export
- send form with spendable balance display, max helper, change-address selection, and native confirmation prompt
- transaction list with filters, search, and detail inspector
- multisig preview screen with local signer export and result export
- settings for backend URL, local service launch, upstream switching, encryption, passphrase changes, private-key import, backups, and wallet path shortcuts

## Build

You need a local Qt 6 toolchain with:

- `Qt6 Widgets`
- `Qt6 Network`
- `cmake`
- a C++17 compiler

Example build:

```bash
qt-cmake -S qt -B qt/build
cmake --build qt/build -j2
```

## Runtime Model

The app talks to the existing wallet backend endpoints:

- `GET /api/overview`
- `GET /api/tx/<txid>`
- `POST /api/addresses`
- `POST /api/send`
- `POST /api/multisig/preview`
- `GET /receive/qr/<address>`

It can also launch a local `pacwallet serve` process through `QProcess` when configured.

## macOS Native Release

On macOS with Qt installed:

```bash
VERSION=0.3.0-rc1 ./scripts/build-macos-qt-release.sh
```

That produces:

- a self-contained `pacwallet-qt.app`
- bundled Qt frameworks via `macdeployqt`
- `release.json`
- a zip archive under `dist/`

## Windows Native Release

On a Windows build machine:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-windows-qt-toolchain.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows-qt-release.ps1
```

That produces:

- a deployable `pacwallet-qt.exe`
- Qt runtime DLLs and plugin folders via `windeployqt`
- a zipped Windows release directory under `dist\`
