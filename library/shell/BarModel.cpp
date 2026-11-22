#include "shell/BarModel.hpp"
#include <algorithm>
#include <optional>

namespace calcu::shell {

namespace {

// A place's control: a module slot's picker (its label "n:" given), a
// button where one peripheral fits, a picker where several do; none for a
// place without a control.
// A picker's short text: the chosen entry's ID after "peripheral-<place>-",
// "—" for none.
std::string shownOf(const std::vector<MenuItem>& choices, const std::string& place) {
    const std::string prefix = "peripheral-" + place + "-";
    for (const auto& i : choices)
        if (i.checked) {
            const std::string id = i.id.rfind(prefix, 0) == 0 ? i.id.substr(prefix.size()) : i.label;
            return id == "none" ? "\u2014" : id;
        }
    return "\u2014";
}

std::optional<BarControl> placeControl(const sessions::Value& place, const ShellProduct& p, std::string slotLabel) {
    BarControl c;
    c.id = read::text(place, "place");
    c.entry = place;
    c.shortShown = p.barShortPickers;
    switch (placeKind(place, p)) {
    case PlaceKind::ModuleSlot:   // the media components' picker
        c.kind = BarControl::Kind::SlotPicker;
        c.label = std::move(slotLabel);
        c.choices = placeChoices(place, true);
        c.shown = shownOf(c.choices, c.id);
        return c;
    case PlaceKind::Button: {
        c.kind = BarControl::Kind::PlaceButton;
        const auto& f = read::elements(place, "fitting").front();
        c.label = read::text(f, "name");
        c.plugged = read::flag(f, "plugged");
        c.choices = placeChoices(place, false);
        return c;
    }
    case PlaceKind::Picker:
        c.kind = BarControl::Kind::PlacePicker;
        c.choices = placeChoices(place, true);
        // Closed, the picker shows only what is plugged in: its empty entry
        // names the place ("No printer"), so it needs no label beside it.
        if (!c.choices.empty() && c.choices.front().id == "peripheral-" + c.id + "-none") {
            std::string name = c.id;
            std::replace(name.begin(), name.end(), '-', ' ');
            c.choices.front().label = "No " + name;
        }
        c.shown = shownOf(c.choices, c.id);
        return c;
    case PlaceKind::NoControl:
        break;
    }
    return std::nullopt;
}

} // namespace

std::vector<BarGroupModel> bar(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<BarGroupModel> out;
    if (!in.started()) return out;   // no control state: no bar
    const sessions::Value& cs = in.controlState;
    const bool mobile = t.mobile;

    BarGroupModel model{BarGroup::AfterModel, {}};
    BarControl picker;
    picker.kind = BarControl::Kind::ModelPicker;
    picker.id = "model";
    picker.choices = modelItems(cs);
    for (auto& choice : picker.choices) {   // a choice's label is the model ID
        if (const auto it = p.barModelLabels.find(choice.label); it != p.barModelLabels.end()) choice.label = it->second;
        if (choice.checked) picker.shown = choice.label;
    }
    picker.shortShown = p.barShortPickers;
    model.controls.push_back(std::move(picker));

    BarGroupModel slots{BarGroup::Slots, {}}, places{BarGroup::Places, {}}, media{BarGroup::Media, {}, p.barSpread};
    std::size_t slotCount = 0;
    for (const auto& place : read::elements(cs, "places"))
        if (placeKind(place, p) == PlaceKind::ModuleSlot) ++slotCount;
    std::size_t slot = 0;
    for (const auto& place : read::elements(cs, "places")) {
        if (p.placesOnPaper.count(read::text(place, "place"))) continue;   // on the paper panel
        const PlaceKind kind = placeKind(place, p);
        const bool isSlot = kind == PlaceKind::ModuleSlot;
        if (isSlot) ++slot;
        // Mobile keeps on its bar what is used while the machine runs: the
        // module slots, not the peripherals' places.
        if (mobile && !isSlot) continue;
        auto c = placeControl(place, p, isSlot && slotCount > 1 ? std::to_string(slot) + ":" : "");
        if (!c) continue;   // no control, no gap
        (isSlot ? slots : places).controls.push_back(std::move(*c));
    }
    for (const auto& m : read::elements(cs, "mediaPlaces")) {   // the owner's controls
        BarControl c;
        c.kind = BarControl::Kind::Media;
        c.id = read::text(m, "id");
        c.mediumKind = read::text(m, "kind");
        // Mobile: only the media the product names; the others are chosen
        // in their submenu of More.
        if (mobile && !p.mobileBarMedia.count(c.mediumKind)) continue;
        c.entry = m;
        media.controls.push_back(std::move(c));
    }
    // More, at the right edge of every bar: every menu's items.
    BarGroupModel end{BarGroup::End, {}, true};
    BarControl more;
    more.kind = BarControl::Kind::More;
    more.id = "more";
    more.label = "More";
    more.icon = "\u22EF";
    more.choices = moreItems(menus(in, p, t));
    end.controls.push_back(std::move(more));
    out.push_back(std::move(model));
    out.push_back(std::move(slots));
    out.push_back(std::move(places));
    out.push_back(std::move(media));
    out.push_back(std::move(end));
    return out;
}

BarFit barFit(double faceplate, double withoutLabels, double withLabels, double room) {
    BarFit f;
    f.labels = withLabels <= faceplate;
    f.width = std::min(room, std::max(faceplate, f.labels ? withLabels : withoutLabels));
    return f;
}

std::vector<Panel> panels(const ShellInputs& in, const ShellProduct& product) {
    std::vector<Panel> out;
    for (const auto& e : read::elements(in.controlState, "panels")) {
        Panel p;
        p.kind = read::text(e, "kind");
        if (const sessions::Value* o = e.find("owner")) {
            p.owner = read::text(*o, "peripheral");
            p.ownerPlace = read::text(*o, "place");
        }
        if (p.kind == "paper")
            for (const auto& place : read::elements(in.controlState, "places"))
                if (product.placesOnPaper.count(read::text(place, "place")))
                    if (auto c = placeControl(place, product, "")) p.places.push_back(std::move(*c));
        out.push_back(std::move(p));
    }
    return out;
}

} // namespace calcu::shell
