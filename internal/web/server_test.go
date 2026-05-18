package web_test

import (
	"bytes"
	"encoding/hex"
	"encoding/json"
	"io"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"os"
	"strings"
	"testing"

	"github.com/Pingancoin/pacwallet/internal/address"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/service"
	walletcore "github.com/Pingancoin/pacwallet/internal/wallet"
	"github.com/Pingancoin/pacwallet/internal/web"
)

func TestServerSetupAndDashboard(t *testing.T) {
	params := chaincfg.SimNetParams()
	tempWallet, err := walletcore.Create(walletcore.Path(t.TempDir(), params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	pkScript, err := address.DecodeAddressScript(params, tempWallet.Keys[0].Address)
	if err != nil {
		t.Fatal(err)
	}

	fakePACD := newFakePACDServer(hex.EncodeToString(pkScript))
	defer fakePACD.Close()

	svc := service.New(params, t.TempDir(), fakePACD.URL)
	server, err := web.New(svc)
	if err != nil {
		t.Fatal(err)
	}
	ts := httptest.NewServer(server.Handler())
	defer ts.Close()

	resp, err := http.Get(ts.URL + "/")
	if err != nil {
		t.Fatal(err)
	}
	body := mustReadString(t, resp)
	if !strings.Contains(body, "Create your wallet") {
		t.Fatalf("home body missing setup state: %s", body)
	}

	client := &http.Client{
		CheckRedirect: func(req *http.Request, via []*http.Request) error {
			return http.ErrUseLastResponse
		},
	}
	createResp, err := client.Post(ts.URL+"/wallet/create", "application/x-www-form-urlencoded", strings.NewReader("passphrase="))
	if err != nil {
		t.Fatal(err)
	}
	if createResp.StatusCode != http.StatusSeeOther {
		t.Fatalf("create status = %d, want 303", createResp.StatusCode)
	}

	resp, err = http.Get(ts.URL + "/")
	if err != nil {
		t.Fatal(err)
	}
	body = mustReadString(t, resp)
	if !strings.Contains(body, "Wallet addresses") {
		t.Fatalf("home body missing dashboard state: %s", body)
	}
	if !strings.Contains(body, "Broadcast transaction") {
		t.Fatalf("home body missing send form: %s", body)
	}
	if !strings.Contains(body, "Wallet upstream") {
		t.Fatalf("home body missing upstream section: %s", body)
	}
}

func TestServerRestoreWalletForm(t *testing.T) {
	params := chaincfg.SimNetParams()
	sourceDir := t.TempDir()
	sourceWallet, err := walletcore.Create(walletcore.Path(sourceDir, params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	if err := sourceWallet.AddKey(params, "restored"); err != nil {
		t.Fatal(err)
	}
	sourceData, err := os.ReadFile(walletcore.Path(sourceDir, params.Name))
	if err != nil {
		t.Fatal(err)
	}

	tempWallet, err := walletcore.Create(walletcore.Path(t.TempDir(), params.Name), params)
	if err != nil {
		t.Fatal(err)
	}
	pkScript, err := address.DecodeAddressScript(params, tempWallet.Keys[0].Address)
	if err != nil {
		t.Fatal(err)
	}
	fakePACD := newFakePACDServer(hex.EncodeToString(pkScript))
	defer fakePACD.Close()

	svc := service.New(params, t.TempDir(), fakePACD.URL)
	server, err := web.New(svc)
	if err != nil {
		t.Fatal(err)
	}
	ts := httptest.NewServer(server.Handler())
	defer ts.Close()

	var body bytes.Buffer
	writer := multipart.NewWriter(&body)
	part, err := writer.CreateFormFile("walletfile", "wallet.json")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := part.Write(sourceData); err != nil {
		t.Fatal(err)
	}
	if err := writer.Close(); err != nil {
		t.Fatal(err)
	}

	client := &http.Client{
		CheckRedirect: func(req *http.Request, via []*http.Request) error {
			return http.ErrUseLastResponse
		},
	}
	req, err := http.NewRequest(http.MethodPost, ts.URL+"/wallet/restore", &body)
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("Content-Type", writer.FormDataContentType())
	resp, err := client.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	if resp.StatusCode != http.StatusSeeOther {
		t.Fatalf("restore status = %d, want 303", resp.StatusCode)
	}

	homeResp, err := http.Get(ts.URL + "/")
	if err != nil {
		t.Fatal(err)
	}
	homeBody := mustReadString(t, homeResp)
	if !strings.Contains(homeBody, "Wallet addresses") {
		t.Fatalf("restored home missing dashboard state: %s", homeBody)
	}
	if !strings.Contains(homeBody, "Archived backups") {
		t.Fatalf("restored home missing backup panel: %s", homeBody)
	}
}

func newFakePACDServer(pkScriptHex string) *httptest.Server {
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
					"pkscript": pkScriptHex,
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
		_ = json.NewEncoder(w).Encode(map[string]any{
			"accepted":    true,
			"txid":        "submitted-tx",
			"mempoolsize": 1,
		})
	})
	return httptest.NewServer(mux)
}

func mustReadString(t *testing.T, resp *http.Response) string {
	t.Helper()
	defer resp.Body.Close()
	data, err := io.ReadAll(resp.Body)
	if err != nil {
		t.Fatal(err)
	}
	return string(data)
}
