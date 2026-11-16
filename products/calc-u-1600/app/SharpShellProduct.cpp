#include "app/SharpShellProduct.hpp"

#include <utility>
#include <vector>

#include "Machine/SharpCatalogue.hpp"
#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/LcdLayout.hpp"
#include "app/Settings.hpp"
#include "app/SharpControlRules.hpp"
#include "shell/ValueForms.hpp"

namespace shared {

namespace sessions = calcu::sessions;
namespace shell = calcu::shell;
using sessions::Value;

shell::ShellProduct shellProduct(std::string version, std::string build, bool withHostFolders) {
    shell::ShellProduct p;
    p.name = "Calc-U-1600";
    p.version = std::move(version);
    p.build = std::move(build);
    p.helpUrl = kHelpUrl;
    p.aboutLabel = "About Calc-U-1600…";
    p.aboutLines = {"A Sharp PC-1500/1500A/1600 pocket-computer emulator.",
                    "See LICENSE, NOTICE and THIRD-PARTY-NOTICES.md, shipped alongside this app, for licensing details."};
    p.variantLabels = {{sharp::kPC1500, "ROM Revision"}, {sharp::kPC1600, "ROM Version"}, {sharp::kCE1600P, "CE-1600P ROM"}};
    p.presetFilter = "Presets (*.pc1500 *.pc1500a *.pc1600)";
    p.pasteText = true;
    p.moduleSlotPinOuts = {sharp::kPC1500SlotPinOut, sharp::kPC1600Slot1PinOut, sharp::kPC1600Slot2PinOut};
    // AC-9: the host drive needs a folder (File ▸ Mount Directory…); a bus
    // ROM is plugged in by a preset only (C1600-4 D-5).
    p.placesWithoutControl = {sharp::kHostDrivePlace, sharp::kBusRomPlace};
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
        p.items.push_back(item(shell::MenuGroup::FileMedia, kItemMountDirectory, "Mount Directory…", kMountDirectoryEntry));
        p.items.push_back(item(shell::MenuGroup::FileMedia, kItemUnmountDirectory, "Unmount", kHostDriveFolderEntry));
    }
    return p;
}

shell::DialogSpec settingsSpec(bool qt6, bool serialPorts) {
    using W = shell::DialogRow::Widget;
    const std::vector<std::string> webKeys{"startup.model", "preset.PC-1500", "preset.PC-1500A", "preset.PC-1600"};
    shell::DialogSpec spec = shell::familySections(settingsDecl(), qt6 ? std::vector<std::string>{} : webKeys);
    for (auto& s : spec.sections) {
        if (s.title != "Folders") continue;
        // D-26: today's labels, "Samples" renamed.
        for (auto& r : s.rows) {
            if (r.key == openFolderKind(OpenFolder::Samples)) r.label = "Examples folder";
            else if (r.key == openFolderKind(OpenFolder::Basic)) r.label = "Basic folder";
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
    // The art's size around the shell's form of the hit map (D-31).
    Value::Object out{{"model", Value(model.value)}, {"width", Value(l.imageWidth)}, {"height", Value(l.imageHeight)}};
    const Value hitMap = shell::toValue(faceplateHitMap(model));
    for (const auto& member : *hitMap.members()) out.push_back(member);
    out.push_back({"lcd", std::move(lcdValue)});
    return Value(std::move(out));
}

} // namespace shared
