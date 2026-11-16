#include "parts/LibraryParts.hpp"

#include "media/PresetMedia.hpp"

namespace calcu::parts {

LibraryParts::LibraryParts(LibraryPartsSetup s)
    : setup_(std::move(s)), machineControl_(*setup_.machines, setup_.roms) {
    if (setup_.typing) typing_ = std::make_unique<typing::TypingPart>(*setup_.typing, setup_.hosting);
    peripherals_ = std::make_unique<peripherals::PeripheralsPart>(setup_.hostLinks, setup_.serialLinkFolder);
    if (setup_.presetRules) {
        presets_ = std::make_unique<presets::PresetsPart>(
            presets::PresetsSetup{setup_.catalogue, setup_.presetRules, &machineControl_, typing_.get(), setup_.bundle,
                                  setup_.home, setup_.screenshotFolder});
    }
    startup_ = std::make_unique<startup::StartupPart>(
        startup::StartupSetup{machineControl_, *setup_.settings, presets_.get(), setup_.romsMissing});
    if (!setup_.papers.empty()) {
        paper_ = std::make_unique<paper::PaperPart>(*setup_.catalogue, setup_.papers, &machineControl_);
        if (presets_) paper::addPaperStep(presets_->vocabulary(), *paper_);
    }
    if (setup_.debugger) {
        debugger_ = std::make_unique<debugger::DebuggerPart>(*setup_.debugger);
        if (presets_) debugger::addDebuggerSteps(presets_->vocabulary(), *debugger_);
    }
    if (!setup_.mediaKinds.empty()) {
        media::MediaSetup m;
        m.kinds = setup_.mediaKinds;
        m.paths = setup_.mediaPaths;
        m.listing = setup_.moduleListing;
        m.catalogue = setup_.catalogue;
        m.control = &machineControl_;
        media_ = std::make_unique<media::MediaPart>(std::move(m));
        if (presets_) media::addMediaToPresets(presets_->vocabulary(), *media_);
    }
    opening::OpenSetup open;
    open.productName = setup_.productName;
    if (presets_) {
        presetsOpener_ = std::make_unique<opening::PresetsOpener>(*presets_, setup_.presetExtensions);
        open.presets = presetsOpener_.get();
    }
    if (media_) {
        mediaOpener_ = std::make_unique<opening::MediaOpener>(*media_);
        open.media = mediaOpener_.get();
    }
    open.kinds = setup_.fileKinds;
    open.maxBytes = setup_.openLimits;
    open_ = std::make_unique<opening::OpenPart>(std::move(open));
    control_ = std::make_unique<control::ControlPart>(
        control::ControlSetup{setup_.catalogue, setup_.printers, setup_.controlRules});
}

LibraryParts::~LibraryParts() = default;

std::vector<sessions::SessionPart*> LibraryParts::sessionParts(const std::vector<sessions::SessionPart*>& fileKindParts) {
    std::vector<sessions::SessionPart*> parts;
    if (typing_) parts.push_back(typing_.get());
    parts.push_back(peripherals_.get());
    if (presets_) parts.push_back(presets_.get());
    parts.push_back(startup_.get());
    if (media_) parts.push_back(media_.get());
    for (sessions::SessionPart* p : fileKindParts) parts.push_back(p);
    parts.push_back(open_.get());
    if (paper_) parts.push_back(paper_.get());
    if (debugger_) parts.push_back(debugger_.get());
    return parts;
}

} // namespace calcu::parts
