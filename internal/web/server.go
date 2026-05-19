package web

import (
	"embed"
	"encoding/json"
	"errors"
	"fmt"
	"html/template"
	"io"
	"io/fs"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"

	"github.com/Pingancoin/pacwallet/internal/service"
	"github.com/Pingancoin/pacwallet/internal/wallet"
	"github.com/skip2/go-qrcode"
)

//go:embed templates/*.html static/* static/branding/*
var assets embed.FS

type Server struct {
	service   *service.Service
	templates *template.Template
	mux       *http.ServeMux
}

type ViewData struct {
	Title            string
	Now              time.Time
	Overview         service.Overview
	Notice           string
	Error            string
	DisplayedHistory []wallet.HistoryEntry
	HistoryFilter    string
	HistorySearch    string
	HistoryCount     int
	SelectedKey      *service.KeySummary
	ReceiveURI       string
	PubKeysExport    string
	MultiSigPreview  *service.MultiSigPreviewResult
	MultiSigPubKeys  string
	MultiSigRequired int
	Transaction      *wallet.TransactionDetail
}

func New(svc *service.Service) (*Server, error) {
	funcs := template.FuncMap{
		"formatPAC": wallet.FormatPAC,
		"short":     short,
	}
	templates, err := template.New("").Funcs(funcs).ParseFS(assets, "templates/*.html")
	if err != nil {
		return nil, err
	}
	server := &Server{
		service:   svc,
		templates: templates,
		mux:       http.NewServeMux(),
	}
	server.routes()
	return server, nil
}

func (s *Server) Handler() http.Handler {
	return s.mux
}

func (s *Server) routes() {
	staticFS, err := fs.Sub(assets, "static")
	if err != nil {
		panic(err)
	}
	s.mux.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.FS(staticFS))))
	s.mux.HandleFunc("/", s.handleHome)
	s.mux.HandleFunc("/healthz", s.handleHealth)
	s.mux.HandleFunc("/download/wallet", s.handleWalletDownload)
	s.mux.HandleFunc("/download/pubkeys", s.handlePubKeysDownload)
	s.mux.HandleFunc("/download/backup/", s.handleBackupDownload)
	s.mux.HandleFunc("/receive/qr/", s.handleReceiveQR)
	s.mux.HandleFunc("/tx/", s.handleTransactionDetail)
	s.mux.HandleFunc("/wallet/create", s.handleWalletCreateForm)
	s.mux.HandleFunc("/wallet/encrypt", s.handleWalletEncryptForm)
	s.mux.HandleFunc("/wallet/changepassphrase", s.handleWalletChangePassphraseForm)
	s.mux.HandleFunc("/wallet/restore", s.handleWalletRestoreForm)
	s.mux.HandleFunc("/upstream/add", s.handleUpstreamAddForm)
	s.mux.HandleFunc("/upstream/select", s.handleUpstreamSelectForm)
	s.mux.HandleFunc("/addresses/create", s.handleAddressCreateForm)
	s.mux.HandleFunc("/keys/import", s.handleImportKeyForm)
	s.mux.HandleFunc("/multisig/preview", s.handleMultiSigPreviewForm)
	s.mux.HandleFunc("/send", s.handleSendForm)

	s.mux.HandleFunc("/api/overview", s.handleOverviewAPI)
	s.mux.HandleFunc("/api/wallet/create", s.handleWalletCreateAPI)
	s.mux.HandleFunc("/api/wallet/encrypt", s.handleWalletEncryptAPI)
	s.mux.HandleFunc("/api/wallet/changepassphrase", s.handleWalletChangePassphraseAPI)
	s.mux.HandleFunc("/api/wallet/restore", s.handleWalletRestoreAPI)
	s.mux.HandleFunc("/api/tx/", s.handleTransactionDetailAPI)
	s.mux.HandleFunc("/api/upstreams", s.handleUpstreamAddAPI)
	s.mux.HandleFunc("/api/upstreams/select", s.handleUpstreamSelectAPI)
	s.mux.HandleFunc("/api/addresses", s.handleAddressCreateAPI)
	s.mux.HandleFunc("/api/keys/import", s.handleImportKeyAPI)
	s.mux.HandleFunc("/api/multisig/preview", s.handleMultiSigPreviewAPI)
	s.mux.HandleFunc("/api/send", s.handleSendAPI)
}

