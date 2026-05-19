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
- receive screen with address list and QR loading
- send form
- transaction list and detail inspector
- multisig preview screen
- settings for backend URL and local service launch parameters

## Build

You need a local Qt 6 toolchain with:

- `Qt6 Widgets`
- `Qt6 Network`
- `cmake`
- a C++17 compiler

Example build:

```bash
cmake -S qt -B qt/build
cmake --build qt/build
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
