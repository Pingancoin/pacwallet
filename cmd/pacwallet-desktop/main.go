package main

import (
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	"github.com/Pingancoin/pacwallet/internal/buildinfo"
	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/service"
	"github.com/Pingancoin/pacwallet/internal/wallet"
	"github.com/Pingancoin/pacwallet/internal/web"
)

const defaultDesktopTitle = "Pingancoin Wallet"

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "pacwallet-desktop:", err)
		os.Exit(1)
	}
}

func run() error {
	flags := flag.NewFlagSet("pacwallet-desktop", flag.ContinueOnError)
	flags.SetOutput(os.Stderr)
	network := flags.String("network", "simnet", "network to use: mainnet, testnet, simnet")
	walletDir := flags.String("walletdir", wallet.DefaultDir(), "base wallet directory")
	rpcURL := flags.String("rpc", "http://127.0.0.1:9509", "pacd RPC URL")
	listen := flags.String("listen", "127.0.0.1:0", "desktop wallet service listen address")
	browser := flags.String("browser", "auto", "launcher preference: auto, edge, chrome, system, none")
	title := flags.String("title", defaultDesktopTitle, "desktop window title used by browser app launchers")
	configPath := flags.String("config", "", "optional desktop config JSON path")
	upstreamsTemplate := flags.String("upstreamstemplate", "", "optional upstream template JSON path")
	showVersion := flags.Bool("version", false, "print desktop wallet version and exit")
	if err := flags.Parse(os.Args[1:]); err != nil {
		return err
	}
	if *showVersion {
		fmt.Println(buildinfo.Summary())
		return nil
	}

	explicit := visitedFlagSet(flags)
	cfg, loadedPath, err := loadDesktopConfig(*configPath)
	if err != nil {
		return err
	}
	applyDesktopConfig(explicit, cfg, network, walletDir, rpcURL, listen, browser, title, upstreamsTemplate)

	params, err := selectParams(*network)
	if err != nil {
		return err
	}
	svc := service.New(params, *walletDir, *rpcURL)
	templatePath := resolveUpstreamsTemplatePath(*upstreamsTemplate, cfg.UpstreamsTemplate, loadedPath, *network)
	if templatePath != "" {
		mergeResult, err := svc.MergeUpstreamTemplate(templatePath)
		if err != nil {
			return err
		}
		if mergeResult.Added > 0 || mergeResult.Updated > 0 {
			fmt.Printf("imported upstream presets from %s (%d added, %d updated)\n", templatePath, mergeResult.Added, mergeResult.Updated)
		}
	}
	server, err := web.New(svc)
	if err != nil {
		return err
	}

	listener, err := net.Listen("tcp", *listen)
	if err != nil {
		return err
	}
	defer listener.Close()

	httpServer := &http.Server{
		Handler:           server.Handler(),
		ReadHeaderTimeout: 5 * time.Second,
	}
	go func() {
		_ = httpServer.Serve(listener)
	}()

	appURL := "http://" + listener.Addr().String()
	fmt.Printf("desktop wallet serving %s\n", appURL)
	fmt.Printf("wallet file: %s\n", svc.WalletPath())
	fmt.Printf("upstream pacd: %s\n", svc.RPCURL())
	if loadedPath != "" {
		fmt.Printf("desktop config: %s\n", loadedPath)
	}
	fmt.Printf("%s\n", buildinfo.Summary())

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	if *browser != "none" {
		cmd, err := launchBrowserApp(appURL, *browser, *title)
		if err != nil {
			fmt.Printf("launcher fallback: %v\n", err)
			fmt.Printf("open this URL manually: %s\n", appURL)
		} else if cmd != nil && cmd.Process != nil {
			go func() {
				_ = cmd.Wait()
				stop()
			}()
		}
	}

	<-ctx.Done()
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	return httpServer.Shutdown(shutdownCtx)
}

func launchBrowserApp(url string, browser string, title string) (*exec.Cmd, error) {
	switch runtime.GOOS {
	case "windows":
		return launchWindowsBrowserApp(url, browser, title)
	case "darwin":
		return launchDarwinBrowserApp(url, browser, title)
	default:
		return launchUnixBrowserApp(url, browser, title)
	}
}

func launchWindowsBrowserApp(url string, browser string, title string) (*exec.Cmd, error) {
	commands := browserLaunchCandidates(browser, url, title)
	for _, candidate := range commands {
		if len(candidate) == 0 {
			continue
		}
		path, args, err := resolveWindowsCommand(candidate)
		if err != nil {
			continue
		}
		cmd := exec.Command(path, args...)
		if err := cmd.Start(); err == nil {
			return cmd, nil
		}
	}
	return nil, errors.New("no supported Windows browser launcher found")
}

