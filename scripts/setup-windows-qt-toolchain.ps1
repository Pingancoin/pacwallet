param(
    [string]$QtVersion = "6.8.3",
    [string]$QtArch = "win64_msvc2022_64",
    [string]$QtRoot = "C:\Qt"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Ensure-WingetPackage {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [string]$Override = ""
    )

    $args = @(
        "install",
        "--id", $Id,
        "-e",
        "--source", "winget",
        "--accept-source-agreements",
        "--accept-package-agreements",
        "--silent"
    )
    if ($Override -ne "") {
        $args += @("--override", $Override)
    }
    Write-Host "Installing $Id via winget..."
    & winget @args
}

Ensure-WingetPackage -Id "Git.Git"
Ensure-WingetPackage -Id "GoLang.Go"
Ensure-WingetPackage -Id "Kitware.CMake"
Ensure-WingetPackage -Id "Ninja-build.Ninja"
Ensure-WingetPackage -Id "Python.Python.3.12"
Ensure-WingetPackage -Id "Microsoft.VisualStudio.2022.BuildTools" -Override "--quiet --wait --norestart --nocache --installPath C:\BuildTools --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

$python = "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe"
if (-not (Test-Path $python)) {
    throw "Python not found at $python"
}

Write-Host "Installing aqtinstall..."
& $python -m pip install --user --upgrade pip aqtinstall

$userScripts = Join-Path $env:APPDATA "Python\Python312\Scripts"
$aqt = Join-Path $userScripts "aqt.exe"
if (-not (Test-Path $aqt)) {
    throw "aqt not found at $aqt"
}

if (-not (Test-Path $QtRoot)) {
    New-Item -ItemType Directory -Path $QtRoot -Force | Out-Null
}

Write-Host "Installing Qt $QtVersion ($QtArch) into $QtRoot ..."
& $aqt install-qt windows desktop $QtVersion $QtArch --outputdir $QtRoot

Write-Host "Toolchain setup complete."
Write-Host "Qt root: $QtRoot"
