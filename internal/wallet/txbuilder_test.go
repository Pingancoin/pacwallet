package wallet_test

import (
	"strings"
	"testing"

	"github.com/Pingancoin/pacwallet/internal/address"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/txscript"
	"github.com/Pingancoin/pacwallet/internal/wallet"
)

func TestParsePACAmount(t *testing.T) {
	tests := map[string]int64{
		"0.00000001": 1,
		"1":          100_000_000,
		"1.23":       123_000_000,
	}
	for input, want := range tests {
		got, err := wallet.ParsePACAmount(input)
		if err != nil {
			t.Fatal(err)
		}
		if got != want {
			t.Fatalf("ParsePACAmount(%q) = %d, want %d", input, got, want)
		}
	}
}

func TestBuildDraftTx(t *testing.T) {
	params := chaincfg.SimNetParams()
	w, err := wallet.Create(wallet.Path(t.TempDir(), params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := w.AddKey(params, "dest"); err != nil {
		t.Fatal(err)
	}

	balance := wallet.Balance{
		UTXOs: []wallet.UTXO{{
			Address: w.Keys[0].Address,
			TxHash:  "0000000000000000000000000000000000000000000000000000000000000001",
			Vout:    0,
			Value:   100_000_000,
			Height:  1,
		}},
	}
	draft, err := wallet.BuildDraftTx(params, w, balance, w.Keys[1].Address, 40_000_000, 1_000, "")
	if err != nil {
		t.Fatal(err)
	}
	if len(draft.Tx.TxIn) != 1 {
		t.Fatalf("inputs = %d, want 1", len(draft.Tx.TxIn))
	}
	if len(draft.Tx.TxOut) != 2 {
		t.Fatalf("outputs = %d, want 2", len(draft.Tx.TxOut))
	}
	if draft.Change != 59_999_000 {
		t.Fatalf("change = %d", draft.Change)
	}
}

func TestBuildDraftTxMany(t *testing.T) {
	params := chaincfg.SimNetParams()
	w, err := wallet.Create(wallet.Path(t.TempDir(), params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := w.AddKey(params, "dest-1"); err != nil {
		t.Fatal(err)
	}
	if err := w.AddKey(params, "dest-2"); err != nil {
		t.Fatal(err)
	}
	balance := wallet.Balance{
		UTXOs: []wallet.UTXO{{
			TxHash:  strings.Repeat("01", 32),
			Vout:    0,
			Value:   100_000_000,
			Address: w.Keys[0].Address,
			Mature:  true,
		}},
	}
	draft, err := wallet.BuildDraftTxMany(params, w, balance, []wallet.PaymentOutput{
		{Address: w.Keys[1].Address, Amount: 25_000_000},
		{Address: w.Keys[2].Address, Amount: 30_000_000},
	}, 1_000, "")
	if err != nil {
		t.Fatal(err)
	}
	if len(draft.Tx.TxOut) != 3 {
		t.Fatalf("outputs = %d, want two payments plus change", len(draft.Tx.TxOut))
	}
	if draft.OutputTotal != 99_999_000 || draft.Change != 44_999_000 {
		t.Fatalf("unexpected draft totals: %+v", draft)
	}
	if err := wallet.SignDraftTx(params, w, draft); err != nil {
		t.Fatal(err)
	}
}

func TestSignDraftTx(t *testing.T) {
	params := chaincfg.SimNetParams()
	w, err := wallet.Create(wallet.Path(t.TempDir(), params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := w.AddKey(params, "dest"); err != nil {
		t.Fatal(err)
	}

	balance := wallet.Balance{
		UTXOs: []wallet.UTXO{{
			Address: w.Keys[0].Address,
			TxHash:  "0000000000000000000000000000000000000000000000000000000000000001",
			Vout:    0,
			Value:   100_000_000,
			Height:  1,
		}},
	}
	draft, err := wallet.BuildDraftTx(params, w, balance, w.Keys[1].Address, 40_000_000, 1_000, "")
	if err != nil {
		t.Fatal(err)
	}
	if err := wallet.SignDraftTx(params, w, draft); err != nil {
		t.Fatal(err)
	}
	if len(draft.Tx.TxIn[0].SignatureScript) == 0 {
		t.Fatal("signature script was not populated")
	}
	prevScript := mustDecodeAddressScript(t, params, w.Keys[0].Address)
	if err := txscript.VerifyP2PKHInput(draft.Tx, 0, prevScript); err != nil {
		t.Fatal(err)
	}
}

func mustDecodeAddressScript(t *testing.T, params *chaincfg.Params, addr string) []byte {
	t.Helper()
	script, err := address.DecodeAddressScript(params, addr)
	if err != nil {
		t.Fatal(err)
	}
	return script
}
