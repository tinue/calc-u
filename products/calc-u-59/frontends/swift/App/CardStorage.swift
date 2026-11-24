import Foundation

/// Resolves the save folder: the magnetic cards (`.magnetcard.yaml`, the
/// library's media part) and `ti58c.memory.yaml` (the TI-58C's constant
/// memory). Reading and writing them is the app layer's via the app session
/// (the bridge's coordinated file access).
///
/// Storage location (in priority order):
///   1. iCloud Documents container — syncs across all signed-in devices
///      automatically, provided the "iCloud Documents" capability is enabled
///      in the Xcode target and a matching container ID exists in the entitlements.
///   2. Local app Documents directory — used as a fallback when iCloud is
///      unavailable (simulator without an account, iCloud disabled, etc.).
///
/// To enable iCloud sync: in Xcode → target → Signing & Capabilities → "+ Capability"
/// → iCloud → check "iCloud Documents" and set the container identifier.
struct CardStorage {

    // Resolved once on a background thread; `url(forUbiquityContainerIdentifier:)`
    // must not be called on the main thread — it blocks and returns nil there.
    private static var _resolvedURL: URL?

    static var directoryURL: URL {
        if let cached = _resolvedURL { return cached }
        // Fallback used only when warmUp() hasn't completed yet.
        return FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    }

    /// Call once at app start off the main thread so the iCloud container
    /// URL is ready before any file operations or panel opens.
    static func warmUp() {
        if _resolvedURL != nil { return }
        if let cloudBase = FileManager.default.url(forUbiquityContainerIdentifier: nil) {
            let docs = cloudBase.appendingPathComponent("Documents", isDirectory: true)
            do {
                try FileManager.default.createDirectory(
                    at: docs, withIntermediateDirectories: true)
            } catch {
                // Still the folder: the files' own writes say what fails.
                TIAppLog.write("The iCloud folder \(docs.path) cannot be created: \(error.localizedDescription)",
                               level: .warn)
            }
            _resolvedURL = docs
        } else {
            _resolvedURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        }
    }
}
