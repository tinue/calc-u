#include "app/SharpShellProduct.hpp"

#include <utility>
#include <vector>

#include "Machine/SharpCatalogue.hpp"
#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/LcdLayout.hpp"
#include "app/Settings.hpp"
#include "app/SharpControlRules.hpp"
#include "shell/ValueForms.hpp"

namespace cu1600 {

namespace sessions = calcu::sessions;
namespace shell = calcu::shell;
using sessions::Value;

shell::ShellProduct shellProduct(std::string version, std::string build, bool withHostFolders, bool withTrace) {
    shell::ShellProduct p;
    p.name = "Calc-U-1600";
    p.version = std::move(version);
    p.build = std::move(build);
    p.helpUrl = kHelpUrl;
    p.aboutLabel = "About Calc-U-1600…";
    p.aboutLines = {"A Sharp PC-1500/1500A/1600 pocket-computer emulator.",
                    "See LICENSE, NOTICE and THIRD-PARTY-NOTICES.md, shipped alongside this app, for licensing details."};
    p.aboutLinks = {{"GitHub", "https://github.com/tinue/calc-u"}, {"Support", "https://github.com/tinue/calc-u/issues"}};
    p.variantLabels = {{sharp::kPC1500, "ROM Revision"}, {sharp::kPC1600, "ROM Version"}, {sharp::kCE1600P, "CE-1600P ROM"}};
    p.presetFilter = "Presets (*.pc1500 *.pc1500a *.pc1600)";
    // What a preset can name: programs (.bas, .bbin, .bin, a tape's .wav), ROM
    // files (.bin), module, disk and cassette files (.yaml).
    p.presetUploads = ".pc1500,.pc1500a,.pc1600,.bas,.bbin,.bin,.wav,.yaml";
    // Copy Screen is each frontend's own act; this placeholder says
    // that the product has the item, so the page's form of it
    // (`c1600.shell-product`) stays as it was.
    p.copyScreen = [] {};
    p.pasteText = true;
    p.moduleSlotPinOuts = {sharp::kPC1500SlotPinOut, sharp::kPC1600Slot1PinOut, sharp::kPC1600Slot2PinOut};
    // The host drive needs a folder (File ▸ Host Drive…); a bus
    // ROM is plugged in by a preset only.
    p.placesWithoutControl = {sharp::kHostDrivePlace, sharp::kBusRomPlace};
    // Short model names, so that a phone's bar fits; the menus keep the IDs.
    p.barModelLabels = {{sharp::kPC1500, "1500"}, {sharp::kPC1500A, "1500A"}, {sharp::kPC1600, "1600"}};
    // The disk stays on a phone's bar (its own controls, the product's);
    // the cassette is chosen through More there.
    p.mobileBarMedia = {"floppy-disk"};
    auto item = [](shell::MenuGroup g, const char* id, const char* label, const char* entry = "") {
        shell::ProductItem i;
        i.group = g;
        i.id = id;
        i.label = label;
        i.entry = entry;
        return i;
    };
    p.items.push_back(item(shell::MenuGroup::FileKinds, kItemLoadBasic, "Load BASIC Program…"));
    p.items.push_back(item(shell::MenuGroup::FileKinds, kItemLoadCode, "Load Machine Code…"));
    if (withHostFolders) {
        p.items.push_back(item(shell::MenuGroup::FileMedia, kItemHostDrive, "Host Drive…", kHostDriveEntry));
        p.items.push_back(item(shell::MenuGroup::FileMedia, kItemUnmountHostDrive, "Unmount", kHostDriveFolderEntry));
    }
    if (withTrace) {
        // The frontend makes Trace a toggle and the levels a choice (their
        // `checked`, from the session's trace and log-level).
        p.items.push_back(item(shell::MenuGroup::MachineEnd, kItemTrace, "Trace"));
        shell::ProductItem log = item(shell::MenuGroup::MachineEnd, kItemLogLevel, "Log Level");
        log.children = {item(shell::MenuGroup::MachineEnd, kItemLogOff, "Off"),
                        item(shell::MenuGroup::MachineEnd, kItemLogInfo, "Info (how presets run)"),
                        item(shell::MenuGroup::MachineEnd, kItemLogDebug, "Debug")};
        p.items.push_back(std::move(log));
    }
    return p;
}

shell::DialogSpec settingsSpec(bool qt6, bool serialPorts) {
    using W = shell::DialogRow::Widget;
    const std::vector<std::string> webKeys{"startup.model", "preset.PC-1500", "preset.PC-1500A", "preset.PC-1600"};
    shell::DialogSpec spec = shell::familySections(settingsDecl(), qt6 ? std::vector<std::string>{} : webKeys);
    for (auto& s : spec.sections) {
        if (s.title != "Folders") continue;
        // Calc-U-1600's labels of the folder rows; the Samples folder is the Presets folder.
        for (auto& r : s.rows) {
            if (r.key == openFolderKind(OpenFolder::Samples)) r.label = "Presets folder";
            else if (r.key == openFolderKind(OpenFolder::Basic)) r.label = "BASIC folder";
            else if (r.key == openFolderKind(OpenFolder::Assembly)) r.label = "Assembly folder";
            else if (r.key == openFolderKind(OpenFolder::HostDrive)) r.label = "Host drive folder";
        }
    }
    if (!qt6) {
        spec.sections.push_back({"Storage", {{W::Note, "storage", "saved-media", kSavedMediaNote}}});
        return spec;
    }
    if (serialPorts) spec.sections.push_back({"Serial ports", {{W::Folder, "serial", "link-folder", "Link folder"}}});
    spec.sections.push_back({"Debugger",
                             {{W::Choice, "debug", "dap-enabled", "Debug server (DAP)"}, {W::Integer, "debug", "dap-port", "Port"}}});
    return spec;
}

shell::HitMap faceplateHitMap(const calcu::machine::ModelId& model) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(model);
    shell::HitMap m;
    for (std::size_t i = 0; i < l.keyCount; ++i)
        m.keys.push_back({calcu::machine::KeyId{l.keys[i].name ? l.keys[i].name : ""},
                          shell::Rect{l.keys[i].x, l.keys[i].y, l.keys[i].w, l.keys[i].h}});
    m.display = shell::Rect{l.lcdRect.x, l.lcdRect.y, l.lcdRect.w, l.lcdRect.h};
    m.distance = kFaceplateHitDistance;
    return m;
}

