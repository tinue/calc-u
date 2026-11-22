#include "shell/BarModel.hpp"
#include <algorithm>
#include <optional>

namespace calcu::shell {

namespace {

// An Action that follows its menu item, under the bar's label.
void addAction(std::vector<BarControl>& into, const std::vector<Menu>& ms, const char* id, const char* label) {
    const MenuItem* i = findItem(ms, id);
    if (!i) return;
    BarControl c;
    c.kind = BarControl::Kind::Action;
    c.id = id;
    c.label = label;
    c.item = *i;
    into.push_back(std::move(c));
}

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
    std::vector<Menu> ms;
    if (!t.menuBar) ms = menus(in, p, t);   // a window without a menu bar: Reset is in More

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
        const bool isSlot = placeKind(place, p) == PlaceKind::ModuleSlot;
        if (isSlot) ++slot;
        auto c = placeControl(place, p, isSlot && slotCount > 1 ? std::to_string(slot) + ":" : "");
        if (!c) continue;   // no control, no gap
        (isSlot ? slots : places).controls.push_back(std::move(*c));
    }
    for (const auto& m : read::elements(cs, "mediaPlaces")) {   // the owner's controls
        BarControl c;
        c.kind = BarControl::Kind::Media;
        c.id = read::text(m, "id");
        c.mediumKind = read::text(m, "kind");
        c.entry = m;
        media.controls.push_back(std::move(c));
    }
    BarGroupModel end{BarGroup::End, {}, p.barSpread};
    if (!t.menuBar) {   // a window without a menu bar
        addAction(end.controls, ms, actions::kLoadPreset, "Preset");
        addAction(end.controls, ms, actions::kSettings, "Settings");
        BarControl more;
        more.kind = BarControl::Kind::More;
        more.id = "more";
        more.label = "More";
        more.choices = moreItems(ms, p);
        end.controls.push_back(std::move(more));
    }
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