func launchDarwinBrowserApp(url string, browser string, title string) (*exec.Cmd, error) {
	app := "Microsoft Edge"
	if browser == "chrome" {
		app = "Google Chrome"
	}
	if browser == "system" {
		cmd := exec.Command("open", url)
		if err := cmd.Start(); err != nil {
			return nil, err
		}
		return nil, nil
	}
	cmd := exec.Command("open", "-a", app, "--args", "--app="+url, "--new-window", "--app-name="+title)
	if err := cmd.Start(); err == nil {
		return nil, nil
	}
	fallback := exec.Command("open", url)
	if err := fallback.Start(); err != nil {
		return nil, err
	}
	return nil, nil
}

func launchUnixBrowserApp(url string, browser string, title string) (*exec.Cmd, error) {
	candidates := [][]string{}
	switch browser {
	case "chrome":
		candidates = append(candidates, []string{"google-chrome", "--app=" + url, "--new-window", "--app-name=" + title})
	case "edge":
		candidates = append(candidates, []string{"microsoft-edge", "--app=" + url, "--new-window", "--app-name=" + title})
	case "system":
		candidates = append(candidates, []string{"xdg-open", url})
	default:
		candidates = append(candidates,
			[]string{"microsoft-edge", "--app=" + url, "--new-window", "--app-name=" + title},
			[]string{"google-chrome", "--app=" + url, "--new-window", "--app-name=" + title},
			[]string{"xdg-open", url},
		)
	}
	for _, candidate := range candidates {
		path, err := exec.LookPath(candidate[0])
		if err != nil {
			continue
		}
		cmd := exec.Command(path, candidate[1:]...)
		if err := cmd.Start(); err == nil {
			return cmd, nil
		}
	}
	return nil, errors.New("no supported browser launcher found")
}

func browserLaunchCandidates(browser string, url string, title string) [][]string {
	edgePaths := []string{
		filepath.Join(os.Getenv("ProgramFiles"), "Microsoft", "Edge", "Application", "msedge.exe"),
		filepath.Join(os.Getenv("ProgramFiles(x86)"), "Microsoft", "Edge", "Application", "msedge.exe"),
		filepath.Join(os.Getenv("LocalAppData"), "Microsoft", "Edge", "Application", "msedge.exe"),
		"msedge.exe",
	}
	chromePaths := []string{
		filepath.Join(os.Getenv("ProgramFiles"), "Google", "Chrome", "Application", "chrome.exe"),
		filepath.Join(os.Getenv("ProgramFiles(x86)"), "Google", "Chrome", "Application", "chrome.exe"),
		filepath.Join(os.Getenv("LocalAppData"), "Google", "Chrome", "Application", "chrome.exe"),
		"chrome.exe",
	}

	build := func(paths []string) [][]string {
		result := make([][]string, 0, len(paths))
		for _, path := range paths {
			result = append(result, []string{path, "--app=" + url, "--new-window", "--app-name=" + title})
		}
		return result
	}

	switch browser {
	case "edge":
		return build(edgePaths)
	case "chrome":
		return build(chromePaths)
	case "system":
		return [][]string{{"rundll32.exe", "url.dll,FileProtocolHandler", url}}
	default:
		result := build(edgePaths)
		result = append(result, build(chromePaths)...)
		result = append(result, []string{"rundll32.exe", "url.dll,FileProtocolHandler", url})
		return result
	}
}

type desktopConfig struct {
	Network           string `json:"network"`
	WalletDir         string `json:"wallet_dir"`
	RPCURL            string `json:"rpc_url"`
	Listen            string `json:"listen"`
	Browser           string `json:"browser"`
	Title             string `json:"title"`
	UpstreamsTemplate string `json:"upstreams_template"`
}

func loadDesktopConfig(explicitPath string) (desktopConfig, string, error) {
	candidates := []string{}
	if strings.TrimSpace(explicitPath) != "" {
		candidates = append(candidates, explicitPath)
	} else {
		if exe, err := os.Executable(); err == nil {
			candidates = append(candidates, filepath.Join(filepath.Dir(exe), "pacwallet-desktop.json"))
		}
		if cwd, err := os.Getwd(); err == nil {
			candidates = append(candidates, filepath.Join(cwd, "pacwallet-desktop.json"))
		}
	}
	seen := map[string]struct{}{}
	for _, candidate := range candidates {
		if candidate == "" {
			continue
		}
		if _, ok := seen[candidate]; ok {
			continue
		}
		seen[candidate] = struct{}{}
		data, err := os.ReadFile(candidate)
		if err != nil {
			if errors.Is(err, os.ErrNotExist) {
				continue
			}
			return desktopConfig{}, "", err
		}
		var cfg desktopConfig
		if err := json.Unmarshal(data, &cfg); err != nil {
			return desktopConfig{}, "", fmt.Errorf("desktop config %s: %w", candidate, err)
		}
		return cfg, candidate, nil
	}
	return desktopConfig{}, "", nil
}

