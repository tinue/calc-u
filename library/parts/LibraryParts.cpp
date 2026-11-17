#include "parts/LibraryParts.hpp"

#include "media/PresetMedia.hpp"
#include <filesystem>

namespace calcu::parts {

namespace {

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// LIB-6 AC-25: the folder for screenshots (the setup's, else the places'),
// made when a step needs it; none where there is none (the Web: a download).
std::function<std::optional<std::string>()> screenshotFolderOf(const settings::Paths& places,
                                                              std::optional<std::string> given) {
    return [&places, given]() -> std::optional<std::string> {
        std::optional<std::string> folder = given ? given : places.screenshotFolder();
        if (!folder || folder->empty()) return std::nullopt;
        std::error_code ec;
        std::filesystem::create_directories(pathOf(*folder), ec);
        return folder;
    };
}

// LIB-10: the trace folder and its limit from the host's places and settings,
// read at each trace; with `placesAtStart`, as they are now, the folder made
// when a trace starts.
debugger::DebuggerSetup withTraces(debugger::DebuggerSetup d, const LibraryPartsSetup& s) {
    const settings::SettingsFile* file = s.settings;
    if (!s.placesAtStart) {
        d.trace.folder = debugger::traceFolderOf(*s.paths, s.traceAvailable);
        d.trace.limitMb = [file] { return static_cast<long>(file->integer("trace", "max-file-size-mb")); };
        return d;
    }
    const long mb = file->integer("trace", "max-file-size-mb");
    d.trace.limitMb = [mb] { return mb; };
    const std::string dir = s.paths->traceFolder().value_or(std::string());
    if (dir.empty()) return d;   // none (the Web)
    const std::function<bool()> available = s.traceAvailable;
    d.trace.folder = [dir, available]() -> std::optional<settings::Paths::Ensured> {
        if (available && !available()) return settings::Paths::Ensured{false, dir, "The trace location is not available."};
        std::error_code ec;
        std::filesystem::create_directories(pathOf(dir), ec);
        if (ec) return settings::Paths::Ensured{false, dir, ec.message()};
        return settings::Paths::Ensured{true, dir, {}};
    };
    return d;
}

// With `placesAtStart`: the host's places as they are now, every folder as
// given (the save and tape folders of the media, the screenshots).
std::unique_ptr<settings::Paths> placesAtStart(const LibraryPartsSetup& s) {
    if (!s.placesAtStart) return nullptr;
    settings::HostDirs dirs;
    dirs.frontend = settings::FrontendKind::Given;
    dirs.home = s.paths->hostDirs().home;
    dirs.saveFolder = s.paths->saveFolder();
    dirs.traceFolder = s.paths->traceFolder().value_or(std::string());
    dirs.screenshotFolder = s.paths->screenshotFolder().value_or(std::string());
    return std::make_unique<settings::Paths>(*s.settings, dirs);
}

std::vector<machine::PeripheralId> printersOf(const std::vector<paper::PrinterPaper>& papers) {
    std::vector<machine::PeripheralId> out;
    for (const auto& p : papers) out.push_back(p.printer);
    return out;
}

} // namespace

LibraryParts::LibraryParts(LibraryPartsSetup s)
    : setup_(std::move(s)), startPlaces_(placesAtStart(setup_)), machineControl_(*setup_.machines, setup_.roms.set) {
    const machine::Catalogue& catalogue = setup_.machines->catalogue();
    const settings::Paths& places = startPlaces_ ? *startPlaces_ : *setup_.paths;
    typing_ = std::make_unique<typing::TypingPart>(*setup_.typing, setup_.hosting);
    peripherals_ = std::make_unique<peripherals::PeripheralsPart>(setup_.hostLinks, setup_.serialLinkFolder);
    presets_ = std::make_unique<presets::PresetsPart>(
        presets::PresetsSetup{&catalogue, setup_.presetRules, &machineControl_, typing_.get(), setup_.bundle,
                              places.hostDirs().home, screenshotFolderOf(places, setup_.screenshotFolder)});
    std::optional<roms::RomsMissing> missing;
    if (!setup_.roms.ok()) missing = setup_.roms.missing;
    startup_ = std::make_unique<startup::StartupPart>(
        startup::StartupSetup{machineControl_, *setup_.settings, presets_.get(), std::move(missing)});
    paper_ = std::make_unique<paper::PaperPart>(catalogue, setup_.papers, &machineControl_);
    paper::addPaperStep(presets_->vocabulary(), *paper_);
    debugger_ = std::make_unique<debugger::DebuggerPart>(withTraces(setup_.debugger, setup_));
    debugger::addDebuggerSteps(presets_->vocabulary(), *debugger_);
    media::MediaSetup m;
    m.kinds = setup_.mediaKinds;
    m.paths = &places;
    m.listing = setup_.moduleListing;
    m.catalogue = &catalogue;
    m.control = &machineControl_;
    m.files = setup_.mediaFiles;
    media_ = std::make_unique<media::MediaPart>(std::move(m));
    media::addMediaToPresets(presets_->vocabulary(), *media_);
    opening::OpenSetup open;
    open.productName = setup_.productName;
    presetsOpener_ = std::make_unique<opening::PresetsOpener>(*presets_, setup_.presetExtensions);
    open.presets = presetsOpener_.get();
    mediaOpener_ = std::make_unique<opening::MediaOpener>(*media_);
    open.media = mediaOpener_.get();
    open.kinds = setup_.fileKinds;
    open.maxBytes = setup_.openLimits;
    open_ = std::make_unique<opening::OpenPart>(std::move(open));
    control_ = std::make_unique<control::ControlPart>(
        control::ControlSetup{&catalogue, printersOf(setup_.papers), setup_.controlRules});
}

LibraryParts::~LibraryParts() = default;

std::vector<sessions::SessionPart*> LibraryParts::sessionParts(const SessionSlots& slots) {
    std::vector<sessions::SessionPart*> parts{control_.get()};
    parts.insert(parts.end(), slots.host.begin(), slots.host.end());
    parts.push_back(typing_.get());
    parts.push_back(peripherals_.get());
    parts.push_back(presets_.get());
    parts.push_back(startup_.get());
    parts.push_back(media_.get());
    parts.insert(parts.end(), slots.fileKinds.begin(), slots.fileKinds.end());
    parts.push_back(open_.get());
    parts.push_back(paper_.get());
    parts.push_back(debugger_.get());
    parts.insert(parts.end(), slots.product.begin(), slots.product.end());
    parts.insert(parts.end(), slots.after.begin(), slots.after.end());
    return parts;
}

} // namespace calcu::parts
