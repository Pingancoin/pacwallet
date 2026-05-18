package main

import (
	"os"
	"path/filepath"
	"testing"
)

func TestApplyDesktopConfig(t *testing.T) {
	network := "simnet"
	walletDir := "/tmp/default-wallet"
	rpcURL := "http://127.0.0.1:9509"
	listen := "127.0.0.1:0"
	browser := "auto"
	title := defaultDesktopTitle

	applyDesktopConfig(map[string]bool{
		"network": true,
	}, desktopConfig{
		Network:   "mainnet",
		WalletDir: "/tmp/custom-wallet",
		RPCURL:    "https://server1.pingancoin.org",
		Listen:    "127.0.0.1:19709",
		Browser:   "edge",
		Title:     "PAC Wallet Desktop",
	}, &network, &walletDir, &rpcURL, &listen, &browser, &title)

	if network != "simnet" {
		t.Fatalf("network = %s, want simnet", network)
	}
	if walletDir != "/tmp/custom-wallet" {
		t.Fatalf("walletDir = %s", walletDir)
	}
	if rpcURL != "https://server1.pingancoin.org" {
		t.Fatalf("rpcURL = %s", rpcURL)
	}
	if listen != "127.0.0.1:19709" {
		t.Fatalf("listen = %s", listen)
	}
	if browser != "edge" {
		t.Fatalf("browser = %s", browser)
	}
	if title != "PAC Wallet Desktop" {
		t.Fatalf("title = %s", title)
	}
}

func TestLoadDesktopConfig(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "pacwallet-desktop.json")
	data := []byte(`{"network":"mainnet","rpc_url":"https://server2.pingancoin.org","browser":"edge"}`)
	if err := os.WriteFile(path, data, 0o600); err != nil {
		t.Fatal(err)
	}

	cfg, loadedPath, err := loadDesktopConfig(path)
	if err != nil {
		t.Fatal(err)
	}
	if loadedPath != path {
		t.Fatalf("loadedPath = %s, want %s", loadedPath, path)
	}
	if cfg.Network != "mainnet" || cfg.RPCURL != "https://server2.pingancoin.org" || cfg.Browser != "edge" {
		t.Fatalf("config = %+v", cfg)
	}
}
