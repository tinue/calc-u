#pragma once
#include <string>
#include <utility>

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
    BundleDirs(std::string resources = {}, std::string examples = {})
        : bundledResources(std::move(resources)), bundledExamples(std::move(examples)) {}
    std::string bundledResources;
    // The bundled example presets (examples/bundled.yaml and its folders);
    // empty = "<bundledResources>/examples".
    std::string bundledExamples;
    // Where the ROM set, the module definitions (and the media templates
    // beside them) and the host drive's ROM lie when they are not all in
    // bundledResources, as in the source tree the harness reads (LIB-11
    // AC-20; roms/, frontends/qt6/resources/cards/, firmware/pc1600-hostdrive/);
    // empty = bundledResources.
    std::string roms, modules, hostDriveRom;
};

// A bundled preset as Settings stores it: "examples:basic/hanoi.pc1600",
// relative to AppPaths::bundledExamplesDir(), so it survives the app moving.
constexpr const char* kExamplePresetPrefix = "examples:";

class AppPaths {
public:
    AppPaths(const Settings& settings, calcu::settings::HostDirs host, BundleDirs bundle)
        : m_paths(settings.file(), std::move(host)), m_bundle(std::move(bundle)) {}

    const std::string& bundledResourcesDir() const { return m_bundle.bundledResources; }
    const std::string& bundledRomsDir() const { return orResources(m_bundle.roms); }
    const std::string& bundledModulesDir() const { return orResources(m_bundle.modules); }
    const std::string& bundledHostDriveRomDir() const { return orResources(m_bundle.hostDriveRom); }
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

    // The save folder: [storage] save-folder if set (Qt6), else
    // defaultInstanceDir(). Where battery-card instances and saved disks
    // live right now (Paths::saveFolder(); the media part reads places()).
    std::string instanceDir() const;

    // `path` the way every label shows one: under defaultInstanceDir() as
    // "~/Calc-U-1600/...", else under the home folder as "~/...", else as
    // is. A screenshot run's throwaway home thus reads like a fresh install's.
    std::string displayPath(const std::string& path) const;


    // Cassette tapes: [storage] tape-folder if set, else "Tapes"
    // inside instanceDir(); created on demand. Tape definitions and their
    // sounds live there (LIB-7 AC-42, LIB-9 AC-16).
    std::string tapeDir() const;

    // [trace] folder / [serial] link-folder if set, else
    // instanceDir(): where traces go (the library's trace session) and where
    // the serial ports' PTY symlinks live. The Web has no trace folder
    // (traceDir() is empty, LIB-10 D-10); its serial link folder is the save
    // folder.
    std::string traceDir() const;
    std::string serialLinkDir() const;


private:
    const std::string& orResources(const std::string& dir) const { return dir.empty() ? m_bundle.bundledResources : dir; }
    calcu::settings::Paths m_paths;
    BundleDirs m_bundle;
};


// "a/b" joined the way every path here is built (forward slashes; an
// absolute `name` wins).
std::string joinPath(const std::string& dir, const std::string& name);

// True if `path` resolves to somewhere inside `dir` (both canonicalized;
// false if either doesn't exist). Whole path components only:
// "<dir>-backup/x" is not under "<dir>".
bool isUnderDir(const std::string& path, const std::string& dir);


} // namespace shared
