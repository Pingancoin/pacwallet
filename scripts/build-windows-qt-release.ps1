param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$QtRoot = "C:\Qt",
    [string]$QtVersion = "6.8.3",
    [string]$QtArch = "msvc2022_64",
    [string]$OutDir = "",
    [string]$Version = "0.3.0-dev",
    [string]$BackendReleaseDir = ""
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if ($OutDir -eq "") {
    $OutDir = Join-Path $Root "dist\pacwallet-windows-qt-amd64"
}

$commit = (git -C $Root rev-parse --short HEAD).Trim()
$buildTime = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
$ldflags = "-X github.com/Pingancoin/pacwallet/internal/buildinfo.Version=$Version -X github.com/Pingancoin/pacwallet/internal/buildinfo.Commit=$commit -X github.com/Pingancoin/pacwallet/internal/buildinfo.BuildTime=$buildTime"

$qtBin = Join-Path $QtRoot "$QtVersion\$QtArch\bin"
$qtCmake = Join-Path $qtBin "qt-cmake.bat"
$windeployqt = Join-Path $qtBin "windeployqt.exe"
$vsDevCmd = "C:\BuildTools\Common7\Tools\VsDevCmd.bat"

if (-not (Test-Path $qtCmake)) { throw "qt-cmake not found at $qtCmake" }
if (-not (Test-Path $windeployqt)) { throw "windeployqt not found at $windeployqt" }
if (-not (Test-Path $vsDevCmd)) { throw "VsDevCmd.bat not found at $vsDevCmd" }

$buildDir = Join-Path $Root "qt\build-win-release"
Remove-Item -Recurse -Force $OutDir,$buildDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $OutDir | Out-Null

function Invoke-VsCommand {
    param([Parameter(Mandatory = $true)][string]$Command)
    $full = "`"$vsDevCmd`" -arch=amd64 && $Command"
    cmd /c $full
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $Command"
    }
}

Push-Location $Root
try {
    if ($BackendReleaseDir -ne "") {
        Write-Host "Copying backend release assets from $BackendReleaseDir ..."
        Copy-Item (Join-Path $BackendReleaseDir "pacwallet.exe") (Join-Path $OutDir "pacwallet.exe")
        foreach ($name in @("pacwallet-desktop.exe", "pacwallet-desktop.json", "upstreams.mainnet.template.json", "WINDOWS_RELEASE_NOTES.txt")) {
            $source = Join-Path $BackendReleaseDir $name
            if (Test-Path $source) {
                try {
                    Copy-Item $source (Join-Path $OutDir $name)
                }
                catch {
                    Write-Warning "Skipping optional asset ${name}: $($_.Exception.Message)"
                }
            }
        }
    } else {
        Write-Host "Building Go backend..."
        $env:GOOS = "windows"
        $env:GOARCH = "amd64"
        go build -ldflags $ldflags -o (Join-Path $OutDir "pacwallet.exe") ./cmd/pacwallet
        if ($LASTEXITCODE -ne 0) { throw "go build pacwallet failed" }
    }

    Write-Host "Configuring Qt frontend..."
    Invoke-VsCommand "`"$qtCmake`" -S qt -B `"$buildDir`" -G Ninja -DCMAKE_BUILD_TYPE=Release"

    Write-Host "Building Qt frontend..."
    Invoke-VsCommand "cmake --build `"$buildDir`" -j4"

    $qtExe = Join-Path $buildDir "pacwallet-qt.exe"
    if (-not (Test-Path $qtExe)) {
        throw "missing pacwallet-qt.exe at $qtExe"
    }
    Copy-Item $qtExe (Join-Path $OutDir "pacwallet-qt.exe")
    & $windeployqt --release --compiler-runtime (Join-Path $OutDir "pacwallet-qt.exe")
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

    Copy-Item (Join-Path $Root "README.md") (Join-Path $OutDir "README.md")
    Copy-Item (Join-Path $Root "packaging\windows\pacwallet-installer.iss") (Join-Path $OutDir "pacwallet-installer.iss")
    New-Item -ItemType Directory -Path (Join-Path $OutDir "branding") | Out-Null
    Copy-Item (Join-Path $Root "assets\branding\pingancoin\*") (Join-Path $OutDir "branding") -Force

    @"
{
  "product": "Pingancoin Wallet Qt",
  "version": "$Version",
  "commit": "$commit",
  "build_time": "$buildTime",
  "platform": "windows-qt-amd64",
  "artifacts": [
    "pacwallet.exe",
    "pacwallet-qt.exe",
    "branding/",
    "README.md",
    "pacwallet-installer.iss"
  ]
}
"@ | Set-Content -Path (Join-Path $OutDir "release.json") -Encoding ascii

    $zipPath = "$OutDir.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path $OutDir -DestinationPath $zipPath -Force

    Write-Host "Windows Qt release complete:"
    Write-Host "  $OutDir"
    Write-Host "  $zipPath"
}
finally {
    Pop-Location
}
