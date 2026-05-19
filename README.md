# Pingancoin `pacwallet`

`pacwallet` is the standalone wallet stack for Pingancoin / PAC.

This first split-out repository keeps the wallet self-contained while remaining
compatible with the local `pacd` HTTP RPC. It can create encrypted wallets,
generate receiving addresses, import/export private keys, sign P2PKH
transactions, submit transactions to `pacd`, and track spendable, immature, and
pending balances.

It now includes:
- the original CLI wallet
- a local wallet service with JSON API
- a browser-based UI wallet
- a fuller desktop-wallet surface modeled after the shape of Bitcoin-style local wallets
- a desktop launcher aimed at Windows app-window usage through Edge or Chrome
- a new native `C++/Qt` desktop wallet project under `qt/`
- a native macOS `.app` release path for the Qt wallet
- upstream RPC endpoint profiles with local-first defaults
- a generated branding/icon set under `assets/branding/pingancoin`

## Try It

Use Go 1.25+:

```bash
go test ./...
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet create --network simnet
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet receive --network simnet --label miner-1
go run ./cmd/pacwallet balance --network simnet --rpc http://127.0.0.1:9509
go run ./cmd/pacwallet history --network simnet --rpc http://127.0.0.1:9509
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet send --network simnet --rpc http://127.0.0.1:9509 --to <address> --amount 1.25
go run ./cmd/pacwallet serve --network simnet --rpc http://127.0.0.1:9509 --listen 127.0.0.1:19709
go run ./cmd/pacwallet-desktop --network simnet --rpc http://127.0.0.1:9509 --browser edge
```

The web wallet serves:
- `GET /` for the UI
- `GET /healthz`
- `GET /receive/qr/<address>` for receive QR images
- `GET /tx/<txid>` for transaction detail pages
- `GET /api/overview`
- `GET /api/tx/<txid>`
- `POST /api/wallet/create`
- `POST /api/wallet/encrypt`
- `POST /api/wallet/changepassphrase`
- `POST /api/wallet/restore`
- `POST /api/upstreams`
- `POST /api/upstreams/select`
- `POST /api/addresses`
- `POST /api/keys/import`
- `POST /api/multisig/preview`
- `POST /api/send`

Backup and restore notes:
- `GET /download/wallet` downloads the active `wallet.json`
- `GET /download/pubkeys` exports local label/address/pubkey lines for signer collection
- the UI can restore a `wallet.json` file directly
- restoring over an existing wallet requires overwrite confirmation
- overwrite restores automatically archive the previous wallet into `wallet-backups/`

Node access notes:
- the wallet talks to `pacd` HTTP/RPC endpoints, not raw P2P seed discovery
- default behavior stays local-node first
- the desktop launcher can auto-import `upstreams.<network>.template.json` presets on first launch
- when official RPC servers are deployed, switch the active endpoint in the UI instead of hand-editing the wallet state
- seed nodes are still useful for `pacd` peer discovery, but they should not be treated as the wallet's default backend unless they also expose the RPC service you want to support publicly

The desktop launcher starts the same wallet service and opens it in an app-style
browser window. On Windows, `--browser edge` is the preferred default. For
headless smoke tests or manual launches, use `--browser none`.

Desktop launcher polish:
- `pacwallet-desktop --version` prints build metadata
- `pacwallet-desktop --config <path>` loads a JSON config file
- `pacwallet-desktop --upstreamstemplate <path>` imports endpoint presets before the UI opens
- the desktop home screen now includes:
  - wallet summary and node health
  - receive/address management with visible pubkeys and receive QR codes
  - send form
  - UTXO and transaction history tables
  - transaction detail drill-down pages
  - multisig preview for 3-of-5 collection
  - backup, encryption, and passphrase rotation controls
- history filtering by incoming/outgoing/pending/coinbase plus txid/address search
- local signer export text for multisig coordination
- the Windows release bundle now includes `pacwallet-desktop.json`, `release.json`, and `upstreams.mainnet.template.json`
- the first-run UI now leads with node endpoint selection before wallet create/restore
- the Windows installer keeps app binaries under the user program directory and the desktop config under `%AppData%\Pingancoin Wallet`

## Windows Desktop Build

From a Windows machine or Windows-capable Go toolchain:

```bash
go build -o pacwallet-desktop.exe ./cmd/pacwallet-desktop
```

Recommended first-run command:

```bash
pacwallet-desktop.exe --network simnet --rpc http://127.0.0.1:9509 --browser edge
```

To build a releasable Windows directory with launch scripts from macOS/Linux:

```bash
./scripts/build-windows-release.sh
```

That release directory now includes:
- a desktop config file with startup defaults
- a mainnet upstream template with `server1/server2/server3` RPC placeholders
- automatic template import when `pacwallet-desktop.json` points at that file
- a machine-readable release manifest
- a generated `branding/` directory with app and website icon assets
- an Inno Setup installer template and signing helper scripts
- Windows-native `build-installer.bat` and `sign-release.bat` helpers
- a zip archive when `ditto` is available

## Native Qt Desktop Wallet

The long-term desktop direction is now:

- backend wallet/core: `Go`
- native desktop front end: `C++/Qt`

The first native Qt project scaffold lives in:

- [qt/CMakeLists.txt](/Users/fanye/Documents/pacwallet/qt/CMakeLists.txt)
- [qt/README.md](/Users/fanye/Documents/pacwallet/qt/README.md)

This path is now the primary native-wallet direction, with:

- first-run create / restore flow
- overview with wallet state and UTXOs
- receive copy/export actions
- send confirmation flow
- filtered history and tx drill-down
- multisig preview and export
- security, backup, import, and upstream controls in settings

For macOS native packaging:

```bash
VERSION=0.3.0-rc1 ./scripts/build-macos-qt-release.sh
```

For Windows native Qt packaging on a Windows build machine:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-windows-qt-toolchain.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows-qt-release.ps1
```

## Security Status

Private keys can be encrypted with passphrase-derived Argon2id keys and
AES-256-GCM. Existing plaintext developer wallets remain readable for local
testing, but launch wallets should be created encrypted from the start.

This wallet is still a developer milestone. Treat mainnet launch use as blocked
until final project multisig parameters, network activation policy, and release
signing are completed in coordination with `pacd`.
