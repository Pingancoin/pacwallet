# Pingancoin `pacwallet`

`pacwallet` is the standalone wallet CLI for Pingancoin / PAC.

This first split-out repository keeps the wallet self-contained while remaining
compatible with the local `pacd` HTTP RPC. It can create encrypted wallets,
generate receiving addresses, import/export private keys, sign P2PKH
transactions, submit transactions to `pacd`, and track spendable, immature, and
pending balances.

## Try It

Use Go 1.25+:

```bash
go test ./...
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet create --network simnet
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet receive --network simnet --label miner-1
go run ./cmd/pacwallet balance --network simnet --rpc http://127.0.0.1:9509
go run ./cmd/pacwallet history --network simnet --rpc http://127.0.0.1:9509
PACWALLET_PASSPHRASE='change-this-dev-passphrase' go run ./cmd/pacwallet send --network simnet --rpc http://127.0.0.1:9509 --to <address> --amount 1.25
```

## Security Status

Private keys can be encrypted with passphrase-derived Argon2id keys and
AES-256-GCM. Existing plaintext developer wallets remain readable for local
testing, but launch wallets should be created encrypted from the start.

This wallet is still a developer milestone. Treat mainnet launch use as blocked
until final project multisig parameters, network activation policy, and release
signing are completed in coordination with `pacd`.
