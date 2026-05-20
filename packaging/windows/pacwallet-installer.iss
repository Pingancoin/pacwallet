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
DefaultDirName={localappdata}\Programs\Pingancoin Wallet
DefaultGroupName=Pingancoin Wallet
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DisableProgramGroupPage=yes
SetupIconFile={#SourceReleaseDir}\branding\pingancoin-icon.ico
OutputDir={#SourceReleaseDir}
OutputBaseFilename=pingancoin-wallet-setup-{#MyAppVersion}
UninstallDisplayIcon={app}\branding\pingancoin-icon.ico

[Dirs]
Name: "{userappdata}\Pingancoin Wallet"; Permissions: users-modify

[Files]
Source: "{#SourceReleaseDir}\pacwallet.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\pacwallet-qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\pacwallet-desktop.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\release.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\WINDOWS_RELEASE_NOTES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\branding\*"; DestDir: "{app}\branding"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceReleaseDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceReleaseDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('{#SourceReleaseDir}\platforms'))
Source: "{#SourceReleaseDir}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('{#SourceReleaseDir}\styles'))
Source: "{#SourceReleaseDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant('{#SourceReleaseDir}\imageformats'))
Source: "{#SourceReleaseDir}\pacwallet-desktop.json"; DestDir: "{userappdata}\Pingancoin Wallet"; DestName: "pacwallet-desktop.json"; Flags: ignoreversion onlyifdoesntexist uninsneveruninstall
Source: "{#SourceReleaseDir}\upstreams.mainnet.template.json"; DestDir: "{userappdata}\Pingancoin Wallet"; DestName: "upstreams.mainnet.template.json"; Flags: ignoreversion onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{autoprograms}\Pingancoin Wallet (Native Qt)"; Filename: "{app}\pacwallet-qt.exe"; WorkingDir: "{app}"; IconFilename: "{app}\branding\pingancoin-icon.ico"
Name: "{autoprograms}\Pingancoin Wallet"; Filename: "{app}\pacwallet-desktop.exe"; Parameters: "--config ""{userappdata}\Pingancoin Wallet\pacwallet-desktop.json"""; WorkingDir: "{app}"; IconFilename: "{app}\branding\pingancoin-icon.ico"
Name: "{autoprograms}\Pingancoin Wallet (Web Service)"; Filename: "{app}\pacwallet.exe"; Parameters: "serve --network mainnet --walletdir ""{userprofile}\.pacwallet"" --rpc http://115.190.57.12/rpc --listen 127.0.0.1:19709"; WorkingDir: "{app}"; IconFilename: "{app}\branding\pingancoin-icon.ico"
Name: "{autoprograms}\Wallet Release Notes"; Filename: "{app}\WINDOWS_RELEASE_NOTES.txt"
Name: "{userdesktop}\Pingancoin Wallet"; Filename: "{app}\pacwallet-desktop.exe"; Parameters: "--config ""{userappdata}\Pingancoin Wallet\pacwallet-desktop.json"""; WorkingDir: "{app}"; IconFilename: "{app}\branding\pingancoin-icon.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\pacwallet-desktop.exe"; Parameters: "--config ""{userappdata}\Pingancoin Wallet\pacwallet-desktop.json"""; WorkingDir: "{app}"; Description: "Launch Pingancoin Wallet"; Flags: nowait postinstall skipifsilent
