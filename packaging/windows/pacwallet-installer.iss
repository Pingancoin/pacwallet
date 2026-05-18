#define MyAppName "Pingancoin Wallet"
#ifndef MyAppVersion
  #define MyAppVersion "0.1.0-dev"
#endif
#ifndef SourceReleaseDir
  #define SourceReleaseDir "..\\..\\dist\\pacwallet-windows-amd64"
#endif

[Setup]
AppId={{D6A2EA8F-5A25-4D42-A5C1-D8F2E6B1A031}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Pingancoin
DefaultDirName={autopf}\Pingancoin Wallet
DefaultGroupName=Pingancoin Wallet
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
PrivilegesRequired=lowest
SetupIconFile={#SourceReleaseDir}\branding\pingancoin-icon.ico
OutputDir={#SourceReleaseDir}
OutputBaseFilename=pingancoin-wallet-setup-{#MyAppVersion}
UninstallDisplayIcon={app}\branding\pingancoin-icon.ico

[Files]
Source: "{#SourceReleaseDir}\pacwallet.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\pacwallet-desktop.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\pacwallet-desktop.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\upstreams.mainnet.template.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\release.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\WINDOWS_RELEASE_NOTES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\branding\*"; DestDir: "{app}\branding"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Pingancoin Wallet"; Filename: "{app}\pacwallet-desktop.exe"; IconFilename: "{app}\branding\pingancoin-icon.ico"
Name: "{group}\Pingancoin Wallet (Web Service)"; Filename: "{app}\pacwallet.exe"; Parameters: "serve --network mainnet --rpc http://127.0.0.1:9509 --listen 127.0.0.1:19709"; IconFilename: "{app}\branding\pingancoin-icon.ico"
Name: "{userdesktop}\Pingancoin Wallet"; Filename: "{app}\pacwallet-desktop.exe"; IconFilename: "{app}\branding\pingancoin-icon.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\pacwallet-desktop.exe"; Description: "Launch Pingancoin Wallet"; Flags: nowait postinstall skipifsilent
