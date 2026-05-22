package wallet

import (
	"encoding/hex"
	"fmt"

	"github.com/Pingancoin/pacwallet/internal/address"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/txscript"
	"github.com/Pingancoin/pacwallet/internal/wire"
	"github.com/decred/dcrd/dcrec/secp256k1/v4"
)

type DraftTx struct {
	Tx          *wire.MsgTx
	Selected    []UTXO
	InputTotal  int64
	OutputTotal int64
	Fee         int64
	Change      int64
	ChangeAddr  string
	Destination string
}

type PaymentOutput struct {
	Address string
	Amount  int64
}

func BuildDraftTx(params *chaincfg.Params, w *Wallet, balance Balance, destination string, amount int64, fee int64, changeAddr string) (DraftTx, error) {
	return BuildDraftTxMany(params, w, balance, []PaymentOutput{{Address: destination, Amount: amount}}, fee, changeAddr)
}

func BuildDraftTxMany(params *chaincfg.Params, w *Wallet, balance Balance, outputs []PaymentOutput, fee int64, changeAddr string) (DraftTx, error) {
	if len(outputs) == 0 {
		return DraftTx{}, fmt.Errorf("at least one payment output is required")
	}
	var amount int64
	for _, output := range outputs {
		if output.Amount <= 0 {
			return DraftTx{}, fmt.Errorf("amount must be positive")
		}
		next := amount + output.Amount
		if next < amount {
			return DraftTx{}, fmt.Errorf("amount overflow")
		}
		amount = next
	}
	if amount <= 0 {
		return DraftTx{}, fmt.Errorf("amount must be positive")
	}
	if fee < 0 {
		return DraftTx{}, fmt.Errorf("fee must not be negative")
	}
	if changeAddr == "" {
		if len(w.Keys) == 0 {
			return DraftTx{}, fmt.Errorf("wallet has no change address")
		}
		changeAddr = w.Keys[0].Address
	}

	changeScript, err := address.DecodeAddressScript(params, changeAddr)
	if err != nil {
		return DraftTx{}, fmt.Errorf("change address: %w", err)
	}

	need := amount + fee
	if need < amount {
		return DraftTx{}, fmt.Errorf("amount overflow")
	}
	selected, inputTotal, err := selectUTXOs(balance.UTXOs, need)
	if err != nil {
		return DraftTx{}, err
	}
	change := inputTotal - need

	tx := &wire.MsgTx{Version: 1}
	for _, utxo := range selected {
		hashBytes, err := hex.DecodeString(utxo.TxHash)
		if err != nil {
			return DraftTx{}, err
		}
		hash, err := wire.NewHashFromBytes(hashBytes)
		if err != nil {
			return DraftTx{}, err
		}
		tx.TxIn = append(tx.TxIn, &wire.TxIn{
			PreviousOutPoint: wire.OutPoint{
				Hash:  hash,
				Index: utxo.Vout,
			},
			Sequence: wire.MaxUint32,
		})
	}
	for _, output := range outputs {
		destScript, err := address.DecodeAddressScript(params, output.Address)
		if err != nil {
			return DraftTx{}, fmt.Errorf("destination address %q: %w", output.Address, err)
		}
		tx.TxOut = append(tx.TxOut, &wire.TxOut{
			Value:    output.Amount,
			PkScript: destScript,
		})
	}
	if change > 0 {
		tx.TxOut = append(tx.TxOut, &wire.TxOut{
			Value:    change,
			PkScript: changeScript,
		})
	}

	return DraftTx{
		Tx:          tx,
		Selected:    selected,
		InputTotal:  inputTotal,
		OutputTotal: amount + change,
		Fee:         fee,
		Change:      change,
		ChangeAddr:  changeAddr,
		Destination: outputs[0].Address,
	}, nil
}

func SignDraftTx(params *chaincfg.Params, w *Wallet, draft DraftTx) error {
	return SignDraftTxWithPassphrase(params, w, draft, "")
}

func SignDraftTxWithPassphrase(params *chaincfg.Params, w *Wallet, draft DraftTx, passphrase string) error {
	keysByAddress := make(map[string]Key, len(w.Keys))
	for _, key := range w.Keys {
		keysByAddress[key.Address] = key
	}
	if len(draft.Selected) != len(draft.Tx.TxIn) {
		return fmt.Errorf("selected utxo count does not match input count")
	}
	for inputIndex, utxo := range draft.Selected {
		key, ok := keysByAddress[utxo.Address]
		if !ok {
			return fmt.Errorf("no wallet key for %s", utxo.Address)
		}
		privBytes, err := w.PrivateKeyBytes(key, passphrase)
		if err != nil {
			return err
		}
		priv := secp256k1.PrivKeyFromBytes(privBytes)
		prevPkScript, err := address.DecodeAddressScript(params, utxo.Address)
		if err != nil {
			return err
		}
		if err := txscript.SignP2PKHInput(draft.Tx, inputIndex, prevPkScript, priv); err != nil {
			return err
		}
	}
	return nil
}

func selectUTXOs(utxos []UTXO, need int64) ([]UTXO, int64, error) {
	var selected []UTXO
	var total int64
	for _, utxo := range utxos {
		if !isSpendableUTXO(utxo) {
			continue
		}
		selected = append(selected, utxo)
		total += utxo.Value
		if total >= need {
			return selected, total, nil
		}
	}
	return nil, 0, fmt.Errorf("insufficient funds: need %d atoms, have %d atoms", need, total)
}

func isSpendableUTXO(utxo UTXO) bool {
	if utxo.Pending {
		return false
	}
	if utxo.Coinbase {
		return utxo.Mature
	}
	return true
}