namespace {

Value strings(const std::vector<std::string>& v) {
    Value::Array out;
    for (const auto& s : v) out.emplace_back(s);
    return Value(std::move(out));
}

Value rgb(Rgb c) { return Value(Value::Array{Value(c.r), Value(c.g), Value(c.b)}); }

const char* styleName(lcd::Indicator::Style s) {
    switch (s) {
        case lcd::Indicator::Style::Inverted: return "inverted";
        case lcd::Indicator::Style::Dot: return "dot";
        case lcd::Indicator::Style::Text: break;
    }
    return "text";
}

const char* kindName(lcd::IndicatorSlot::Kind k) {
    switch (k) {
        case lcd::IndicatorSlot::Kind::Join: return "join";
        case lcd::IndicatorSlot::Kind::First: return "first";
        case lcd::IndicatorSlot::Kind::Always: return "always";
        case lcd::IndicatorSlot::Kind::Flag: break;
    }
    return "flag";
}

} // namespace

Value faceplateMapValue(const calcu::machine::ModelId& model) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(model);
    Value::Array slots;
    for (const lcd::IndicatorSlot& s : lcd::indicatorSlots(model))
        slots.push_back(Value::object({{"kind", kindName(s.kind)},
                                       {"names", strings(s.names)},
                                       {"caption", s.caption},
                                       {"x", s.x},
                                       {"scale", s.scale},
                                       {"style", styleName(s.style)}}));
    using namespace lcd;
    const bool p16 = model.value == sharp::kPC1600;
    Value lcdValue = Value::object({{"pc1600", p16},
                                    {"background", rgb(p16 ? kP16BackgroundRgb : kP15BackgroundRgb)},
                                    {"ink", rgb(kInkRgb)},
                                    {"p15IndicatorBarFraction", kP15IndicatorBarFraction},
                                    {"p15IndicatorFontHeightFraction", kP15IndicatorFontHeightFraction},
                                    {"p15IndicatorTopInsetFraction", kP15IndicatorTopInsetFraction},
                                    {"p15ContentScale", kP15ContentScale},
                                    {"p15MatrixTopGapFraction", kP15MatrixTopGapFraction},
                                    {"p16IndicatorRowTop", kP16IndicatorRowTop},
                                    {"p16LabelFontHeightFraction", kP16LabelFontHeightFraction},
                                    {"p16RowHeightFraction", kP16RowHeightFraction},
                                    {"p16MatrixLeft", kP16MatrixLeft},
                                    {"p16MatrixRight", kP16MatrixRight},
                                    {"p16MatrixTop", kP16MatrixTop},
                                    {"p16MatrixBottom", kP16MatrixBottom},
                                    {"indicators", Value(std::move(slots))}});
    // The art's size around the shell's form of the hit map.
    Value::Object out{{"model", Value(model.value)}, {"width", Value(l.imageWidth)}, {"height", Value(l.imageHeight)}};
    const Value hitMap = shell::toValue(faceplateHitMap(model));
    for (const auto& member : *hitMap.members()) out.push_back(member);
    out.push_back({"lcd", std::move(lcdValue)});
    return Value(std::move(out));
}

} // namespace cu1600
