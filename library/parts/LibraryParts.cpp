#include "parts/LibraryParts.hpp"

#include "media/PresetMedia.hpp"
#include <filesystem>

namespace calcu::parts {

namespace {

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

// The folder for screenshots (the places'), made when a step needs it; none
// where there is none (the Web: a download).
std::function<std::optional<std::string>()> screenshotFolderOf(const settings::Paths& places) {
    return [&places]() -> std::optional<std::string> {
        std::optional<std::string> folder = places.screenshotFolder();
        if (!folder || folder->empty()) return std::nullopt;
        std::error_code ec;
        std::filesystem::create_directories(pathOf(*folder), ec);
        return folder;
    };
}

// The trace folder and its limit from the host's places and settings, read
// at each trace start, the folder made and written once (Paths::ensure),
// for every product.
debugger::DebuggerSetup withTraces(debugger::DebuggerSetup d, const LibraryPartsSetup& s) {
    const settings::SettingsFile* file = s.settings;
    d.trace.folder = debugger::traceFolderOf(*s.paths);
    d.trace.limitMb = [file] { return static_cast<long>(file->integer("trace", "max-file-size-mb")); };
    return d;
}

std::vector<machine::PeripheralId> printersOf(const std::vector<paper::PrinterPaper>& papers) {
    std::vector<machine::PeripheralId> out;
    for (const auto& p : papers) out.push_back(p.printer);
    return out;
}

} // namespace

LibraryParts::LibraryParts(LibraryPartsSetup s)
    : setup_(std::move(s)), machineControl_(*setup_.machines, setup_.roms.set) {
    const machine::Catalogue& catalogue = setup_.machines->catalogue();
    const settings::Paths& places = *setup_.paths;
    typing_ = std::make_unique<typing::TypingPart>(*setup_.typing, setup_.hosting);
    peripherals_ = std::make_unique<peripherals::PeripheralsPart>(setup_.hostLinks, setup_.serialLinkFolder);
    presets_ = std::make_unique<presets::PresetsPart>(
        presets::PresetsSetup{&catalogue, setup_.presetRules, &machineControl_, typing_.get(), setup_.bundle,
                              places.hostDirs().home, screenshotFolderOf(places), setup_.cacheFolder});
    std::optional<roms::RomsMissing> missing;
    if (!setup_.roms.ok()) missing = setup_.roms.missing;
    startup_ = std::make_unique<startup::StartupPart>(
        startup::StartupSetup{machineControl_, *setup_.settings, presets_.get(), std::move(missing)});
    paper_ = std::make_unique<paper::PaperPart>(catalogue, setup_.papers, machineControl_);
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
    open.presets = presets_.get();
    open.presetExtensions = setup_.presetExtensions;
    open.media = media_.get();
    open.kinds = setup_.fileKinds;
    open.maxBytes = setup_.openLimits;
    open.uploadFolder = setup_.uploadFolder;
    open.cacheFolder = setup_.cacheFolder;
    open.settings = setup_.settings;
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
