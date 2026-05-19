package service

import (
	"bytes"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/Pingancoin/pacwallet/internal/address"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	walletcore "github.com/Pingancoin/pacwallet/internal/wallet"
)

var ErrWalletNotFound = errors.New("wallet not found")
var ErrWalletAlreadyExists = errors.New("wallet already exists")
var ErrBackupNotFound = errors.New("backup not found")

type Service struct {
	params        *chaincfg.Params
	walletPath    string
	endpointsPath string
	rpcURL        string

	mu sync.Mutex
}

type Overview struct {
	Wallet   WalletSummary             `json:"wallet"`
	Balance  walletcore.Balance        `json:"balance"`
	History  []walletcore.HistoryEntry `json:"history"`
	RPCURL   string                    `json:"rpc_url"`
	Upstream UpstreamSettings          `json:"upstream"`
	Node     NodeStatus                `json:"node"`
}

type WalletSummary struct {
	Exists      bool         `json:"exists"`
	Path        string       `json:"path"`
	BackupDir   string       `json:"backup_dir"`
	Network     string       `json:"network"`
	Encrypted   bool         `json:"encrypted"`
	CreatedAt   time.Time    `json:"created_at"`
	KeyCount    int          `json:"key_count"`
	Keys        []KeySummary `json:"keys"`
	AddressHint string       `json:"address_hint"`
	Backups     []BackupInfo `json:"backups"`
}

type KeySummary struct {
	Label     string    `json:"label"`
	Address   string    `json:"address"`
	PubKeyHex string    `json:"pubkey_hex"`
	CreatedAt time.Time `json:"created_at"`
}

type NodeStatus struct {
	Online      bool      `json:"online"`
	Endpoint    string    `json:"endpoint"`
	Network     string    `json:"network"`
	BestHeight  uint32    `json:"best_height"`
	BestHash    string    `json:"best_hash"`
	MempoolSize int       `json:"mempool_size"`
	PeerCount   int       `json:"peer_count"`
	ScannedAt   time.Time `json:"scanned_at"`
	Error       string    `json:"error,omitempty"`
}

type CreateWalletRequest struct {
	Passphrase string `json:"passphrase"`
}

type CreateAddressRequest struct {
	Label      string `json:"label"`
	Passphrase string `json:"passphrase"`
}

type ImportPrivateKeyRequest struct {
	Label      string `json:"label"`
	PrivKeyHex string `json:"privkey_hex"`
	Passphrase string `json:"passphrase"`
}

type SendRequest struct {
	To         string `json:"to"`
	Amount     string `json:"amount"`
	Fee        string `json:"fee"`
	Change     string `json:"change"`
	Passphrase string `json:"passphrase"`
}

type SendResponse struct {
	Accepted    bool   `json:"accepted"`
	TxID        string `json:"txid"`
	MempoolSize int    `json:"mempool_size"`
	InputTotal  int64  `json:"input_total"`
	OutputTotal int64  `json:"output_total"`
	Fee         int64  `json:"fee"`
	Change      int64  `json:"change"`
	ChangeAddr  string `json:"change_address"`
	Destination string `json:"destination"`
}

type RestoreWalletRequest struct {
	Data      []byte `json:"-"`
	Overwrite bool   `json:"overwrite"`
}

type EncryptWalletRequest struct {
	Passphrase string `json:"passphrase"`
}

type ChangePassphraseRequest struct {
	OldPassphrase string `json:"old_passphrase"`
	NewPassphrase string `json:"new_passphrase"`
}

type BackupInfo struct {
	Name      string    `json:"name"`
	Path      string    `json:"path"`
	SizeBytes int64     `json:"size_bytes"`
	CreatedAt time.Time `json:"created_at"`
}

type UpstreamSettings struct {
	ConfigPath string            `json:"config_path"`
	ActiveID   string            `json:"active_id"`
	ActiveURL  string            `json:"active_url"`
	Profiles   []UpstreamProfile `json:"profiles"`
}

type UpstreamProfile struct {
	ID         string    `json:"id"`
	Name       string    `json:"name"`
	URL        string    `json:"url"`
	Source     string    `json:"source"`
	LastUsedAt time.Time `json:"last_used_at,omitempty"`
}

