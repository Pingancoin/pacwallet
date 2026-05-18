package main

import (
	"context"
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
	"time"

	"github.com/Pingancoin/pacwallet/internal/chaincfg"
	"github.com/Pingancoin/pacwallet/internal/service"
	"github.com/Pingancoin/pacwallet/internal/wallet"
	"github.com/Pingancoin/pacwallet/internal/web"
)

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
	if err := flags.Parse(os.Args[1:]); err != nil {
		return err
	}

	params, err := selectParams(*network)
	if err != nil {
		return err
	}
	svc := service.New(params, *walletDir, *rpcURL)
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
	fmt.Printf("upstream pacd: %s\n", *rpcURL)

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	if *browser != "none" {
		cmd, err := launchBrowserApp(appURL, *browser)
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

func launchBrowserApp(url string, browser string) (*exec.Cmd, error) {
	switch runtime.GOOS {
	case "windows":
		return launchWindowsBrowserApp(url, browser)
	case "darwin":
		return launchDarwinBrowserApp(url, browser)
	default:
		return launchUnixBrowserApp(url, browser)
	}
}

func launchWindowsBrowserApp(url string, browser string) (*exec.Cmd, error) {
	commands := browserLaunchCandidates(browser, url)
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

func launchDarwinBrowserApp(url string, browser string) (*exec.Cmd, error) {
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
	cmd := exec.Command("open", "-a", app, "--args", "--app="+url, "--new-window")
	if err := cmd.Start(); err == nil {
		return nil, nil
	}
	fallback := exec.Command("open", url)
	if err := fallback.Start(); err != nil {
		return nil, err
	}
	return nil, nil
}

func launchUnixBrowserApp(url string, browser string) (*exec.Cmd, error) {
	candidates := [][]string{}
	switch browser {
	case "chrome":
		candidates = append(candidates, []string{"google-chrome", "--app=" + url, "--new-window"})
	case "edge":
		candidates = append(candidates, []string{"microsoft-edge", "--app=" + url, "--new-window"})
	case "system":
		candidates = append(candidates, []string{"xdg-open", url})
	default:
		candidates = append(candidates,
			[]string{"microsoft-edge", "--app=" + url, "--new-window"},
			[]string{"google-chrome", "--app=" + url, "--new-window"},
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

func browserLaunchCandidates(browser string, url string) [][]string {
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
			result = append(result, []string{path, "--app=" + url, "--new-window"})
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
