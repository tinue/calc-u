import Foundation
import os

struct ModuleMetadata {
    var title: String = ""
    var id: String = ""
    var menuTitle: String = ""
}

/// The app bundle's ROM files and cuecards.txt. The ROM files are looked up
/// once through the library's lookup (LIB-12, Bridge/TIRomSet); parsing and
/// checks live in the C++ layers. This only names the folder.
struct ROMLoader {
    private static let logger = Logger(subsystem: Bundle.main.bundleIdentifier ?? "Calc-U-59", category: "ROMLoader")

    /// The full set from the bundle's resource folder, or the text that says
    /// which files are missing or bad (LIB-12 AC-5, AC-10).
    static func lookUp() -> Result<TIRomSet, RomsMissing> {
        var leavingOut: String? = nil
        #if DEBUG
        // UI test only (LIB-12 design D-8): a file treated as missing.
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

    /// cuecards.txt (roms/solid-state): the module list and cue cards (until M-55).
    static var cuecardsText: String {
        guard let url = Bundle.main.url(forResource: "cuecards", withExtension: "txt"),
              let text = try? String(contentsOf: url, encoding: .utf8) else {
            logger.error("cuecards.txt not found in app bundle")
            return ""
        }
        return text
    }

    /// Metadata for all solid-state modules, in menu order.
    static func loadAllModuleMetadata() -> [ModuleMetadata] {
        TIFileFormats.allModuleMetadata(cuecardsText).map { ModuleMetadata($0) }
    }
}

extension ModuleMetadata {
    init(_ m: TIModuleMetadata) {
        self.init(title: m.title, id: m.moduleID, menuTitle: m.menuTitle)
    }
}
