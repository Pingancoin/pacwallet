package web

import (
	"embed"
	"encoding/json"
	"errors"
	"html/template"
	"io"
	"io/fs"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/Pingancoin/pacwallet/internal/service"
	"github.com/Pingancoin/pacwallet/internal/wallet"
)

//go:embed templates/*.html static/* static/branding/*
var assets embed.FS

type Server struct {
	service   *service.Service
	templates *template.Template
	mux       *http.ServeMux
}

type ViewData struct {
	Title    string
	Now      time.Time
	Overview service.Overview
	Notice   string
	Error    string
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
	s.mux.HandleFunc("/download/backup/", s.handleBackupDownload)
	s.mux.HandleFunc("/wallet/create", s.handleWalletCreateForm)
	s.mux.HandleFunc("/wallet/restore", s.handleWalletRestoreForm)
	s.mux.HandleFunc("/upstream/add", s.handleUpstreamAddForm)
	s.mux.HandleFunc("/upstream/select", s.handleUpstreamSelectForm)
	s.mux.HandleFunc("/addresses/create", s.handleAddressCreateForm)
	s.mux.HandleFunc("/keys/import", s.handleImportKeyForm)
	s.mux.HandleFunc("/send", s.handleSendForm)

	s.mux.HandleFunc("/api/overview", s.handleOverviewAPI)
	s.mux.HandleFunc("/api/wallet/create", s.handleWalletCreateAPI)
	s.mux.HandleFunc("/api/wallet/restore", s.handleWalletRestoreAPI)
	s.mux.HandleFunc("/api/upstreams", s.handleUpstreamAddAPI)
	s.mux.HandleFunc("/api/upstreams/select", s.handleUpstreamSelectAPI)
	s.mux.HandleFunc("/api/addresses", s.handleAddressCreateAPI)
	s.mux.HandleFunc("/api/keys/import", s.handleImportKeyAPI)
	s.mux.HandleFunc("/api/send", s.handleSendAPI)
}

func (s *Server) handleHealth(w http.ResponseWriter, _ *http.Request) {
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte("ok\n"))
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
		}, "", err.Error())
		return
	}
	s.renderHome(w, http.StatusOK, overview, r.URL.Query().Get("notice"), r.URL.Query().Get("error"))
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

func (s *Server) renderHome(w http.ResponseWriter, status int, overview service.Overview, notice string, errText string) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.WriteHeader(status)
	_ = s.templates.ExecuteTemplate(w, "home", ViewData{
		Title:    "PAC Wallet",
		Now:      time.Now().UTC(),
		Overview: overview,
		Notice:   notice,
		Error:    errText,
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
	s.renderHome(w, status, overview, "", prefix+": "+err.Error())
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
