package service

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	walletcore "github.com/Pingancoin/pacwallet/internal/wallet"
)

var ErrWalletNotFound = errors.New("wallet not found")

type Service struct {
	params     *chaincfg.Params
	walletPath string
	rpcURL     string

	mu sync.Mutex
}

type Overview struct {
	Wallet  WalletSummary             `json:"wallet"`
	Balance walletcore.Balance        `json:"balance"`
	History []walletcore.HistoryEntry `json:"history"`
	RPCURL  string                    `json:"rpc_url"`
}

type WalletSummary struct {
	Exists      bool         `json:"exists"`
	Path        string       `json:"path"`
	Network     string       `json:"network"`
	Encrypted   bool         `json:"encrypted"`
	CreatedAt   time.Time    `json:"created_at"`
	KeyCount    int          `json:"key_count"`
	Keys        []KeySummary `json:"keys"`
	AddressHint string       `json:"address_hint"`
}

type KeySummary struct {
	Label     string    `json:"label"`
	Address   string    `json:"address"`
	CreatedAt time.Time `json:"created_at"`
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

func New(params *chaincfg.Params, walletDir string, rpcURL string) *Service {
	return &Service{
		params:     params,
		walletPath: walletcore.Path(walletDir, params.Name),
		rpcURL:     strings.TrimRight(rpcURL, "/"),
	}
}

func (s *Service) WalletPath() string {
	return s.walletPath
}

func (s *Service) Overview() (Overview, error) {
	s.mu.Lock()
	w, summary, err := s.loadWalletSummaryLocked()
	s.mu.Unlock()
	if err != nil {
		if errors.Is(err, ErrWalletNotFound) {
			return Overview{
				Wallet: WalletSummary{
					Exists:      false,
					Path:        s.walletPath,
					Network:     s.params.Name,
					AddressHint: s.params.AddressPrefix + "...",
				},
				RPCURL: s.rpcURL,
			}, nil
		}
		return Overview{}, err
	}
	view, err := walletcore.ScanWallet(s.params, w, s.rpcURL)
	if err != nil {
		return Overview{}, err
	}
	return Overview{
		Wallet:  summary,
		Balance: view.Balance,
		History: view.History,
		RPCURL:  s.rpcURL,
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
	return summarizeWallet(s.params, s.walletPath, w), nil
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
	return w, summarizeWallet(s.params, s.walletPath, w), nil
}

func summarizeWallet(params *chaincfg.Params, walletPath string, w *walletcore.Wallet) WalletSummary {
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
		Network:     w.Network,
		Encrypted:   w.IsEncrypted(),
		CreatedAt:   w.CreatedAt,
		KeyCount:    len(w.Keys),
		Keys:        keys,
		AddressHint: params.AddressPrefix + "...",
	}
}

func summarizeKey(key walletcore.Key) KeySummary {
	return KeySummary{
		Label:     key.Label,
		Address:   key.Address,
		CreatedAt: key.CreatedAt,
	}
}