type MergeUpstreamTemplateResult struct {
	Settings UpstreamSettings `json:"settings"`
	Added    int              `json:"added"`
	Updated  int              `json:"updated"`
}

type AddUpstreamRequest struct {
	Name       string `json:"name"`
	URL        string `json:"url"`
	MakeActive bool   `json:"make_active"`
}

type SelectUpstreamRequest struct {
	ID string `json:"id"`
}

type MultiSigPreviewRequest struct {
	Required int      `json:"required"`
	PubKeys  []string `json:"pubkeys"`
}

type MultiSigPreviewResult struct {
	Required     int      `json:"required"`
	Participants int      `json:"participants"`
	PubKeys      []string `json:"pubkeys"`
	Address      string   `json:"address"`
	ScriptHash   string   `json:"script_hash"`
	RedeemScript string   `json:"redeem_script"`
	P2SHScript   string   `json:"p2sh_script"`
}

type ReceiveRequest struct {
	Address string `json:"address"`
}

type upstreamFile struct {
	ActiveID string            `json:"active_id"`
	Profiles []UpstreamProfile `json:"profiles"`
}

func New(params *chaincfg.Params, walletDir string, rpcURL string) *Service {
	s := &Service{
		params:        params,
		walletPath:    walletcore.Path(walletDir, params.Name),
		endpointsPath: filepath.Join(walletDir, params.Name, "upstreams.json"),
		rpcURL:        strings.TrimRight(rpcURL, "/"),
	}
	s.bootstrapUpstreams()
	return s
}

func (s *Service) WalletPath() string {
	return s.walletPath
}

func (s *Service) BackupDir() string {
	return filepath.Join(filepath.Dir(s.walletPath), "wallet-backups")
}

func (s *Service) UpstreamsPath() string {
	return s.endpointsPath
}

func (s *Service) RPCURL() string {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.rpcURL
}

func (s *Service) Overview() (Overview, error) {
	s.mu.Lock()
	w, summary, err := s.loadWalletSummaryLocked()
	rpcURL := s.rpcURL
	s.mu.Unlock()
	node := s.probeNode(rpcURL)
	if err != nil {
		if errors.Is(err, ErrWalletNotFound) {
			return Overview{
				Wallet: WalletSummary{
					Exists:      false,
					Path:        s.walletPath,
					BackupDir:   s.BackupDir(),
					Network:     s.params.Name,
					AddressHint: s.params.AddressPrefix + "...",
				},
				RPCURL:   s.rpcURL,
				Upstream: s.upstreamSettingsLocked(),
				Node:     node,
			}, nil
		}
		return Overview{}, err
	}
	view, err := walletcore.ScanWallet(s.params, w, s.rpcURL)
	if err != nil {
		node.Error = err.Error()
		return Overview{
			Wallet:   summary,
			RPCURL:   s.rpcURL,
			Upstream: s.upstreamSettingsLocked(),
			Node:     node,
		}, nil
	}
	if node.BestHeight == 0 && view.Balance.BestHeight > 0 {
		node.Online = true
		node.Endpoint = rpcURL
		node.Network = s.params.Name
		node.BestHeight = view.Balance.BestHeight
		node.BestHash = view.Balance.BestHash
		node.ScannedAt = time.Now().UTC()
	}
	return Overview{
		Wallet:   summary,
		Balance:  view.Balance,
		History:  view.History,
		RPCURL:   s.rpcURL,
		Upstream: s.upstreamSettingsLocked(),
		Node:     node,
	}, nil
}

func (s *Service) CreateWallet(req CreateWalletRequest) (WalletSummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var (
		w   *walletcore.Wallet
		err error
	)
	if strings.TrimSpace(req.Passphrase) != "" {
		w, err = walletcore.CreateEncrypted(s.walletPath, s.params, req.Passphrase)
	} else {
		w, err = walletcore.Create(s.walletPath, s.params)
	}
	if err != nil {
		return WalletSummary{}, err
	}
	return summarizeWallet(s.params, s.walletPath, w, s.BackupDir()), nil
}

