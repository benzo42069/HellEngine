#define MyAppName "EngineDemo"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef MyAppPublisher
  #define MyAppPublisher "EngineDemo"
#endif
#ifndef MyDistDir
  #define MyDistDir "dist\\EngineDemo"
#endif
#ifndef MyOutputDir
  #define MyOutputDir "dist"
#endif

[Setup]
AppId={{7A8B2F6A-7A5A-4A4F-8D2F-A8C77AB8D9B2}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyAppName}Installer
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayIcon={app}\EngineDemo.exe
WizardStyle=modern
DisableProgramGroupPage=no
ChangesAssociations=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#MyDistDir}\\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\\EngineDemo"; Filename: "{app}\\EngineDemo.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\\EngineDemo"; Filename: "{app}\\EngineDemo.exe"; Tasks: desktopicon; WorkingDir: "{app}"

[Run]
Filename: "{app}\\EngineDemo.exe"; Description: "Launch EngineDemo"; Flags: nowait postinstall skipifsilent

[Code]
function InitializeSetup(): Boolean;
var
  ExePath: string;
begin
  ExePath := ExpandConstant('{#MyDistDir}\\EngineDemo.exe');
  if not FileExists(ExePath) then
  begin
    MsgBox('Missing staged distribution executable: ' + ExePath + #13#10 +
      'Run tools\\package_dist.ps1 first.', mbCriticalError, MB_OK);
    Result := False;
    exit;
  end;
  Result := True;
end;
