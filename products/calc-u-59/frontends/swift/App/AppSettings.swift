import Foundation

// The settings are the session's (C59-8 D-3): Calc-U-59.ini is read and
// written by LIB-14's SettingsPart on the session thread, the views read the
// `settings` topic through `TIShell` and change a value with `set-setting`
// (TIShell.setSetting). What stays here is the platform's: where the file
// lives, the launch argument of a Debug build that names a file of its own,
// and the security-scoped bookmark macOS grants for a chosen trace folder
// (LIB-9 AC-34), which is not a setting.

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

// MARK: - Places

/// The one platform-store key: the security-scoped bookmark macOS grants for a
/// chosen trace folder (LIB-9 AC-34). It is not a setting.
nonisolated private let traceFolderBookmarkKey = "traceCustomPathBookmark"

#if os(macOS)
/// The bookmark last resolved and the folder it granted (nil: none), with its
/// access opened; asked from the session thread (`chosenTraceFolder`).
nonisolated(unsafe) private var resolvedTraceFolder: (bookmark: Data, url: URL?)?
nonisolated private let resolvedTraceFolderLock = NSLock()
#endif

enum AppSettings {

    /// Application Support (inside the sandbox container): the settings
    /// file's base (`Calc-U-59/Calc-U-59.ini`, LIB-9 AC-2).
    static var applicationSupportBase: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
    }

    /// C59-8 AC-21, D-10: a Debug build takes `--settings-file <path>`, the
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

    /// The folder the bookmark of `[trace] folder` grants (macOS); nil without
    /// a usable bookmark. The session asks it at each trace start, on its own
    /// thread (LIB-10 AC-53; the open panel's grant is the frontend's, the
    /// bookmark carries it to the next launch, LIB-9 AC-34): a bookmark is
    /// resolved once, its folder opened until another bookmark replaces it.
    nonisolated static func chosenTraceFolder() -> String? {
        #if os(macOS)
        guard let bookmarkData = UserDefaults.standard.data(forKey: traceFolderBookmarkKey) else { return nil }
        resolvedTraceFolderLock.lock()
        defer { resolvedTraceFolderLock.unlock() }
        if let kept = resolvedTraceFolder, kept.bookmark == bookmarkData { return kept.url?.path }
        resolvedTraceFolder?.url?.stopAccessingSecurityScopedResource()
        let url = resolveTraceFolderBookmark(bookmarkData)
        resolvedTraceFolder = (bookmarkData, url)
        return url?.path
        #else
        return nil
        #endif
    }

    #if os(macOS)
    /// The folder of a bookmark, its access opened; nil when it is stale or
    /// cannot be resolved (the plain path of `[trace] folder` is used then).
    nonisolated private static func resolveTraceFolderBookmark(_ bookmarkData: Data) -> URL? {
        var isStale = false
        do {
            let url = try URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil,
                              bookmarkDataIsStale: &isStale)
            if isStale {
                print("[AppSettings] the bookmark of the trace folder is stale: the plain path is used")
                return nil
            }
            if !url.startAccessingSecurityScopedResource() {
                print("[AppSettings] no sandbox extension for \(url.path): traces there will fail")
            }
            return url
        } catch {
            print("[AppSettings] the bookmark of the trace folder cannot be resolved: \(error)")
            return nil
        }
    }
    #endif

    /// Whether a chosen trace folder needs to be chosen again (a path, no bookmark).
    static func traceFolderNeedsReauthorization(path: String) -> Bool {
        #if os(macOS)
        return !path.isEmpty && UserDefaults.standard.data(forKey: traceFolderBookmarkKey) == nil
        #else
        return false
        #endif
    }

    /// Keeps the bookmark of a trace folder the user chose (macOS; the path
    /// itself is `[trace] folder`, set through the session).
    static func keepTraceFolderBookmark(_ url: URL) {
        #if os(macOS)
        if let bookmark = try? url.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil) {
            UserDefaults.standard.set(bookmark, forKey: traceFolderBookmarkKey)
        } else {
            UserDefaults.standard.removeObject(forKey: traceFolderBookmarkKey)
        }
        #endif
    }
}