func (s *Service) CreateAddress(req CreateAddressRequest) (KeySummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	w, err := s.loadWalletLocked()
	if err != nil {
		return KeySummary{}, err
	}
	if err := w.AddKeyWithPassphrase(s.params, req.Label, req.Passphrase); err != nil {
		return KeySummary{}, err
	}
	if err := walletcore.Save(s.walletPath, w); err != nil {
		return KeySummary{}, err
	}
	return summarizeKey(w.Keys[len(w.Keys)-1]), nil
}

func (s *Service) EncryptWallet(req EncryptWalletRequest) (WalletSummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	passphrase := strings.TrimSpace(req.Passphrase)
	if passphrase == "" {
		return WalletSummary{}, fmt.Errorf("passphrase is required")
	}
	w, err := s.loadWalletLocked()
	if err != nil {
		return WalletSummary{}, err
	}
	if err := w.Encrypt(passphrase); err != nil {
		return WalletSummary{}, err
	}
	if err := walletcore.Save(s.walletPath, w); err != nil {
		return WalletSummary{}, err
	}
	return summarizeWallet(s.params, s.walletPath, w, s.BackupDir()), nil
}

func (s *Service) ChangePassphrase(req ChangePassphraseRequest) (WalletSummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	w, err := s.loadWalletLocked()
	if err != nil {
		return WalletSummary{}, err
	}
	if err := w.ChangePassphrase(strings.TrimSpace(req.OldPassphrase), strings.TrimSpace(req.NewPassphrase)); err != nil {
		return WalletSummary{}, err
	}
	if err := walletcore.Save(s.walletPath, w); err != nil {
		return WalletSummary{}, err
	}
	return summarizeWallet(s.params, s.walletPath, w, s.BackupDir()), nil
}

func (s *Service) ImportPrivateKey(req ImportPrivateKeyRequest) (KeySummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	w, err := s.loadWalletLocked()
	if err != nil {
		return KeySummary{}, err
	}
	key, err := w.ImportPrivateKey(s.params, req.Label, req.PrivKeyHex, req.Passphrase)
	if err != nil {
		return KeySummary{}, err
	}
	if err := walletcore.Save(s.walletPath, w); err != nil {
		return KeySummary{}, err
	}
	return summarizeKey(key), nil
}

func (s *Service) Send(req SendRequest) (SendResponse, error) {
	amount, err := walletcore.ParsePACAmount(req.Amount)
	if err != nil {
		return SendResponse{}, fmt.Errorf("amount: %w", err)
	}
	feeText := strings.TrimSpace(req.Fee)
	if feeText == "" {
		feeText = "0.0001"
	}
	fee, err := walletcore.ParsePACAmount(feeText)
	if err != nil {
		return SendResponse{}, fmt.Errorf("fee: %w", err)
	}

	s.mu.Lock()
	w, err := s.loadWalletLocked()
	s.mu.Unlock()
	if err != nil {
		return SendResponse{}, err
	}

	balance, err := walletcore.ScanBalance(s.params, w, s.rpcURL)
	if err != nil {
		return SendResponse{}, err
	}
	draft, err := walletcore.BuildDraftTx(s.params, w, balance, req.To, amount, fee, req.Change)
	if err != nil {
		return SendResponse{}, err
	}
	if err := walletcore.SignDraftTxWithPassphrase(s.params, w, draft, req.Passphrase); err != nil {
		return SendResponse{}, err
	}
	result, err := walletcore.SubmitRawTransaction(s.rpcURL, draft.Tx)
	if err != nil {
		return SendResponse{}, err
	}
	return SendResponse{
		Accepted:    result.Accepted,
		TxID:        result.TxID,
		MempoolSize: result.MempoolSize,
		InputTotal:  draft.InputTotal,
		OutputTotal: draft.OutputTotal,
		Fee:         draft.Fee,
		Change:      draft.Change,
		ChangeAddr:  draft.ChangeAddr,
		Destination: draft.Destination,
	}, nil
}

