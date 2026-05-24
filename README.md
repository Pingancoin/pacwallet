# Pingancoin Wallet

Pingancoin Wallet is the official desktop wallet for Pingancoin / PAC.

Current public release:

- version: `v1.0.1`
- platform: macOS
- application: native `C++/Qt` desktop wallet
- wallet core: local `pacwallet` service bundled inside the app
- chain backend: remote `pacd` RPC endpoint, defaulting to `https://rpc.pingancoin.org/rpc`

This release is the first clean macOS desktop build. Older experimental
interfaces, launchers, and release-candidate notes have been removed from this
repository documentation so the README reflects the current shipping wallet.

## Download

Download the latest macOS build from:

- https://github.com/Pingancoin/pacwallet/releases/tag/v1.0.1

The macOS asset is:

- `pacwallet-macos-v1.0.1.zip`

The release also includes a SHA256 checksum file.

## What The Wallet Does

- create a new local PAC wallet
- restore a wallet from a wallet file
- show balance, wallet status, and node status
- generate receive addresses and QR codes
- send PAC transactions through the configured RPC backend
- serve a local-only batch payment API for pool hot-wallet payouts
- show transaction history and transaction details
- manage backups, passphrase changes, private-key import, and upstream settings
- preview and export 3-of-5 multisig payout data

The desktop app starts the bundled wallet service automatically when it opens and
stops it when the app closes. It does not run a local `pacd` node or download the
blockchain. Chain data is read from the configured remote node RPC service.

For pool operations, `pacwallet serve` exposes `POST /api/sendmany` for local
batch payouts. Keep this service bound to `127.0.0.1` and set
`PACWALLET_API_TOKEN` when another local service, such as `pacpool`, calls it.

## Wallet Files

On macOS mainnet, the wallet data directory is:

```text
/Users/<your-user>/.pacwallet/mainnet/
```

The main wallet file is:

```text
wallet.json
```

Automatic overwrite backups are stored under:

```text
wallet-backups/
```

Keep `wallet.json` and every backup private. Anyone with the wallet file and the
correct passphrase may be able to spend the coins.

## Source Layout

- `cmd/pacwallet` - Go wallet command and local wallet service
- `internal/` - wallet storage, API, chain RPC, transaction, and key logic
- `qt/` - native Qt desktop wallet
- `assets/branding/pingancoin/` - app and website icon assets
- `scripts/build-macos-qt-release.sh` - macOS release builder

## Build macOS Release

Requirements:

- Go 1.25 or newer
- Qt 6 with Qt Widgets and Qt Network
- CMake
- a C++17 compiler

Build:

```bash
VERSION=1.0.0 ./scripts/build-macos-qt-release.sh
```

The script builds the Go wallet service, builds the Qt app, bundles Qt
frameworks with `macdeployqt`, and creates a zipped macOS app release.

## Development Checks

Run focused Go tests from the repository root:

```bash
go test ./cmd/... ./internal/...
```

Do not use `go test ./...` while Qt build directories exist under `qt/`, because
CMake-generated folders are not Go packages.

## Security Notes

- The macOS app is currently unsigned.
- Only download release builds from the official Pingancoin GitHub release page.
- Back up `wallet.json` before moving, replacing, or restoring wallet data.
- The default wallet mode uses a remote chain RPC endpoint, so endpoint
  availability affects balance and transaction display.

## Legal Notice

Pingancoin Wallet and related Pingancoin software are provided for technical
research, protocol experimentation, and open-source software development only.
This project does not provide investment advice, financial advice, trading
advice, or any promise of token value, liquidity, exchange listing, or future
profit.

The project maintainer does not conduct exchange-listing activities on behalf of
users and does not authorize anyone to market PAC as an investment product.
Anyone who downloads, runs, mines, transfers, or otherwise uses this software is
responsible for understanding and complying with the laws and regulations that
apply in their own jurisdiction. Users act at their own risk.

## Current Scope

This repository currently publishes the macOS wallet as the first official
desktop release. Other platform packages and code signing will be handled in
later releases after the macOS wallet line is stable.
