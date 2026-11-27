; The Inno Setup installer of a product's Qt6 app, included by the
; product's frontends/qt6/resources/windows/<app>.iss after its #defines:
;
;   AppName        the program's name, e.g. "Calc-U-59" (the staged folder
;                  build\release\install\<AppName>, <AppName>.exe)
;   AppProgId      the presets' ProgID, e.g. "CalcU59.Preset"
;   APP_VERSION    from generated\Version.iss of the app's folder in
;                  build\release (the product's .iss includes it)
;
; The product's .iss adds a [Registry] line per preset extension, naming
; {#AppProgId}. ISCC runs on the product's .iss, so the relative paths
; below start in the product's frontends\qt6\resources\windows. The
; workflow (.github/workflows/qt-desktop.yml) stages the app with
; `cmake --install build/release --prefix build/release/install` (the
; program, its bundled files and licences, Qt's DLLs and the MSVC
; runtime's installer from windeployqt --compiler-runtime) and runs
; ISCC.exe /DAPP_ARCH=<x64|arm64>. Unsigned.

#ifndef APP_ARCH
  #define APP_ARCH "x64"
#endif

[Setup]
AppName={#AppName}
AppVersion={#APP_VERSION}
AppPublisher=Martin Erzberger
AppPublisherURL=https://github.com/tinue/Calc-U
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#AppName}.exe
LicenseFile=..\..\..\..\..\..\LICENSE
#if APP_ARCH == "arm64"
  OutputBaseFilename={#AppName}-Setup-arm64
  ArchitecturesInstallIn64BitMode=arm64
  ArchitecturesAllowed=arm64
#else
  OutputBaseFilename={#AppName}-Setup
  ArchitecturesInstallIn64BitMode=x64compatible
  ArchitecturesAllowed=x64compatible
#endif
; The product's folder, where the workflow uploads the installer from.
OutputDir=..\..\..\..
Compression=lzma2
SolidCompression=yes
ChangesAssociations=yes

[Files]
Source: "..\..\..\..\..\..\build\release\install\{#AppName}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

; windeployqt --compiler-runtime puts no loose msvcp140.dll or
; vcruntime140.dll into the app's folder, but the VC++ redistributable's
; installer (vc_redist.x64.exe or vc_redist.arm64.exe), which [Files]
; copies into {app}. Without running it the app does not start on a clean
; machine ("missing MSVCP140.dll"). With an equal or newer redistributable
; already there it ends with a nonzero code (e.g. 1638), which Setup does
; not take as a failure.
[Run]
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Check: FileExists(ExpandConstant('{app}\vc_redist.x64.exe')); Flags: waituntilterminated
Filename: "{app}\vc_redist.arm64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Check: FileExists(ExpandConstant('{app}\vc_redist.arm64.exe')); Flags: waituntilterminated
Filename: "{app}\{#AppName}.exe"; Description: "Start {#AppName}"; Flags: nowait postinstall skipifsilent

[Registry]
; The presets open with the app; the product's .iss maps each extension to
; {#AppProgId}.
Root: HKA; Subkey: "Software\Classes\{#AppProgId}"; ValueType: string; ValueData: "{#AppName} Preset"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#AppProgId}\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppName}.exe,0"
Root: HKA; Subkey: "Software\Classes\{#AppProgId}\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppName}.exe"" ""%1"""

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
; {commondesktop}, not {userdesktop}: the install into {autopf} runs
; elevated, and an elevated install's {userdesktop} is the elevated
; account's desktop, not the user's.
Name: "{commondesktop}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked
