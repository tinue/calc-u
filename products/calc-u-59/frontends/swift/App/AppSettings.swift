import Foundation

// The settings are the session's: Calc-U-59.ini is read and
// written by the library's SettingsPart on the session thread, the views read the
// `settings` topic through `TIShell` and change a value with `set-setting`
// (TIShell.setSetting). What stays here is the platform's: where the file
// lives, and the launch argument of a Debug build that names a file of its own.

// MARK: - The values the views draw with

enum LEDFontStyle: Int, CaseIterable, Identifiable {
    case modernized = 0
    case classic    = 1

    var id: Int { rawValue }

    /// `[display] led-style` as the settings topic has it.
    init(setting: String) { self = setting == "classic" ? .classic : .modernized }
}

enum KeyboardFeedbackType: Int, CaseIterable, Identifiable {
    case off    = 0
    case haptic = 1
    case click  = 2

    var id: Int { rawValue }

    /// `[keyboard] feedback` as the settings topic has it.
    init(setting: String) {
        switch setting {
        case "haptic": self = .haptic
        case "click": self = .click
        default: self = .off
        }
    }
}

enum AppSettings {

    /// Application Support (inside the sandbox container): the settings
    /// file's base (`Calc-U-59/Calc-U-59.ini`).
    static var applicationSupportBase: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
    }

    /// A Debug build takes `--settings-file <path>`, the
    /// settings file of this launch (a relative path is taken in the app's
    /// temporary folder); a Release build has none.
    static var launchSettingsFile: String? {
        #if DEBUG
        let args = ProcessInfo.processInfo.arguments
        guard let i = args.firstIndex(of: "--settings-file"), i + 1 < args.count else { return nil }
        let path = args[i + 1]
        return path.hasPrefix("/") ? path : (NSTemporaryDirectory() as NSString).appendingPathComponent(path)
        #else
        return nil
        #endif
    }
}
