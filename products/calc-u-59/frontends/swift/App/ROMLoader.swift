import Foundation
import os

/// The app bundle's ROM files (the module files hold the cue cards). They are looked up
/// once through the library's lookup (Bridge/TIRomSet); parsing and
/// checks live in the C++ layers. This only names the folder.
struct ROMLoader {
    private static let logger = Logger(subsystem: Bundle.main.bundleIdentifier ?? "Calc-U-59", category: "ROMLoader")

    /// The full set from the bundle's resource folder, or the text that says
    /// which files are missing or bad.
    static func lookUp() -> Result<TIRomSet, RomsMissing> {
        var leavingOut: String? = nil
        #if DEBUG
        // UI test only: a file treated as missing.
        leavingOut = ProcessInfo.processInfo.environment["CALCU59_ROM_LEAVE_OUT"]
        #endif
        var missing: NSString? = nil
        let folder = Bundle.main.resourcePath ?? Bundle.main.bundlePath
        if let set = TIRomSet.lookUp(inFolder: folder, leavingOut: leavingOut, missing: &missing) {
            return .success(set)
        }
        return .failure(RomsMissing(text: (missing as String?) ?? "ROM files missing"))
    }

    struct RomsMissing: Error {
        let text: String
    }
}
