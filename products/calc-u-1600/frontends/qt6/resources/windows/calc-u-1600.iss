; Inno Setup script for Calc-U-1600. Compiled headlessly in CI via
; ISCC.exe (see .github/workflows/calc-u-1600.yml's windows-x86_64 and
; windows-arm64 jobs) against the app folder build\release\install\Calc-U-1600
; (exe + Qt DLLs + MSVC runtime + resources\ + licenses\, staged by that
; job's "Stage standalone-runnable folder" step, `cmake --install`).
; Unsigned for now.
;
; APP_ARCH selects which arch's installer this produces -- pass
; /DAPP_ARCH=arm64 on the ISCC command line for the arm64 build; the
; windows-x86_64 job doesn't pass it, so it keeps the original x64
; defaults/filename below.
#ifndef APP_ARCH
  #define APP_ARCH "x64"
#endif

; APP_VERSION comes from frontends/qt6/CMakeLists.txt's APP_VERSION, written
; at configure time into the Qt6 app's directory of the root project's
; build/release tree -- so configure (cmake) before running ISCC.
#include "..\..\..\..\..\..\build\release\products\calc-u-1600\frontends\qt6\generated\Version.iss"

[Setup]
AppName=Calc-U-1600
AppVersion={#APP_VERSION}
DefaultDirName={autopf}\Calc-U-1600
DefaultGroupName=Calc-U-1600
UninstallDisplayIcon={app}\Calc-U-1600.exe
#if APP_ARCH == "arm64"
  OutputBaseFilename=Calc-U-1600-Setup-arm64
  ArchitecturesInstallIn64BitMode=arm64
  ArchitecturesAllowed=arm64
#else
  OutputBaseFilename=Calc-U-1600-Setup
  ArchitecturesInstallIn64BitMode=x64compatible
#endif
OutputDir=..\..\..\..
Compression=lzma2
SolidCompression=yes
ChangesAssociations=yes

[Files]
Source: "..\..\..\..\..\..\build\release\install\Calc-U-1600\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

; windeployqt --compiler-runtime (run by the install rules, see
; library/qt6/CMakeLists.txt) drops no loose msvcp140.dll/vcruntime140.dll etc.
; into the app folder for this Qt/VS toolset, but the vc_redist bootstrapper
; (vc_redist.x64.exe / vc_redist.arm64.exe), copied into {app} by the
; [Files] entry above, on the assumption the installer
; will run it. Without this [Run] entry the app fails to start on a clean
; machine with "missing MSVCP140.dll" (confirmed on real ARM64 hardware).
; /install /quiet /norestart is the standard silent-bootstrap invocation;
; it's a no-op (nonzero exit, e.g. 1638) if an equal-or-newer redist is
; already present, which Inno doesn't treat as a Setup failure.
[Run]
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Check: FileExists(ExpandConstant('{app}\vc_redist.x64.exe')); Flags: waituntilterminated
Filename: "{app}\vc_redist.arm64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Check: FileExists(ExpandConstant('{app}\vc_redist.arm64.exe')); Flags: waituntilterminated

[Registry]
; Open .pc1500 / .pc1500a / .pc1600 presets with Calc-U-1600.
Root: HKA; Subkey: "Software\Classes\.pc1500"; ValueType: string; ValueData: "CalcU1600.Preset"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.pc1500a"; ValueType: string; ValueData: "CalcU1600.Preset"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.pc1600"; ValueType: string; ValueData: "CalcU1600.Preset"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\CalcU1600.Preset"; ValueType: string; ValueData: "Calc-U-1600 Preset"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\CalcU1600.Preset\DefaultIcon"; ValueType: string; ValueData: "{app}\Calc-U-1600.exe,0"
Root: HKA; Subkey: "Software\Classes\CalcU1600.Preset\shell\open\command"; ValueType: string; ValueData: """{app}\Calc-U-1600.exe"" ""%1"""

[Icons]
Name: "{group}\Calc-U-1600"; Filename: "{app}\Calc-U-1600.exe"
Name: "{group}\Uninstall Calc-U-1600"; Filename: "{uninstallexe}"
; {commondesktop}, not {userdesktop}: PrivilegesRequired defaults to
; "admin" (needed for the {autopf}\Calc-U-1600 install dir above), and an
; elevated install's {userdesktop} would resolve to the elevated
; account's desktop, not the actual end user's.
Name: "{commondesktop}\Calc-U-1600"; Filename: "{app}\Calc-U-1600.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked
