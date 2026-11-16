import Foundation
import Observation

// MARK: - LED Font Style

enum LEDFontStyle: Int, CaseIterable, Identifiable {
    case modernized = 0
    case classic    = 1

    var id: Int { rawValue }

    var displayName: String {
        switch self {
        case .modernized: return "Modernized"
        case .classic:    return "Classic"
        }
    }
}

// MARK: - Keyboard Feedback Type

enum KeyboardFeedbackType: Int, CaseIterable, Identifiable {
    case off    = 0
    case haptic = 1
    case click  = 2

    var id: Int { rawValue }

    var displayName: String {
        switch self {
        case .off:    return "Off"
        case .haptic: return "Haptic"
        case .click:  return "Click Sound"
        }
    }
}

// MARK: - The settings file

/// The app's settings: Calc-U-59.ini through `TISettings` (app/Settings.hpp
/// and the library's settings file, LIB-9). Views bind to `SettingsModel.shared`;
/// each change is written through. Under `--UITesting` the file is in memory
/// only, with the `--setting <section>.<key>=<value>` launch values applied,
/// so a test run neither reads nor changes the simulator's settings (AC-18).
/// The one value outside the file is the security-scoped bookmark of a chosen
/// trace folder (macOS), which stays in the platform store (AC-34).
@Observable
final class SettingsModel {
    static let shared = SettingsModel()

    @ObservationIgnored let file: TISettings

    /// The app's folder in Application Support (inside the sandbox container):
    /// the settings file's folder, and where traces go by default (AC-24).
    static var applicationSupportFolder: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Calc-U-59", isDirectory: true)
    }

    private init() {
        let args = ProcessInfo.processInfo.arguments
        if args.contains("--UITesting") {
            var values: [String] = []
            for (i, a) in args.enumerated() where a == "--setting" && i + 1 < args.count { values.append(args[i + 1]) }
            file = TISettings(defaultsOnlyWithLaunchSettings: values)
        } else {
            let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            file = TISettings(settingsBase: base.path)
        }
        if let problem = file.problem { print("[Settings] \(problem)") }
        startupModelRaw = file.startupModelChoice
        traceMaxFileSizeMB = file.traceMaxFileSizeMB
        ledFontStyleRaw = file.ledStyle
        keyboardFeedbackRaw = file.keyboardFeedback
        traceLocationRaw = file.traceLocation
        traceCustomPath = file.traceFolder
        portraitDebugPage = file.portraitDebugPage
    }

    /// -1 = last used; 0/1/2 = MachineModel.rawValue.
    var startupModelRaw: Int { didSet { file.startupModelChoice = startupModelRaw } }
    var traceMaxFileSizeMB: Int { didSet { file.traceMaxFileSizeMB = traceMaxFileSizeMB } }
    var ledFontStyleRaw: Int { didSet { file.ledStyle = ledFontStyleRaw } }
    var keyboardFeedbackRaw: Int { didSet { file.keyboardFeedback = keyboardFeedbackRaw } }
    var traceLocationRaw: Int { didSet { file.traceLocation = traceLocationRaw } }
    var traceCustomPath: String { didSet { file.traceFolder = traceCustomPath } }
    var portraitDebugPage: Bool { didSet { file.portraitDebugPage = portraitDebugPage } }
}

/// The one platform-store key: the security-scoped bookmark macOS grants for a
/// chosen trace folder (LIB-9 AC-34). It is not a setting.
private let traceFolderBookmarkKey = "traceCustomPathBookmark"

// MARK: - Trace file location

enum TraceLocation: Int, CaseIterable {
    case local   = 0   // Application Support/Calc-U-59/ (default — no TCC, always writable)
    case iCloud  = 1   // CardStorage.directoryURL (iCloud when available)
    case custom  = 2   // user-chosen directory, [trace] folder (macOS only)

    var displayName: String {
        switch self {
        case .local:   return "Local (App Library)"
        case .iCloud:  return "iCloud"
        case .custom:  return "Custom Folder"
        }
    }
}

// MARK: - AppSettings

enum AppSettings {

    static var settings: SettingsModel { SettingsModel.shared }

