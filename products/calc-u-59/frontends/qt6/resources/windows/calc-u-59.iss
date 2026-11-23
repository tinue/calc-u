; The Calc-U-59 Windows installer: the library's installer of a Qt6 app
; (library/qt6/windows/CalcUApp.iss) with the product's name, ProgID and
; preset extensions. The version comes from the root project's
; build/release tree, generated/Version.iss of the Qt6 app's folder in it,
; itself from the Xcode project's MARKETING_VERSION.

#define AppName "Calc-U-59"
#define AppProgId "CalcU59.Preset"
#include "..\..\..\..\..\..\build\release\products\calc-u-59\frontends\qt6\generated\Version.iss"
#include "..\..\..\..\..\..\library\qt6\windows\CalcUApp.iss"

[Registry]
; Open .ti59 / .ti58 / .ti58c presets with Calc-U-59.
Root: HKA; Subkey: "Software\Classes\.ti59"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.ti58"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\.ti58c"; ValueType: string; ValueData: "{#AppProgId}"; Flags: uninsdeletevalue
