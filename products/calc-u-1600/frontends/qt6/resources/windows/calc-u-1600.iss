; The Calc-U-1600 Windows installer: the library's installer of a Qt6 app
; (library/qt6/windows/CalcUApp.iss) with the product's name, ProgID and
; preset extensions. The version comes from the root project's
; build/release tree, generated/Version.iss of the Qt6 app's folder in it,
; itself from frontends/qt6/CMakeLists.txt's APP_VERSION.

#define AppName "Calc-U-1600"
#define AppProgId "CalcU1600.Preset"
#include "..\..\..\..\..\..\build\release\products\calc-u-1600\frontends\qt6\generated\Version.iss"
#include "..\..\..\..\..\..\library\qt6\windows\CalcUApp.iss"

[Registry]
; Open .pc1500 / .pc1500a / .pc1600 presets with Calc-U-1600.
Root: HKA; Subkey: "Software\Classes\.pc1500"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.pc1500a"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.pc1600"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