func (s *Service) WalletFile() ([]byte, string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if _, err := os.Stat(s.walletPath); err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return nil, "", ErrWalletNotFound
		}
		return nil, "", err
	}
	data, err := os.ReadFile(s.walletPath)
	if err != nil {
		return nil, "", err
	}
	return data, filepath.Base(s.walletPath), nil
}

func (s *Service) PubKeysFile() ([]byte, string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	w, err := s.loadWalletLocked()
	if err != nil {
		return nil, "", err
	}
	var b strings.Builder
	for _, key := range w.Keys {
		fmt.Fprintf(&b, "%s %s %s\n", strings.TrimSpace(key.Label), key.Address, key.PubKeyHex)
	}
	return []byte(b.String()), "pubkeys.txt", nil
}

func (s *Service) KeyByAddress(address string) (KeySummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	w, err := s.loadWalletLocked()
	if err != nil {
		return KeySummary{}, err
	}
	address = strings.TrimSpace(address)
	for _, key := range w.Keys {
		if key.Address == address {
			return summarizeKey(key), nil
		}
	}
	return KeySummary{}, fmt.Errorf("wallet address %q not found", address)
}

func (s *Service) RestoreWallet(req RestoreWalletRequest) (WalletSummary, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if len(bytes.TrimSpace(req.Data)) == 0 {
		return WalletSummary{}, fmt.Errorf("wallet restore data is empty")
	}
	if err := os.MkdirAll(filepath.Dir(s.walletPath), 0o700); err != nil {
		return WalletSummary{}, err
	}
	if _, err := os.Stat(s.walletPath); err == nil && !req.Overwrite {
		return WalletSummary{}, ErrWalletAlreadyExists
	} else if err != nil && !errors.Is(err, os.ErrNotExist) {
		return WalletSummary{}, err
	}

	w, normalized, err := s.validateWalletDataLocked(req.Data)
	if err != nil {
		return WalletSummary{}, err
	}

	if _, err := os.Stat(s.walletPath); err == nil {
		if _, err := s.snapshotExistingWalletLocked(); err != nil {
			return WalletSummary{}, err
		}
	}
	if err := os.WriteFile(s.walletPath, normalized, 0o600); err != nil {
		return WalletSummary{}, err
	}
	return summarizeWallet(s.params, s.walletPath, w, s.BackupDir()), nil
}

func (s *Service) BackupFile(name string) ([]byte, string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if name == "" {
		return nil, "", fmt.Errorf("backup name is required")
	}
	cleanName := filepath.Base(name)
	if cleanName != name {
		return nil, "", fmt.Errorf("invalid backup name")
	}
	path := filepath.Join(s.BackupDir(), cleanName)
	data, err := os.ReadFile(path)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return nil, "", ErrBackupNotFound
		}
		return nil, "", err
	}
	return data, cleanName, nil
}

func (s *Service) PreviewMultiSig(req MultiSigPreviewRequest) (MultiSigPreviewResult, error) {
	required := req.Required
	if required == 0 {
		required = 3
	}
	pubKeysHex := make([]string, 0, len(req.PubKeys))
	pubKeys := make([][]byte, 0, len(req.PubKeys))
	for _, value := range req.PubKeys {
		value = strings.TrimSpace(value)
		if value == "" {
			continue
		}
		pubKey, err := decodeHexString(value)
		if err != nil {
			return MultiSigPreviewResult{}, fmt.Errorf("pubkey %q: %w", value, err)
		}
		pubKeysHex = append(pubKeysHex, strings.ToLower(value))
		pubKeys = append(pubKeys, pubKey)
	}
	script, err := address.MultiSigRedeemScript(required, pubKeys)
	if err != nil {
		return MultiSigPreviewResult{}, err
	}
	addr, scriptHash, p2shScript, err := address.AddressFromScript(s.params, script)
	if err != nil {
		return MultiSigPreviewResult{}, err
	}
	return MultiSigPreviewResult{
		Required:     required,
		Participants: len(pubKeys),
		PubKeys:      pubKeysHex,
		Address:      addr,
		ScriptHash:   encodeHexString(scriptHash),
		RedeemScript: encodeHexString(script),
		P2SHScript:   encodeHexString(p2shScript),
	}, nil
}

func (s *Service) TransactionDetail(txHash string) (walletcore.TransactionDetail, error) {
	s.mu.Lock()
	w, err := s.loadWalletLocked()
	rpcURL := s.rpcURL
	s.mu.Unlock()
	if err != nil {
		return walletcore.TransactionDetail{}, err
	}
	return walletcore.FindTransaction(s.params, w, rpcURL, txHash)
}

func (s *Service) AddUpstream(req AddUpstreamRequest) (UpstreamSettings, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	url := normalizeRPCURL(req.URL)
	if url == "" {
		return UpstreamSettings{}, fmt.Errorf("upstream URL is required")
	}
	name := strings.TrimSpace(req.Name)
	if name == "" {
		name = url
	}
	cfg, err := s.loadUpstreamsLocked()
	if err != nil {
		return UpstreamSettings{}, err
	}
	id := profileIDFromName(name)
	if id == "" {
		id = profileIDFromName(url)
	}
	id = ensureUniqueProfileID(cfg.Profiles, id)
	cfg.Profiles = append(cfg.Profiles, UpstreamProfile{
		ID:     id,
		Name:   name,
		URL:    url,
		Source: "custom",
	})
	if req.MakeActive || cfg.ActiveID == "" {
		cfg.ActiveID = id
		s.rpcURL = url
	}
	if err := s.saveUpstreamsLocked(cfg); err != nil {
		return UpstreamSettings{}, err
	}
	return s.upstreamSettingsFromConfig(cfg), nil
}

func (s *Service) SelectUpstream(req SelectUpstreamRequest) (UpstreamSettings, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	cfg, err := s.loadUpstreamsLocked()
	if err != nil {
		return UpstreamSettings{}, err
	}
	id := strings.TrimSpace(req.ID)
	for i := range cfg.Profiles {
		if cfg.Profiles[i].ID != id {
			continue
		}
		cfg.ActiveID = id
		cfg.Profiles[i].LastUsedAt = time.Now().UTC()
		s.rpcURL = cfg.Profiles[i].URL
		if err := s.saveUpstreamsLocked(cfg); err != nil {
			return UpstreamSettings{}, err
		}
		return s.upstreamSettingsFromConfig(cfg), nil
	}
	return UpstreamSettings{}, fmt.Errorf("upstream profile %q not found", id)
}

func (s *Service) MergeUpstreamTemplate(path string) (MergeUpstreamTemplateResult, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	path = strings.TrimSpace(path)
	if path == "" {
		return MergeUpstreamTemplateResult{}, fmt.Errorf("upstream template path is required")
	}
	data, err := os.ReadFile(path)
	if err != nil {
		return MergeUpstreamTemplateResult{}, err
	}
	var template upstreamFile
	if err := json.Unmarshal(data, &template); err != nil {
		return MergeUpstreamTemplateResult{}, fmt.Errorf("upstream template %s: %w", path, err)
	}

	cfg, err := s.loadUpstreamsLocked()
	if err != nil {
		if !errors.Is(err, os.ErrNotExist) {
			return MergeUpstreamTemplateResult{}, err
		}
		cfg = upstreamFile{}
	}

	added := 0
	updated := 0
	for _, profile := range template.Profiles {
		normalized, ok := normalizeTemplateProfile(profile)
		if !ok {
			continue
		}
		match := findMatchingProfile(cfg.Profiles, normalized)
		if match < 0 {
			cfg.Profiles = append(cfg.Profiles, normalized)
			added++
			continue
		}
		if mergeTemplateProfile(&cfg.Profiles[match], normalized) {
			updated++
		}
	}

	if cfg.ActiveID == "" {
		cfg.ActiveID = strings.TrimSpace(template.ActiveID)
	}
	if cfg.ActiveID == "" && len(cfg.Profiles) > 0 {
		cfg.ActiveID = cfg.Profiles[0].ID
	}
	if cfg.ActiveID != "" {
		for _, profile := range cfg.Profiles {
			if profile.ID == cfg.ActiveID && profile.URL != "" {
				s.rpcURL = profile.URL
				break
			}
		}
	}
	if err := s.saveUpstreamsLocked(cfg); err != nil {
		return MergeUpstreamTemplateResult{}, err
	}
	return MergeUpstreamTemplateResult{
		Settings: s.upstreamSettingsFromConfig(cfg),
		Added:    added,
		Updated:  updated,
	}, nil
}

func (s *Service) loadWalletLocked() (*walletcore.Wallet, error) {
	if _, err := os.Stat(s.walletPath); err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return nil, ErrWalletNotFound
		}
		return nil, err
	}
	return walletcore.Load(s.walletPath)
}

func (s *Service) loadWalletSummaryLocked() (*walletcore.Wallet, WalletSummary, error) {
	w, err := s.loadWalletLocked()
	if err != nil {
		return nil, WalletSummary{}, err
	}
	return w, summarizeWallet(s.params, s.walletPath, w, s.BackupDir()), nil
}

func summarizeWallet(params *chaincfg.Params, walletPath string, w *walletcore.Wallet, backupDir string) WalletSummary {
	keys := make([]KeySummary, 0, len(w.Keys))
	for _, key := range w.Keys {
		keys = append(keys, summarizeKey(key))
	}
	sort.Slice(keys, func(i, j int) bool {
		if keys[i].CreatedAt.Equal(keys[j].CreatedAt) {
			return keys[i].Address < keys[j].Address
		}
		return keys[i].CreatedAt.Before(keys[j].CreatedAt)
	})
	return WalletSummary{
		Exists:      true,
		Path:        walletPath,
		BackupDir:   backupDir,
		Network:     w.Network,
		Encrypted:   w.IsEncrypted(),
		CreatedAt:   w.CreatedAt,
		KeyCount:    len(w.Keys),
		Keys:        keys,
		AddressHint: params.AddressPrefix + "...",
		Backups:     listBackupInfo(backupDir),
	}
}

func summarizeKey(key walletcore.Key) KeySummary {
	return KeySummary{
		Label:     key.Label,
		Address:   key.Address,
		PubKeyHex: key.PubKeyHex,
		CreatedAt: key.CreatedAt,
	}
}

func (s *Service) bootstrapUpstreams() {
	s.mu.Lock()
	defer s.mu.Unlock()

	cfg, err := s.loadUpstreamsLocked()
	if err == nil && len(cfg.Profiles) > 0 {
		for _, profile := range cfg.Profiles {
			if profile.ID == cfg.ActiveID && profile.URL != "" {
				s.rpcURL = normalizeRPCURL(profile.URL)
				return
			}
		}
	}
	defaultURL := normalizeRPCURL(s.rpcURL)
	if defaultURL == "" {
		defaultURL = "http://127.0.0.1:9509"
	}
	cfg = upstreamFile{
		ActiveID: "local-node",
		Profiles: []UpstreamProfile{{
			ID:     "local-node",
			Name:   "Local Node",
			URL:    defaultURL,
			Source: "local",
		}},
	}
	s.rpcURL = defaultURL
	_ = s.saveUpstreamsLocked(cfg)
}

func (s *Service) loadUpstreamsLocked() (upstreamFile, error) {
	data, err := os.ReadFile(s.endpointsPath)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return upstreamFile{}, os.ErrNotExist
		}
		return upstreamFile{}, err
	}
	var cfg upstreamFile
	if err := json.Unmarshal(data, &cfg); err != nil {
		return upstreamFile{}, err
	}
	for i := range cfg.Profiles {
		cfg.Profiles[i].URL = normalizeRPCURL(cfg.Profiles[i].URL)
	}
	return cfg, nil
}

func (s *Service) saveUpstreamsLocked(cfg upstreamFile) error {
	if err := os.MkdirAll(filepath.Dir(s.endpointsPath), 0o700); err != nil {
		return err
	}
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(s.endpointsPath, append(data, '\n'), 0o600)
}

func (s *Service) upstreamSettingsLocked() UpstreamSettings {
	cfg, err := s.loadUpstreamsLocked()
	if err != nil {
		return UpstreamSettings{
			ConfigPath: s.endpointsPath,
			ActiveURL:  s.rpcURL,
		}
	}
	return s.upstreamSettingsFromConfig(cfg)
}