func (s *Server) handleHealth(w http.ResponseWriter, _ *http.Request) {
	overview, err := s.service.Overview()
	w.Header().Set("Content-Type", "application/json")
	if err != nil {
		writeJSON(w, http.StatusServiceUnavailable, map[string]any{
			"ok":    false,
			"error": err.Error(),
		})
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{
		"ok":       overview.Node.Online,
		"endpoint": overview.Node.Endpoint,
		"network":  overview.Node.Network,
		"height":   overview.Node.BestHeight,
		"error":    overview.Node.Error,
	})
}

func (s *Server) handleHome(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/" {
		http.NotFound(w, r)
		return
	}
	overview, err := s.service.Overview()
	if err != nil {
		s.renderHome(w, http.StatusBadGateway, service.Overview{
			Wallet: service.WalletSummary{
				Path:    s.service.WalletPath(),
				Network: "",
			},
		}, "", err.Error(), nil, "", 3, "", "", nil)
		return
	}
	filter := strings.TrimSpace(r.URL.Query().Get("history"))
	search := strings.TrimSpace(r.URL.Query().Get("q"))
	selectedKey := selectReceiveKey(overview.Wallet.Keys, strings.TrimSpace(r.URL.Query().Get("receive")))
	s.renderHome(w, http.StatusOK, overview, r.URL.Query().Get("notice"), r.URL.Query().Get("error"), nil, "", 3, filter, search, selectedKey)
}

