package service_test

import (
	"encoding/hex"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"sync"
	"testing"

	"github.com/Pingancoin/pacwallet/internal/address"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/service"
	walletcore "github.com/Pingancoin/pacwallet/internal/wallet"
)

func TestServiceOverviewAndAddressCreate(t *testing.T) {
	params := chaincfg.SimNetParams()
	walletDir := t.TempDir()
	chain := &fakePACD{}
	server := chain.server()
	defer server.Close()

	svc := service.New(params, walletDir, server.URL)
	initial, err := svc.Overview()
	if err != nil {
		t.Fatal(err)
	}
	if initial.Wallet.Exists {
		t.Fatal("wallet should not exist yet")
	}

	summary, err := svc.CreateWallet(service.CreateWalletRequest{Passphrase: "correct horse battery staple"})
	if err != nil {
		t.Fatal(err)
	}
	if !summary.Encrypted {
		t.Fatal("wallet should be encrypted")
	}
	loaded, err := walletcore.Load(walletcore.Path(walletDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}
	pkScript, err := address.DecodeAddressScript(params, loaded.Keys[0].Address)
	if err != nil {
		t.Fatal(err)
	}
	chain.setPkScript(hex.EncodeToString(pkScript))

	if _, err := svc.CreateAddress(service.CreateAddressRequest{
		Label:      "second",
		Passphrase: "correct horse battery staple",
	}); err != nil {
		t.Fatal(err)
	}

	overview, err := svc.Overview()
	if err != nil {
		t.Fatal(err)
	}
	if !overview.Wallet.Exists {
		t.Fatal("wallet should exist")
	}
	if overview.Wallet.KeyCount != 2 {
		t.Fatalf("key_count = %d, want 2", overview.Wallet.KeyCount)
	}
	if overview.Balance.Total != 200_000_000 {
		t.Fatalf("balance total = %d, want 200000000", overview.Balance.Total)
	}
}

func TestServiceSend(t *testing.T) {
	params := chaincfg.SimNetParams()
	walletDir := t.TempDir()
	w, err := walletcore.Create(walletcore.Path(walletDir, params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := w.AddKey(params, "dest"); err != nil {
		t.Fatal(err)
	}
	if err := walletcore.Save(walletcore.Path(walletDir, params.Name), w); err != nil {
		t.Fatal(err)
	}
	pkScript, err := address.DecodeAddressScript(params, w.Keys[0].Address)
	if err != nil {
		t.Fatal(err)
	}

	server := newFakePACDServer(hex.EncodeToString(pkScript))
	defer server.Close()

	svc := service.New(params, walletDir, server.URL)
	result, err := svc.Send(service.SendRequest{
		To:     w.Keys[1].Address,
		Amount: "1.00000000",
		Fee:    "0.00010000",
	})
	if err != nil {
		t.Fatal(err)
	}
	if !result.Accepted {
		t.Fatal("send should be accepted")
	}
	if result.TxID != "submitted-tx" {
		t.Fatalf("txid = %s, want submitted-tx", result.TxID)
	}
	if result.Change <= 0 {
		t.Fatalf("change = %d, want > 0", result.Change)
	}
}

func TestServiceRestoreWallet(t *testing.T) {
	params := chaincfg.SimNetParams()
	targetDir := t.TempDir()
	sourceDir := t.TempDir()

	sourceWallet, err := walletcore.Create(walletcore.Path(sourceDir, params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := sourceWallet.AddKey(params, "restored"); err != nil {
		t.Fatal(err)
	}
	if err := walletcore.Save(walletcore.Path(sourceDir, params.Name), sourceWallet); err != nil {
		t.Fatal(err)
	}
	sourceData, err := os.ReadFile(walletcore.Path(sourceDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}

	svc := service.New(params, targetDir, "http://127.0.0.1:9509")
	summary, err := svc.RestoreWallet(service.RestoreWalletRequest{Data: sourceData})
	if err != nil {
		t.Fatal(err)
	}
	if !summary.Exists || summary.KeyCount != 2 {
		t.Fatalf("summary exists=%t key_count=%d", summary.Exists, summary.KeyCount)
	}
	if len(summary.Backups) != 0 {
		t.Fatalf("backup count = %d, want 0", len(summary.Backups))
	}
	restored, err := walletcore.Load(walletcore.Path(targetDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}
	if len(restored.Keys) != 2 {
		t.Fatalf("restored keys = %d, want 2", len(restored.Keys))
	}

	if _, err := svc.RestoreWallet(service.RestoreWalletRequest{Data: sourceData}); err != service.ErrWalletAlreadyExists {
		t.Fatalf("restore without overwrite err = %v, want %v", err, service.ErrWalletAlreadyExists)
	}

	overwriteDir := t.TempDir()
	overwriteSource, err := walletcore.Create(walletcore.Path(overwriteDir, params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := overwriteSource.AddKey(params, "replacement"); err != nil {
		t.Fatal(err)
	}
	if err := overwriteSource.AddKey(params, "replacement-2"); err != nil {
		t.Fatal(err)
	}
	if err := walletcore.Save(walletcore.Path(overwriteDir, params.Name), overwriteSource); err != nil {
		t.Fatal(err)
	}
	overwriteData, err := os.ReadFile(walletcore.Path(overwriteDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}

	summary, err = svc.RestoreWallet(service.RestoreWalletRequest{
		Data:      overwriteData,
		Overwrite: true,
	})
	if err != nil {
		t.Fatal(err)
	}
	if summary.KeyCount != 3 {
		t.Fatalf("restored overwrite key_count = %d, want 3", summary.KeyCount)
	}
	if len(summary.Backups) != 1 {
		t.Fatalf("backup count after overwrite = %d, want 1", len(summary.Backups))
	}
	if _, _, err := svc.BackupFile(summary.Backups[0].Name); err != nil {
		t.Fatalf("backup file read err = %v", err)
	}
}

type fakePACD struct {
	mu          sync.RWMutex
	pkScriptHex string
}

func (f *fakePACD) setPkScript(pkScriptHex string) {
	f.mu.Lock()
	defer f.mu.Unlock()
	f.pkScriptHex = pkScriptHex
}

func (f *fakePACD) currentPkScript() string {
	f.mu.RLock()
	defer f.mu.RUnlock()
	return f.pkScriptHex
}

func (f *fakePACD) server() *httptest.Server {
	mux := http.NewServeMux()
	mux.HandleFunc("/getbestblock", func(w http.ResponseWriter, r *http.Request) {
		_ = json.NewEncoder(w).Encode(map[string]any{
			"height": 0,
			"hash":   "block-0",
		})
	})
	mux.HandleFunc("/getblock/0", func(w http.ResponseWriter, r *http.Request) {
		_ = json.NewEncoder(w).Encode(map[string]any{
			"height": 0,
			"hash":   "block-0",
			"tx": []map[string]any{{
				"hash":     "0000000000000000000000000000000000000000000000000000000000000001",
				"coinbase": false,
				"vin":      []map[string]any{},
				"vout": []map[string]any{{
					"n":        0,
					"value":    200_000_000,
					"pkscript": f.currentPkScript(),
				}},
			}},
		})
	})
	mux.HandleFunc("/getrawmempool", func(w http.ResponseWriter, r *http.Request) {
		_ = json.NewEncoder(w).Encode(map[string]any{
			"size": 0,
			"tx":   []any{},
		})
	})
	mux.HandleFunc("/submitrawtransaction", func(w http.ResponseWriter, r *http.Request) {
		_ = json.NewDecoder(r.Body).Decode(&map[string]string{})
		_ = json.NewEncoder(w).Encode(map[string]any{
			"accepted":    true,
			"txid":        "submitted-tx",
			"mempoolsize": 1,
		})
	})
	return httptest.NewServer(mux)
}

func newFakePACDServer(pkScriptHex string) *httptest.Server {
	chain := &fakePACD{pkScriptHex: pkScriptHex}
	return chain.server()
}