func (s *Service) upstreamSettingsFromConfig(cfg upstreamFile) UpstreamSettings {
	profiles := append([]UpstreamProfile(nil), cfg.Profiles...)
	sort.Slice(profiles, func(i, j int) bool {
		return profiles[i].Name < profiles[j].Name
	})
	activeURL := s.rpcURL
	for _, profile := range cfg.Profiles {
		if profile.ID == cfg.ActiveID {
			activeURL = profile.URL
			break
		}
	}
	return UpstreamSettings{
		ConfigPath: s.endpointsPath,
		ActiveID:   cfg.ActiveID,
		ActiveURL:  activeURL,
		Profiles:   profiles,
	}
}

func (s *Service) validateWalletDataLocked(data []byte) (*walletcore.Wallet, []byte, error) {
	var w walletcore.Wallet
	if err := json.Unmarshal(data, &w); err != nil {
		return nil, nil, fmt.Errorf("wallet restore parse: %w", err)
	}
	if w.Network != s.params.Name {
		return nil, nil, fmt.Errorf("wallet restore network %q does not match %q", w.Network, s.params.Name)
	}
	if len(w.Keys) == 0 {
		return nil, nil, fmt.Errorf("wallet restore contains no keys")
	}
	normalized, err := json.MarshalIndent(&w, "", "  ")
	if err != nil {
		return nil, nil, err
	}
	return &w, append(normalized, '\n'), nil
}

func (s *Service) snapshotExistingWalletLocked() (BackupInfo, error) {
	data, err := os.ReadFile(s.walletPath)
	if err != nil {
		return BackupInfo{}, err
	}
	if err := os.MkdirAll(s.BackupDir(), 0o700); err != nil {
		return BackupInfo{}, err
	}
	name := "wallet-" + time.Now().UTC().Format("20060102T150405Z") + ".json"
	path := filepath.Join(s.BackupDir(), name)
	if err := os.WriteFile(path, data, 0o600); err != nil {
		return BackupInfo{}, err
	}
	info, err := os.Stat(path)
	if err != nil {
		return BackupInfo{}, err
	}
	return BackupInfo{
		Name:      name,
		Path:      path,
		SizeBytes: info.Size(),
		CreatedAt: info.ModTime().UTC(),
	}, nil
}

func listBackupInfo(backupDir string) []BackupInfo {
	entries, err := os.ReadDir(backupDir)
	if err != nil {
		return nil
	}
	backups := make([]BackupInfo, 0, len(entries))
	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}
		info, err := entry.Info()
		if err != nil {
			continue
		}
		backups = append(backups, BackupInfo{
			Name:      entry.Name(),
			Path:      filepath.Join(backupDir, entry.Name()),
			SizeBytes: info.Size(),
			CreatedAt: info.ModTime().UTC(),
		})
	}
	sort.Slice(backups, func(i, j int) bool {
		if backups[i].CreatedAt.Equal(backups[j].CreatedAt) {
			return backups[i].Name > backups[j].Name
		}
		return backups[i].CreatedAt.After(backups[j].CreatedAt)
	})
	return backups
}

func normalizeRPCURL(value string) string {
	value = strings.TrimSpace(strings.TrimRight(value, "/"))
	if value == "" {
		return ""
	}
	if !strings.Contains(value, "://") {
		value = "http://" + value
	}
	return value
}

func profileIDFromName(value string) string {
	value = strings.ToLower(strings.TrimSpace(value))
	if value == "" {
		return ""
	}
	var b strings.Builder
	lastDash := false
	for _, r := range value {
		switch {
		case r >= 'a' && r <= 'z', r >= '0' && r <= '9':
			b.WriteRune(r)
			lastDash = false
		default:
			if !lastDash {
				b.WriteByte('-')
				lastDash = true
			}
		}
	}
	return strings.Trim(b.String(), "-")
}

func ensureUniqueProfileID(existing []UpstreamProfile, id string) string {
	if id == "" {
		id = "upstream"
	}
	candidate := id
	seq := 2
	for {
		conflict := false
		for _, profile := range existing {
			if profile.ID == candidate {
				conflict = true
				break
			}
		}
		if !conflict {
			return candidate
		}
		candidate = fmt.Sprintf("%s-%d", id, seq)
		seq++
	}
}

