package web_test

import (
	"encoding/hex"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
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