    static func resolvedLEDFontStyle() -> LEDFontStyle {
        LEDFontStyle(rawValue: settings.ledFontStyleRaw) ?? .modernized
    }

    static func resolvedKeyboardFeedback() -> KeyboardFeedbackType {
        KeyboardFeedbackType(rawValue: settings.keyboardFeedbackRaw) ?? .off
    }

    /// [trace] location: Application Support unless the file says otherwise
    /// (LIB-9 AC-24; on iOS too, behaviour change 59-30).
    static func resolvedTraceLocation() -> TraceLocation {
        TraceLocation(rawValue: settings.traceLocationRaw) ?? .local
    }

    /// The model to launch at startup: the chosen one, else the last used, else TI-59.
    static func resolvedStartupModel() -> MachineModel {
        MachineModel(rawValue: settings.file.startupModel) ?? .ti59
    }

    /// Maximum trace file size in MB (50 when not positive, LIB-9 AC-11).
    static func resolvedTraceMaxMB() -> Int { settings.file.traceMaxFileSizeMB }

    /// Whether the trace location is reachable (iCloud signed in, directory creatable).
    static func traceLocationAvailable() -> Bool {
        #if !os(macOS)
        if resolvedTraceLocation() == .iCloud && FileManager.default.ubiquityIdentityToken == nil {
            return false
        }
        #endif
        let traceDir = traceDirectory()
        do {
            try FileManager.default.createDirectory(at: traceDir, withIntermediateDirectories: true)
            return true
        } catch {
            print("[AppSettings] trace directory \(traceDir.path) is not available: \(error)")
            return false
        }
    }

    /// Returns the directory URL where trace files are written (manual and
    /// KEYSTROKES "Trace:"): calcu::settings::Paths by [trace] location
    /// (LIB-9 AC-24). For a chosen folder on macOS, the security-scoped
    /// bookmark grants access and its folder is passed in (AC-34); without a
    /// usable bookmark, the plain [trace] folder, else Application Support.
    static func traceDirectory() -> URL {
        var chosen: String? = nil
        #if os(macOS)
        if resolvedTraceLocation() == .custom, let bookmarkData = UserDefaults.standard.data(forKey: traceFolderBookmarkKey) {
            var isStale = false
            do {
                let url = try URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil, bookmarkDataIsStale: &isStale)
                if !isStale {
                    if !url.startAccessingSecurityScopedResource() {
                        print("[AppSettings] startAccessingSecurityScopedResource() returned false for \(url.path) — sandbox extension not granted, trace writes to this folder will fail")
                    }
                    chosen = url.path
                } else {
                    print("[AppSettings] Security-scoped bookmark for custom trace path is stale — falling back to plain path")
                }
            } catch {
                print("[AppSettings] Failed to resolve security-scoped bookmark for custom trace path: \(error)")
            }
        }
        #endif
        let path = settings.file.traceFolder(withSaveFolder: CardStorage.directoryURL.path,
                                             applicationSupport: SettingsModel.applicationSupportFolder.path,
                                             chosenFolder: chosen)
        return URL(fileURLWithPath: path, isDirectory: true)
    }

    /// Check if the custom trace path needs re-authorization (has path but no valid bookmark).
    static func customTracePathNeedsReauthorization() -> Bool {
        #if os(macOS)
        let hasPath = !settings.traceCustomPath.isEmpty
        let hasBookmark = UserDefaults.standard.data(forKey: traceFolderBookmarkKey) != nil
        return hasPath && !hasBookmark
        #else
        return false
        #endif
    }

    /// Save a custom trace directory ([trace] folder) and create a
    /// security-scoped bookmark for it (macOS only; the bookmark alone stays
    /// in the platform store, AC-34).
    static func setCustomTraceDirectory(_ url: URL) {
        #if os(macOS)
        if let bookmark = try? url.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil) {
            UserDefaults.standard.set(bookmark, forKey: traceFolderBookmarkKey)
        } else {
            UserDefaults.standard.removeObject(forKey: traceFolderBookmarkKey)
        }
        #endif
        settings.traceCustomPath = url.path
    }
}
