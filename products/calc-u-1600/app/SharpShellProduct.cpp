#include "app/SharpShellProduct.hpp"

#include <utility>
#include <vector>

#include "Machine/SharpCatalogue.hpp"
#include "app/KeyLayout/FaceplateLayout.hpp"
#include "app/LcdLayout.hpp"
#include "app/Settings.hpp"
#include "app/SharpControlRules.hpp"

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

const char* groupName(shell::MenuGroup g) {
    switch (g) {
        case shell::MenuGroup::FileOpen: return "FileOpen";
        case shell::MenuGroup::FileKinds: return "FileKinds";
        case shell::MenuGroup::FileMedia: return "FileMedia";
        case shell::MenuGroup::FileEnd: return "FileEnd";
        case shell::MenuGroup::Edit: return "Edit";
        case shell::MenuGroup::MachineEnd: return "MachineEnd";
        case shell::MenuGroup::Help: return "Help";
    }
    return "FileKinds";
}

Value rect(const shell::Rect& r) { return Value::object({{"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}}); }

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

const char* widgetName(shell::DialogRow::Widget w) {
    using W = shell::DialogRow::Widget;
    switch (w) {
        case W::StartupModel: return "startup-model";
        case W::StartupPreset: return "startup-preset";
        case W::Folder: return "folder";
        case W::Integer: return "integer";
        case W::Choice: return "choice";
        case W::Note: return "note";
        case W::Text: break;
    }
    return "text";
}

} // namespace

Value dialogSpecValue(const shell::DialogSpec& spec) {
    Value::Array sections;
    for (const auto& s : spec.sections) {
        Value::Array rows;
        for (const auto& r : s.rows)
            rows.push_back(Value::object(
                {{"widget", widgetName(r.widget)}, {"section", r.section}, {"key", r.key}, {"label", r.label}}));
        sections.push_back(Value::object({{"title", s.title}, {"rows", Value(std::move(rows))}}));
    }
    return Value(std::move(sections));
}

Value shellProductValue(const shell::ShellProduct& p) {
    Value::Object variantLabels;
    for (const auto& [id, label] : p.variantLabels) variantLabels.push_back({id, Value(label)});
    Value::Array items;
    for (const auto& i : p.items)
        items.push_back(Value::object({{"group", groupName(i.group)}, {"id", i.id}, {"label", i.label}, {"entry", i.entry}}));
    return Value::object({{"name", p.name},
                          {"helpUrl", p.helpUrl},
                          {"aboutLabel", p.aboutLabel},
                          {"aboutLines", strings(p.aboutLines)},
                          {"variantLabels", Value(std::move(variantLabels))},
                          {"presetFilter", p.presetFilter},
                          {"copyScreen", true},
                          {"pasteText", p.pasteText},
                          {"moduleSlotPinOuts", strings({p.moduleSlotPinOuts.begin(), p.moduleSlotPinOuts.end()})},
                          {"placesWithoutControl", strings({p.placesWithoutControl.begin(), p.placesWithoutControl.end()})},
                          {"items", Value(std::move(items))}});
}

Value faceplateMapValue(const calcu::machine::ModelId& model) {
    const KeyLayout::Layout& l = KeyLayout::layoutFor(model);
    const shell::HitMap m = faceplateHitMap(model);
    Value::Array keys;
    for (const auto& [id, r] : m.keys)
        keys.push_back(Value::object({{"name", id.value}, {"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h}}));
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
    return Value::object({{"model", model.value},
                          {"width", l.imageWidth},
                          {"height", l.imageHeight},
                          {"distance", m.distance},
                          {"display", rect(*m.display)},
                          {"keys", Value(std::move(keys))},
                          {"lcd", std::move(lcdValue)}});
}

} // namespace shared
