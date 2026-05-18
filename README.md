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
- a desktop launcher aimed at Windows app-window usage through Edge or Chrome

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
- `GET /api/overview`
- `POST /api/wallet/create`
- `POST /api/addresses`
- `POST /api/keys/import`
- `POST /api/send`

The desktop launcher starts the same wallet service and opens it in an app-style
browser window. On Windows, `--browser edge` is the preferred default. For
headless smoke tests or manual launches, use `--browser none`.

## Windows Desktop Build

From a Windows machine or Windows-capable Go toolchain:

```bash
go build -o pacwallet-desktop.exe ./cmd/pacwallet-desktop
```

Recommended first-run command:

```bash
pacwallet-desktop.exe --network simnet --rpc http://127.0.0.1:9509 --browser edge
```

## Security Status

Private keys can be encrypted with passphrase-derived Argon2id keys and
AES-256-GCM. Existing plaintext developer wallets remain readable for local
testing, but launch wallets should be created encrypted from the start.

This wallet is still a developer milestone. Treat mainnet launch use as blocked
until final project multisig parameters, network activation policy, and release
signing are completed in coordination with `pacd`.