func (s *Server) handleWalletDownload(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	data, name, err := s.service.WalletFile()
	if err != nil {
		if errors.Is(err, service.ErrWalletNotFound) {
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Content-Disposition", `attachment; filename="`+name+`"`)
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write(data)
}

func (s *Server) handlePubKeysDownload(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	data, name, err := s.service.PubKeysFile()
	if err != nil {
		if errors.Is(err, service.ErrWalletNotFound) {
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("Content-Disposition", `attachment; filename="`+name+`"`)
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write(data)
}

func (s *Server) handleReceiveQR(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	address := strings.TrimSpace(strings.TrimPrefix(r.URL.Path, "/receive/qr/"))
	if address == "" {
		http.NotFound(w, r)
		return
	}
	key, err := s.service.KeyByAddress(address)
	if err != nil {
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}
	size := 240
	if raw := strings.TrimSpace(r.URL.Query().Get("size")); raw != "" {
		if parsed, parseErr := strconv.Atoi(raw); parseErr == nil && parsed >= 96 && parsed <= 512 {
			size = parsed
		}
	}
	png, err := qrcode.Encode(receiveURI(key.Address), qrcode.Medium, size)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "image/png")
	w.Header().Set("Cache-Control", "no-store")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write(png)
}

func (s *Server) handleBackupDownload(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	name := strings.TrimPrefix(r.URL.Path, "/download/backup/")
	data, fileName, err := s.service.BackupFile(name)
	if err != nil {
		if errors.Is(err, service.ErrBackupNotFound) {
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Content-Disposition", `attachment; filename="`+fileName+`"`)
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write(data)
}

func (s *Server) handleWalletCreateForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.CreateWallet(service.CreateWalletRequest{
		Passphrase: strings.TrimSpace(r.FormValue("passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet create failed", err)
		return
	}
	s.redirectNotice(w, r, "Wallet created.")
}

func (s *Server) handleWalletRestoreForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseMultipartForm(8 << 20); err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet restore failed", err)
		return
	}
	file, _, err := r.FormFile("walletfile")
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet restore failed", err)
		return
	}
	defer file.Close()
	data, err := io.ReadAll(file)
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet restore failed", err)
		return
	}
	overwrite := r.FormValue("overwrite") == "on"
	_, err = s.service.RestoreWallet(service.RestoreWalletRequest{
		Data:      data,
		Overwrite: overwrite,
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet restore failed", err)
		return
	}
	notice := "Wallet restored."
	if overwrite {
		notice = "Wallet restored and previous wallet archived."
	}
	s.redirectNotice(w, r, notice)
}

func (s *Server) handleWalletEncryptForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.EncryptWallet(service.EncryptWalletRequest{
		Passphrase: strings.TrimSpace(r.FormValue("passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "wallet encryption failed", err)
		return
	}
	s.redirectNotice(w, r, "Wallet encryption enabled.")
}

func (s *Server) handleWalletChangePassphraseForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.ChangePassphrase(service.ChangePassphraseRequest{
		OldPassphrase: strings.TrimSpace(r.FormValue("old_passphrase")),
		NewPassphrase: strings.TrimSpace(r.FormValue("new_passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "passphrase change failed", err)
		return
	}
	s.redirectNotice(w, r, "Wallet passphrase updated.")
}

func (s *Server) handleAddressCreateForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.CreateAddress(service.CreateAddressRequest{
		Label:      strings.TrimSpace(r.FormValue("label")),
		Passphrase: strings.TrimSpace(r.FormValue("passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "address create failed", err)
		return
	}
	s.redirectNotice(w, r, "New receive address created.")
}

func (s *Server) handleUpstreamAddForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.AddUpstream(service.AddUpstreamRequest{
		Name:       strings.TrimSpace(r.FormValue("name")),
		URL:        strings.TrimSpace(r.FormValue("url")),
		MakeActive: r.FormValue("make_active") == "on",
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "upstream add failed", err)
		return
	}
	s.redirectNotice(w, r, "Node endpoint saved.")
}

func (s *Server) handleUpstreamSelectForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.SelectUpstream(service.SelectUpstreamRequest{
		ID: strings.TrimSpace(r.FormValue("upstream_id")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "upstream switch failed", err)
		return
	}
	s.redirectNotice(w, r, "Node endpoint switched.")
}

func (s *Server) handleImportKeyForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	_, err := s.service.ImportPrivateKey(service.ImportPrivateKeyRequest{
		Label:      strings.TrimSpace(r.FormValue("label")),
		PrivKeyHex: strings.TrimSpace(r.FormValue("privkey")),
		Passphrase: strings.TrimSpace(r.FormValue("passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "private key import failed", err)
		return
	}
	s.redirectNotice(w, r, "Private key imported.")
}

func (s *Server) handleMultiSigPreviewForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	required := 3
	if raw := strings.TrimSpace(r.FormValue("required")); raw != "" {
		if parsed, err := strconv.Atoi(raw); err == nil {
			required = parsed
		}
	}
	pubKeysText := strings.TrimSpace(r.FormValue("pubkeys"))
	preview, err := s.service.PreviewMultiSig(service.MultiSigPreviewRequest{
		Required: required,
		PubKeys:  splitLines(pubKeysText),
	})
	if err != nil {
		s.renderMultiSigError(w, http.StatusBadRequest, pubKeysText, required, err)
		return
	}
	overview, loadErr := s.service.Overview()
	if loadErr != nil {
		s.renderMultiSigError(w, http.StatusBadGateway, pubKeysText, required, loadErr)
		return
	}
	filter := strings.TrimSpace(r.URL.Query().Get("history"))
	search := strings.TrimSpace(r.URL.Query().Get("q"))
	selectedKey := selectReceiveKey(overview.Wallet.Keys, strings.TrimSpace(r.URL.Query().Get("receive")))
	s.renderHome(w, http.StatusOK, overview, "", "", &preview, pubKeysText, required, filter, search, selectedKey)
}

func (s *Server) handleSendForm(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	result, err := s.service.Send(service.SendRequest{
		To:         strings.TrimSpace(r.FormValue("to")),
		Amount:     strings.TrimSpace(r.FormValue("amount")),
		Fee:        strings.TrimSpace(r.FormValue("fee")),
		Change:     strings.TrimSpace(r.FormValue("change")),
		Passphrase: strings.TrimSpace(r.FormValue("passphrase")),
	})
	if err != nil {
		s.renderFormError(w, http.StatusBadRequest, "send failed", err)
		return
	}
	s.redirectNotice(w, r, "Transaction sent: "+result.TxID)
}

func (s *Server) handleTransactionDetail(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	txHash := strings.TrimSpace(strings.TrimPrefix(r.URL.Path, "/tx/"))
	if txHash == "" {
		http.NotFound(w, r)
		return
	}
	overview, err := s.service.Overview()
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}
	txDetail, err := s.service.TransactionDetail(txHash)
	if err != nil {
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}
	s.renderTransaction(w, http.StatusOK, overview, txDetail)
}

func (s *Server) handleOverviewAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	overview, err := s.service.Overview()
	if err != nil {
		writeJSON(w, http.StatusBadGateway, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, overview)
}

func (s *Server) handleWalletCreateAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.CreateWalletRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.CreateWallet(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusCreated, result)
}

func (s *Server) handleWalletEncryptAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.EncryptWalletRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.EncryptWallet(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) handleWalletChangePassphraseAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.ChangePassphraseRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.ChangePassphrase(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) handleWalletRestoreAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req struct {
		WalletJSON string `json:"wallet_json"`
		Overwrite  bool   `json:"overwrite"`
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.RestoreWallet(service.RestoreWalletRequest{
		Data:      []byte(req.WalletJSON),
		Overwrite: req.Overwrite,
	})
	if err != nil {
		status := http.StatusBadRequest
		if errors.Is(err, service.ErrWalletAlreadyExists) {
			status = http.StatusConflict
		}
		writeJSON(w, status, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusCreated, result)
}

func (s *Server) handleTransactionDetailAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	txHash := strings.TrimSpace(strings.TrimPrefix(r.URL.Path, "/api/tx/"))
	if txHash == "" {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "transaction hash is required"})
		return
	}
	result, err := s.service.TransactionDetail(txHash)
	if err != nil {
		writeJSON(w, http.StatusNotFound, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) handleAddressCreateAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.CreateAddressRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.CreateAddress(req)
	if err != nil {
		status := http.StatusBadRequest
		if errors.Is(err, service.ErrWalletNotFound) {
			status = http.StatusNotFound
		}
		writeJSON(w, status, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusCreated, result)
}

func (s *Server) handleUpstreamAddAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.AddUpstreamRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.AddUpstream(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusCreated, result)
}

func (s *Server) handleUpstreamSelectAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.SelectUpstreamRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.SelectUpstream(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) handleImportKeyAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.ImportPrivateKeyRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.ImportPrivateKey(req)
	if err != nil {
		status := http.StatusBadRequest
		if errors.Is(err, service.ErrWalletNotFound) {
			status = http.StatusNotFound
		}
		writeJSON(w, status, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusCreated, result)
}

func (s *Server) handleSendAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.SendRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.Send(req)
	if err != nil {
		status := http.StatusBadRequest
		if errors.Is(err, service.ErrWalletNotFound) {
			status = http.StatusNotFound
		}
		writeJSON(w, status, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) handleMultiSigPreviewAPI(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req service.MultiSigPreviewRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	result, err := s.service.PreviewMultiSig(req)
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}
	writeJSON(w, http.StatusOK, result)
}

func (s *Server) renderHome(w http.ResponseWriter, status int, overview service.Overview, notice string, errText string, preview *service.MultiSigPreviewResult, pubKeysText string, required int, historyFilter string, historySearch string, selectedKey *service.KeySummary) {
	displayedHistory := filterHistory(overview.History, historyFilter, historySearch)
	if selectedKey == nil {
		selectedKey = selectReceiveKey(overview.Wallet.Keys, "")
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(status)
	_ = s.templates.ExecuteTemplate(w, "home", ViewData{
		Title:            "PAC Wallet",
		Now:              time.Now().UTC(),
		Overview:         overview,
		Notice:           notice,
		Error:            errText,
		DisplayedHistory: displayedHistory,
		HistoryFilter:    normalizeHistoryFilter(historyFilter),
		HistorySearch:    historySearch,
		HistoryCount:     len(displayedHistory),
		SelectedKey:      selectedKey,
		ReceiveURI:       receiveURIFromKey(selectedKey),
		PubKeysExport:    buildPubKeysExport(overview.Wallet.Keys),
		MultiSigPreview:  preview,
		MultiSigPubKeys:  pubKeysText,
		MultiSigRequired: required,
	})
}

func (s *Server) renderTransaction(w http.ResponseWriter, status int, overview service.Overview, txDetail wallet.TransactionDetail) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(status)
	_ = s.templates.ExecuteTemplate(w, "tx", ViewData{
		Title:       "Transaction " + short(txDetail.TxHash),
		Now:         time.Now().UTC(),
		Overview:    overview,
		Transaction: &txDetail,
	})
}

func (s *Server) renderFormError(w http.ResponseWriter, status int, prefix string, err error) {
	overview, loadErr := s.service.Overview()
	if loadErr != nil && !errors.Is(loadErr, service.ErrWalletNotFound) {
		overview = service.Overview{
			Wallet: service.WalletSummary{
				Path: s.service.WalletPath(),
			},
		}
	}
	s.renderHome(w, status, overview, "", prefix+": "+err.Error(), nil, "", 3, "", "", nil)
}

func (s *Server) renderMultiSigError(w http.ResponseWriter, status int, pubKeysText string, required int, err error) {
	overview, loadErr := s.service.Overview()
	if loadErr != nil && !errors.Is(loadErr, service.ErrWalletNotFound) {
		overview = service.Overview{
			Wallet: service.WalletSummary{
				Path: s.service.WalletPath(),
			},
		}
	}
	s.renderHome(w, status, overview, "", "multisig preview failed: "+err.Error(), nil, pubKeysText, required, "", "", nil)
}

func (s *Server) redirectNotice(w http.ResponseWriter, r *http.Request, notice string) {
	target := "/?notice=" + url.QueryEscape(notice)
	http.Redirect(w, r, target, http.StatusSeeOther)
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(v)
}

func short(value string) string {
	if len(value) <= 16 {
		return value
	}
	return value[:8] + "..." + value[len(value)-8:]
}

func selectReceiveKey(keys []service.KeySummary, requested string) *service.KeySummary {
	if len(keys) == 0 {
		return nil
	}
	requested = strings.TrimSpace(requested)
	if requested != "" {
		for i := range keys {
			if keys[i].Address == requested {
				key := keys[i]
				return &key
			}
		}
	}
	key := keys[0]
	return &key
}

func receiveURI(address string) string {
	return "pingancoin:" + strings.TrimSpace(address)
}

func receiveURIFromKey(key *service.KeySummary) string {
	if key == nil {
		return ""
	}
	return receiveURI(key.Address)
}

func normalizeHistoryFilter(filter string) string {
	switch strings.ToLower(strings.TrimSpace(filter)) {
	case "incoming", "outgoing", "pending", "coinbase":
		return strings.ToLower(strings.TrimSpace(filter))
	default:
		return "all"
	}
}

func filterHistory(entries []wallet.HistoryEntry, filter string, search string) []wallet.HistoryEntry {
	filter = normalizeHistoryFilter(filter)
	search = strings.ToLower(strings.TrimSpace(search))
	filtered := make([]wallet.HistoryEntry, 0, len(entries))
	for _, entry := range entries {
		switch filter {
		case "incoming":
			if entry.Net <= 0 {
				continue
			}
		case "outgoing":
			if entry.Net >= 0 {
				continue
			}
		case "pending":
			if !entry.Pending {
				continue
			}
		case "coinbase":
			if !entry.Coinbase {
				continue
			}
		}
		if search != "" {
			match := strings.Contains(strings.ToLower(entry.TxHash), search)
			if !match {
				for _, addr := range entry.Addresses {
					if strings.Contains(strings.ToLower(addr), search) {
						match = true
						break
					}
				}
			}
			if !match {
				continue
			}
		}
		filtered = append(filtered, entry)
	}
	return filtered
}

func buildPubKeysExport(keys []service.KeySummary) string {
	var b strings.Builder
	for _, key := range keys {
		fmt.Fprintf(&b, "%s %s %s\n", strings.TrimSpace(key.Label), key.Address, key.PubKeyHex)
	}
	return b.String()
}

func splitLines(value string) []string {
	lines := strings.Split(value, "\n")
	out := make([]string, 0, len(lines))
	for _, line := range lines {
		line = strings.TrimSpace(strings.TrimSuffix(line, "\r"))
		if line == "" {
			continue
		}
		for _, field := range strings.Fields(line) {
			if strings.HasPrefix(field, "02") || strings.HasPrefix(field, "03") || strings.HasPrefix(field, "04") {
				out = append(out, field)
			}
		}
	}
	return out
}
