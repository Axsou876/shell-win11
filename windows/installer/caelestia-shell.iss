; Per-user installer for the Windows port. Build it with
; windows\scripts\installer.ps1, which passes MyAppVersion from CMakeLists.txt
; and fills build-release\bin with the deployed Qt runtime first.
#define MyAppName "Caelestia Shell"
#define MyAppExeName "caelestia-shell.exe"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif

[Setup]
AppId={{F192D452-0C1D-4980-AB4D-3FA4BD2A15C7}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=Axsou876
AppPublisherURL=https://github.com/Axsou876/shell-win11
AppSupportURL=https://github.com/Axsou876/shell-win11/issues
AppUpdatesURL=https://github.com/Axsou876/shell-win11/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; Per-user install: no administrator prompt, same user as the HKCU settings
; and the autostart entry the tray icon writes.
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir=..\dist
OutputBaseFilename=Caelestia-Shell-{#MyAppVersion}-Setup
SetupIconFile=app.ico
UninstallDisplayIcon={app}\app.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Refuse to touch files while the shell runs: its tray "Quit" is the only path
; that gives the Windows taskbar back, and Setup cannot do that for it.
AppMutex=Local\CaelestiaShellWindows
CloseApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[CustomMessages]
english.StartWithWindows=Start {#MyAppName} when I sign in
french.StartWithWindows=Démarrer {#MyAppName} à l'ouverture de session

[Tasks]
Name: "autostart"; Description: "{cm:StartWithWindows}"; GroupDescription: "{cm:AutoStartProgramGroupDescription}"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build-release\bin\*"; DestDir: "{app}"; Excludes: "*.log,*.out,caelestia-smoke.png"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "app.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"
Name: "{autoprograms}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"; Tasks: desktopicon

[Registry]
; Same value the tray menu's "Start with Windows" writes, so both stay in sync.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "CaelestiaShell"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // Remove the autostart value even when the user enabled it from the tray
  // rather than from this installer.
  if CurUninstallStep = usUninstall then
    RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'CaelestiaShell');
end;