func applyDesktopConfig(explicit map[string]bool, cfg desktopConfig, network, walletDir, rpcURL, listen, browser, title, upstreamsTemplate *string) {
	if !explicit["network"] && strings.TrimSpace(cfg.Network) != "" {
		*network = strings.TrimSpace(cfg.Network)
	}
	if !explicit["walletdir"] && strings.TrimSpace(cfg.WalletDir) != "" {
		*walletDir = strings.TrimSpace(cfg.WalletDir)
	}
	if !explicit["rpc"] && strings.TrimSpace(cfg.RPCURL) != "" {
		*rpcURL = strings.TrimSpace(cfg.RPCURL)
	}
	if !explicit["listen"] && strings.TrimSpace(cfg.Listen) != "" {
		*listen = strings.TrimSpace(cfg.Listen)
	}
	if !explicit["browser"] && strings.TrimSpace(cfg.Browser) != "" {
		*browser = strings.TrimSpace(cfg.Browser)
	}
	if !explicit["title"] && strings.TrimSpace(cfg.Title) != "" {
		*title = strings.TrimSpace(cfg.Title)
	}
	if !explicit["upstreamstemplate"] && strings.TrimSpace(cfg.UpstreamsTemplate) != "" {
		*upstreamsTemplate = strings.TrimSpace(cfg.UpstreamsTemplate)
	}
}

func resolveUpstreamsTemplatePath(explicitPath string, configPath string, loadedConfigPath string, network string) string {
	candidates := []string{}
	add := func(base string, anchorDir string, includeSelf bool) {
		base = strings.TrimSpace(base)
		if base == "" {
			return
		}
		if anchorDir != "" && !filepath.IsAbs(base) {
			base = filepath.Join(anchorDir, base)
		}
		if includeSelf {
			candidates = append(candidates, base)
		}
		if info, err := os.Stat(base); err == nil && info.IsDir() {
			candidates = append(candidates,
				filepath.Join(base, "upstreams."+network+".template.json"),
				filepath.Join(base, "upstreams.template.json"),
				filepath.Join(base, "upstreams.mainnet.template.json"),
			)
			return
		}
		if strings.HasSuffix(strings.ToLower(base), ".json") {
			dir := filepath.Dir(base)
			candidates = append(candidates,
				filepath.Join(dir, "upstreams."+network+".template.json"),
				filepath.Join(dir, "upstreams.template.json"),
				filepath.Join(dir, "upstreams.mainnet.template.json"),
			)
		}
	}

	add(explicitPath, "", true)
	add(configPath, filepath.Dir(loadedConfigPath), true)
	add(loadedConfigPath, "", false)
	if exe, err := os.Executable(); err == nil {
		add(filepath.Dir(exe), "", false)
	}
	if cwd, err := os.Getwd(); err == nil {
		add(cwd, "", false)
	}

	seen := map[string]struct{}{}
	for _, candidate := range candidates {
		candidate = strings.TrimSpace(candidate)
		if candidate == "" {
			continue
		}
		resolved := candidate
		if !filepath.IsAbs(resolved) {
			resolved = filepath.Clean(resolved)
		}
		if _, ok := seen[resolved]; ok {
			continue
		}
		seen[resolved] = struct{}{}
		info, err := os.Stat(resolved)
		if err != nil || info.IsDir() {
			continue
		}
		return resolved
	}
	return ""
}

func visitedFlagSet(fs *flag.FlagSet) map[string]bool {
	out := map[string]bool{}
	fs.Visit(func(f *flag.Flag) {
		out[f.Name] = true
	})
	return out
}

func resolveWindowsCommand(candidate []string) (string, []string, error) {
	if filepath.IsAbs(candidate[0]) {
		if _, err := os.Stat(candidate[0]); err != nil {
			return "", nil, err
		}
		return candidate[0], candidate[1:], nil
	}
	path, err := exec.LookPath(candidate[0])
	if err != nil {
		return "", nil, err
	}
	return path, candidate[1:], nil
}

func selectParams(network string) (*chaincfg.Params, error) {
	switch network {
	case "mainnet":
		return chaincfg.MainNetParams(), nil
	case "testnet":
		return chaincfg.TestNetParams(), nil
	case "simnet":
		return chaincfg.SimNetParams(), nil
	default:
		return nil, fmt.Errorf("unknown network %q", network)
	}
}
