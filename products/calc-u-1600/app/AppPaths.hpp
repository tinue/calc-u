#pragma once
#include <string>
#include <utility>

#include "Connector/MediaSave.hpp"
#include "app/Settings.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"

// ── AppPaths: where the app's files live ─────────────────────────────────
//
// The save folder (~/Calc-U-1600 unless overridden in Settings), the
// tapes folder, the file a by-name save of a card, disk or tape goes to,
// and how a path is shown to the user. The places come from the library
// (calcu::settings::Paths, LIB-9 AC-12): the frontend passes the host's
// folders (calcu::settings::HostDirs: home, settings base, and on the Web
// the save folder) and the bundled folders (BundleDirs: *.card.yaml /
// *.floppy.yaml templates and ROM images, read only). Folders the app
// writes to are created when asked for.
namespace shared {

// The bundled folders the frontend passes (LIB-12, LIB-6 own them later).
struct BundleDirs {
    std::string bundledResources;
    // The bundled example presets (examples/bundled.yaml and its folders);
    // empty = "<bundledResources>/examples".
    std::string bundledExamples;
};

// A bundled preset as Settings stores it: "examples:basic/hanoi.pc1600",
// relative to AppPaths::bundledExamplesDir(), so it survives the app moving.
constexpr const char* kExamplePresetPrefix = "examples:";

class AppPaths {
public:
    AppPaths(const Settings& settings, calcu::settings::HostDirs host, BundleDirs bundle)
        : m_paths(settings.file(), std::move(host)), m_bundle(std::move(bundle)) {}

    const std::string& bundledResourcesDir() const { return m_bundle.bundledResources; }
    const std::string& homeDir() const { return m_paths.hostDirs().home; }
    std::string bundledExamplesDir() const;
    // The library's places (settings file, save folder, ...).
    const calcu::settings::Paths& places() const { return m_paths; }

    // A preset reference -- a path, or an "examples:" one -- as a path.
    std::string resolvePresetRef(const std::string& ref) const;

    // The save folder without the Settings override: ~/Calc-U-1600 (home
    // dir, non-dotted) on Qt6, the passed one on the Web; created if
    // missing.
    std::string defaultInstanceDir() const;

    // The save folder: Settings::instanceDirOverride() if set (Qt6), else
    // defaultInstanceDir(). Where battery-card instances and saved disks
    // live right now. (Adapter A-1 over Paths::saveFolder(), until M-47.)
    std::string instanceDir() const;

    // `path` the way every label shows one: under defaultInstanceDir() as
    // "~/Calc-U-1600/...", else under the home folder as "~/...", else as
    // is. A screenshot run's throwaway home thus reads like a fresh install's.
    std::string displayPath(const std::string& path) const;

    // Where a fresh by-name save goes: "<instanceDir>/<name>.card.yaml" /
    // ".floppy.yaml" (namedFileName()'s rules strip separators etc.).
    std::string instancePathFor(const std::string& instanceName) const;
    std::string floppyInstancePathFor(const std::string& diskName) const;

    // Cassette tapes: Settings::tapeDirOverride() if set, else "Tapes"
    // inside instanceDir(); created on demand. A tape named `tapeName` is
    // "<tapeName>.wav" there.
    std::string tapeDir() const;
    std::string tapePathFor(const std::string& tapeName) const;

    // Settings::traceDirOverride() / serialLinkDirOverride() if set, else
    // instanceDir(): where traces go (the library's trace session) and where
    // the serial ports' PTY symlinks live. The Web has no trace folder
    // (traceDir() is empty, LIB-10 D-10); its serial link folder is the save
    // folder (adapter A-4).
    std::string traceDir() const;
    std::string serialLinkDir() const;

    // A save of a card or a floppy under a name (Name & Save, a preset's
    // `saveas:`) by the shared rules (Connector/MediaSave.hpp): the path in
    // *path, and the refusal (None if allowed).
    MediaSaveRefusal planNamedSave(MediaKind kind, const std::string& name, const std::string& explicitPath,
                                   bool asTemplate, std::string* path) const;

private:
    calcu::settings::Paths m_paths;
    BundleDirs m_bundle;
};

// A planNamedSave() refusal as a sentence for the GUI (empty for None);
// Core's mediaSaveRefusalText() is the CLI/log wording.
std::string mediaSaveRefusalMessage(MediaSaveRefusal refusal, const std::string& name, const std::string& path);

// "a/b" joined the way every path here is built (forward slashes; an
// absolute `name` wins).
std::string joinPath(const std::string& dir, const std::string& name);

// True if `path` resolves to somewhere inside `dir` (both canonicalized;
// false if either doesn't exist). Whole path components only:
// "<dir>-backup/x" is not under "<dir>".
bool isUnderDir(const std::string& path, const std::string& dir);

// Writes `text` to `path` atomically: a temp file beside it, renamed into
// place, so a crash never leaves a half-written card or disk. False on
// failure.
bool atomicWriteFile(const std::string& path, const std::string& text);

} // namespace shared
