#pragma once
#include <set>
#include <string>
#include <utility>

#include "app/Settings.hpp"
#include "applog/AppLog.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"

// ── AppPaths: where the app's files live ─────────────────────────────────
//
// The save folder (~/Calc-U-1600 unless overridden in Settings), the
// cassette folder, the file a by-name save of a module, disk or cassette goes to,
// and how a path is shown to the user. The places come from the library
// (calcu::settings::Paths): the frontend passes the host's
// folders (calcu::settings::HostDirs: home, settings base, and on the Web
// the save folder) and the bundled folders (BundleDirs: *.module.yaml /
// *.floppy.yaml templates and ROM images, read only). Folders the app
// writes to are created when asked for.
namespace cu1600 {

// The bundled folders the frontend passes.
struct BundleDirs {
    BundleDirs(std::string resources = {}, std::string examples = {})
        : bundledResources(std::move(resources)), bundledExamples(std::move(examples)) {}
    std::string bundledResources;
    // The bundled example presets (examples/bundle.yaml and its folders);
    // empty = "<bundledResources>/examples".
    std::string bundledExamples;
    // Where the ROM set, the module definitions (and the media templates
    // beside them) and the host drive's ROM lie when they are not all in
    // bundledResources, as in the source tree the harness reads
    // (roms/, frontends/qt6/resources/cards/, firmware/pc1600-hostdrive/);
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
    // Where a folder that cannot be created is told (none: nowhere).
    void useAppLog(calcu::applog::AppLog& log) { m_appLog = &log; }

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


    // Cassettes: [storage] cassette-folder if set, else "Cassettes"
    // inside instanceDir(); created on demand. Cassette definitions and their
    // sounds live there.
    std::string cassetteDir() const;

    // [trace] folder / [serial] link-folder if set, else
    // instanceDir(): where traces go (the library's trace session) and where
    // the serial ports' PTY symlinks live. The Web has no trace folder
    // (traceDir() is empty); its serial link folder is the save
    // folder.
    std::string traceDir() const;
    std::string serialLinkDir() const;


private:
    const std::string& orResources(const std::string& dir) const { return dir.empty() ? m_bundle.bundledResources : dir; }
    void makeDirs(const std::string& dir) const;
    calcu::settings::Paths m_paths;
    BundleDirs m_bundle;
    calcu::applog::AppLog* m_appLog = nullptr;
    mutable std::set<std::string> m_unmade;   // told once each
};


// "a/b" joined the way every path here is built (forward slashes; an
// absolute `name` wins).
std::string joinPath(const std::string& dir, const std::string& name);

// True if `path` resolves to somewhere inside `dir` (both canonicalized;
// false if either doesn't exist). Whole path components only:
// "<dir>-backup/x" is not under "<dir>".
bool isUnderDir(const std::string& path, const std::string& dir);


} // namespace cu1600