func normalizeTemplateProfile(profile UpstreamProfile) (UpstreamProfile, bool) {
	profile.ID = strings.TrimSpace(profile.ID)
	profile.Name = strings.TrimSpace(profile.Name)
	profile.URL = normalizeRPCURL(profile.URL)
	profile.Source = strings.TrimSpace(profile.Source)
	if profile.ID == "" {
		profile.ID = profileIDFromName(profile.Name)
	}
	if profile.ID == "" {
		profile.ID = profileIDFromName(profile.URL)
	}
	if profile.Name == "" {
		profile.Name = profile.URL
	}
	if profile.Source == "" {
		profile.Source = "template"
	}
	if profile.ID == "" || profile.URL == "" {
		return UpstreamProfile{}, false
	}
	return profile, true
}

func findMatchingProfile(existing []UpstreamProfile, candidate UpstreamProfile) int {
	for i, profile := range existing {
		if profile.ID == candidate.ID {
			return i
		}
	}
	for i, profile := range existing {
		if normalizeRPCURL(profile.URL) == candidate.URL {
			return i
		}
	}
	return -1
}

func mergeTemplateProfile(existing *UpstreamProfile, candidate UpstreamProfile) bool {
	changed := false
	if strings.TrimSpace(existing.Name) == "" && candidate.Name != "" {
		existing.Name = candidate.Name
		changed = true
	}
	if normalizeRPCURL(existing.URL) == "" && candidate.URL != "" {
		existing.URL = candidate.URL
		changed = true
	}
	if strings.TrimSpace(existing.Source) == "" && candidate.Source != "" {
		existing.Source = candidate.Source
		changed = true
	}
	if strings.TrimSpace(existing.ID) == "" && candidate.ID != "" {
		existing.ID = candidate.ID
		changed = true
	}
	return changed
}

type upstreamNetworkInfo struct {
	Network     string `json:"network"`
	BestHeight  uint32 `json:"bestheight"`
	BestHash    string `json:"bestblockhash"`
	MempoolSize int    `json:"mempoolsize"`
	PeerCount   int    `json:"peercount"`
}

type chainTip struct {
	Height uint32 `json:"height"`
	Hash   string `json:"hash"`
}

type rpcMempool struct {
	Size int `json:"size"`
}

func (s *Service) probeNode(rpcURL string) NodeStatus {
	node := NodeStatus{
		Endpoint:  rpcURL,
		Network:   s.params.Name,
		ScannedAt: time.Now().UTC(),
	}
	rpcURL = normalizeRPCURL(rpcURL)
	if rpcURL == "" {
		node.Error = "upstream RPC URL is empty"
		return node
	}
	var networkInfo upstreamNetworkInfo
	if err := fetchJSON(rpcURL+"/getnetworkinfo", &networkInfo); err == nil {
		node.Online = true
		node.Network = networkInfo.Network
		node.BestHeight = networkInfo.BestHeight
		node.BestHash = networkInfo.BestHash
		node.MempoolSize = networkInfo.MempoolSize
		node.PeerCount = networkInfo.PeerCount
		return node
	}

	var tip chainTip
	if err := fetchJSON(rpcURL+"/getbestblock", &tip); err != nil {
		node.Error = err.Error()
		return node
	}
	node.Online = true
	node.BestHeight = tip.Height
	node.BestHash = tip.Hash

	var mempool rpcMempool
	if err := fetchJSON(rpcURL+"/getrawmempool", &mempool); err == nil {
		node.MempoolSize = mempool.Size
	}
	return node
}

func fetchJSON(url string, dest any) error {
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("%s returned %s", url, resp.Status)
	}
	return json.NewDecoder(resp.Body).Decode(dest)
}

func decodeHexString(value string) ([]byte, error) {
	value = strings.TrimSpace(value)
	if value == "" {
		return nil, fmt.Errorf("empty hex string")
	}
	return hex.DecodeString(value)
}

func encodeHexString(value []byte) string {
	return hex.EncodeToString(value)
}
