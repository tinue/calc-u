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

// MARK: - Startup presets chosen with "Choose…"

/// The sandbox lets the app read a picked file only while its security
/// scope is open, so a startup preset chosen with "Choose…" is kept as a
/// security-scoped bookmark per model and opened again before the session
/// reads it. The bookmark is a grant, not a setting: the setting is
/// `[preset] <model>` in Calc-U-59.ini.
enum StartupPresetGrants {
    /// Kept after the session took the preset; its scope stays open for the run.
    static func keep(_ url: URL, for model: String) {
        do {
            let data = try url.bookmarkData(options: [], includingResourceValuesForKeys: nil, relativeTo: nil)
            var all = UserDefaults.standard.dictionary(forKey: "startupPresetBookmarks") as? [String: Data] ?? [:]
            all[model] = data
            UserDefaults.standard.set(all, forKey: "startupPresetBookmarks")
        } catch {
            TIAppLog.write("The startup preset \(url.path) of the \(model) cannot be kept for the next start: "
                           + error.localizedDescription, level: .warn)
        }
    }

    /// Every kept grant opened for this run (at the start and every model
    /// switch the session may read it); one that no longer resolves is
    /// dropped, and the session's start notice says the preset was not read.
    static func openAll() {
        let kept = UserDefaults.standard.dictionary(forKey: "startupPresetBookmarks") as? [String: Data] ?? [:]
        var left: [String: Data] = [:]
        for (model, data) in kept {
            var stale = false
            guard let url = try? URL(resolvingBookmarkData: data, options: [], relativeTo: nil,
                                     bookmarkDataIsStale: &stale) else {
                TIAppLog.write("The kept startup preset of the \(model) no longer resolves; it is dropped.", level: .warn)
                continue
            }
            _ = url.startAccessingSecurityScopedResource()
            left[model] = stale ? (try? url.bookmarkData(options: [], includingResourceValuesForKeys: nil,
                                                         relativeTo: nil)) ?? data : data
        }
        if left != kept { UserDefaults.standard.set(left, forKey: "startupPresetBookmarks") }
    }
}
