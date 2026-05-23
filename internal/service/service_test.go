package service_test

import (
	"encoding/hex"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
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
	if overview.Wallet.Keys[0].PubKeyHex == "" {
		t.Fatal("primary key pubkey should be present")
	}
	if overview.Balance.Total != 200_000_000 {
		t.Fatalf("balance total = %d, want 200000000", overview.Balance.Total)
	}
	if !overview.Node.Online {
		t.Fatal("node should be reported online")
	}
	if overview.Node.BestHeight != 0 {
		t.Fatalf("node best height = %d, want 0", overview.Node.BestHeight)
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

func TestServiceUpstreamProfiles(t *testing.T) {
	params := chaincfg.SimNetParams()
	walletDir := t.TempDir()

	svc := service.New(params, walletDir, "http://127.0.0.1:29509")
	initial, err := svc.Overview()
	if err != nil {
		t.Fatal(err)
	}
	if initial.Upstream.ActiveID != "local-node" {
		t.Fatalf("active upstream = %s, want local-node", initial.Upstream.ActiveID)
	}
	if initial.Upstream.ActiveURL != "http://127.0.0.1:29509" {
		t.Fatalf("active URL = %s", initial.Upstream.ActiveURL)
	}

	settings, err := svc.AddUpstream(service.AddUpstreamRequest{
		Name:       "Official RPC 1",
		URL:        "https://rpc1.pingancoin.org",
		MakeActive: true,
	})
	if err != nil {
		t.Fatal(err)
	}
	if settings.ActiveURL != "https://rpc1.pingancoin.org" {
		t.Fatalf("active URL after add = %s", settings.ActiveURL)
	}
	if len(settings.Profiles) != 2 {
		t.Fatalf("profile count = %d, want 2", len(settings.Profiles))
	}

	settings, err = svc.SelectUpstream(service.SelectUpstreamRequest{ID: "local-node"})
	if err != nil {
		t.Fatal(err)
	}
	if settings.ActiveID != "local-node" || settings.ActiveURL != "http://127.0.0.1:29509" {
		t.Fatalf("select local result = %+v", settings)
	}
}

func TestServiceMigratesLegacyOfficialRPC(t *testing.T) {
	params := chaincfg.MainNetParams()
	walletDir := t.TempDir()
	upstreamsDir := filepath.Join(walletDir, params.Name)
	if err := os.MkdirAll(upstreamsDir, 0o700); err != nil {
		t.Fatal(err)
	}
	upstreamsPath := filepath.Join(upstreamsDir, "upstreams.json")
	if err := os.WriteFile(upstreamsPath, []byte(`{
  "active_id": "official-rpc",
  "profiles": [
    {
      "id": "official-rpc",
      "name": "Official RPC",
      "url": "http://rpc.pingancoin.org/rpc",
      "source": "official"
    }
  ]
}`), 0o600); err != nil {
		t.Fatal(err)
	}

	svc := service.New(params, walletDir, "")
	if svc.RPCURL() != "https://rpc.pingancoin.org/rpc" {
		t.Fatalf("rpc URL = %s, want HTTPS official RPC", svc.RPCURL())
	}
	data, err := os.ReadFile(upstreamsPath)
	if err != nil {
		t.Fatal(err)
	}
	if strings.Contains(string(data), "http://rpc.pingancoin.org/rpc") {
		t.Fatalf("legacy RPC URL was not migrated: %s", string(data))
	}
}

func TestServiceMergeUpstreamTemplate(t *testing.T) {
	params := chaincfg.MainNetParams()
	walletDir := t.TempDir()

	svc := service.New(params, walletDir, "http://127.0.0.1:9509")
	templatePath := filepath.Join(walletDir, "upstreams.mainnet.template.json")
	template := []byte(`{
  "active_id": "server1-rpc",
  "profiles": [
    {
      "id": "local-node",
      "name": "Local Node",
      "url": "http://127.0.0.1:9509",
      "source": "local"
    },
    {
      "id": "server1-rpc",
      "name": "Server 1 RPC",
      "url": "https://server1.pingancoin.org",
      "source": "official"
    },
    {
      "id": "server2-rpc",
      "name": "Server 2 RPC",
      "url": "https://server2.pingancoin.org",
      "source": "official"
    }
  ]
}`)
	if err := os.WriteFile(templatePath, template, 0o600); err != nil {
		t.Fatal(err)
	}

	result, err := svc.MergeUpstreamTemplate(templatePath)
	if err != nil {
		t.Fatal(err)
	}
	if result.Added != 2 {
		t.Fatalf("added = %d, want 2", result.Added)
	}
	if result.Settings.ActiveID != "local-node" {
		t.Fatalf("active ID = %s, want local-node", result.Settings.ActiveID)
	}
	if result.Settings.ActiveURL != "http://127.0.0.1:9509" {
		t.Fatalf("active URL = %s, want local node URL", result.Settings.ActiveURL)
	}
	if len(result.Settings.Profiles) != 3 {
		t.Fatalf("profile count = %d, want 3", len(result.Settings.Profiles))
	}

	result, err = svc.MergeUpstreamTemplate(templatePath)
	if err != nil {
		t.Fatal(err)
	}
	if result.Added != 0 || result.Updated != 0 {
		t.Fatalf("second merge result = %+v, want no changes", result)
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

func TestServiceEncryptPassphraseAndPubKeys(t *testing.T) {
	params := chaincfg.SimNetParams()
	walletDir := t.TempDir()

	svc := service.New(params, walletDir, "http://127.0.0.1:9509")
	if _, err := svc.CreateWallet(service.CreateWalletRequest{}); err != nil {
		t.Fatal(err)
	}

	summary, err := svc.EncryptWallet(service.EncryptWalletRequest{Passphrase: "correct horse battery staple"})
	if err != nil {
		t.Fatal(err)
	}
	if !summary.Encrypted {
		t.Fatal("wallet should be encrypted after encrypt call")
	}

	summary, err = svc.ChangePassphrase(service.ChangePassphraseRequest{
		OldPassphrase: "correct horse battery staple",
		NewPassphrase: "new correct horse battery staple",
	})
	if err != nil {
		t.Fatal(err)
	}
	if !summary.Encrypted {
		t.Fatal("wallet should stay encrypted after passphrase rotation")
	}

	pubkeys, name, err := svc.PubKeysFile()
	if err != nil {
		t.Fatal(err)
	}
	if name != "pubkeys.txt" {
		t.Fatalf("pubkeys filename = %s, want pubkeys.txt", name)
	}
	if len(pubkeys) == 0 {
		t.Fatal("pubkeys file should not be empty")
	}
	if string(pubkeys) == "" {
		t.Fatal("pubkeys text should be populated")
	}
}

func TestServicePreviewMultiSig(t *testing.T) {
	params := chaincfg.MainNetParams()
	walletDir := t.TempDir()

	svc := service.New(params, walletDir, "http://127.0.0.1:9509")
	summary, err := svc.CreateWallet(service.CreateWalletRequest{})
	if err != nil {
		t.Fatal(err)
	}
	w, err := walletcore.Load(walletcore.Path(walletDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}
	for len(w.Keys) < 5 {
		if err := w.AddKey(params, "signer"); err != nil {
			t.Fatal(err)
		}
	}
	if err := walletcore.Save(walletcore.Path(walletDir, params.Name), w); err != nil {
		t.Fatal(err)
	}
	if summary.KeyCount == 0 {
		t.Fatal("expected at least one key in created wallet")
	}

	pubKeys := make([]string, 0, 5)
	for i := 0; i < 5; i++ {
		pubKeys = append(pubKeys, w.Keys[i].PubKeyHex)
	}
	result, err := svc.PreviewMultiSig(service.MultiSigPreviewRequest{
		Required: 3,
		PubKeys:  pubKeys,
	})
	if err != nil {
		t.Fatal(err)
	}
	if result.Required != 3 || result.Participants != 5 {
		t.Fatalf("preview result = %+v", result)
	}
	if result.Address == "" || result.RedeemScript == "" || result.P2SHScript == "" {
		t.Fatalf("incomplete multisig preview: %+v", result)
	}
}

func TestServiceKeyLookupAndTransactionDetail(t *testing.T) {
	params := chaincfg.SimNetParams()
	walletDir := t.TempDir()
	w, err := walletcore.Create(walletcore.Path(walletDir, params.Name), params)
	if err != nil {
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
	key, err := svc.KeyByAddress(w.Keys[0].Address)
	if err != nil {
		t.Fatal(err)
	}
	if key.Address != w.Keys[0].Address {
		t.Fatalf("key address = %s, want %s", key.Address, w.Keys[0].Address)
	}

	txDetail, err := svc.TransactionDetail("0000000000000000000000000000000000000000000000000000000000000001")
	if err != nil {
		t.Fatal(err)
	}
	if txDetail.TxHash == "" || len(txDetail.Outputs) == 0 {
		t.Fatalf("transaction detail not populated: %+v", txDetail)
	}
	if txDetail.Received != 200_000_000 {
		t.Fatalf("received = %d, want 200000000", txDetail.Received)
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
	mux.HandleFunc("/getnetworkinfo", func(w http.ResponseWriter, r *http.Request) {
		_ = json.NewEncoder(w).Encode(map[string]any{
			"network":    "simnet",
			"peers":      3,
			"besthash":   "block-0",
			"bestheight": 0,
		})
	})
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
