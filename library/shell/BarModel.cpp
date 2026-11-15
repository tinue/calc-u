#include "shell/BarModel.hpp"

namespace calcu::shell {

std::vector<BarGroupModel> bar(const ShellInputs& in, const ShellProduct& p) {
    std::vector<BarGroupModel> out;
    if (!in.started()) return out;   // B7 (AC-19)
    const sessions::Value& cs = in.controlState;

    BarGroupModel model{BarGroup::AfterModel, {}};
    BarControl picker;
    picker.kind = BarControl::Kind::ModelPicker;
    picker.id = "model";
    picker.choices = modelItems(cs);
    model.controls.push_back(std::move(picker));

    BarGroupModel slots{BarGroup::Slots, {}}, places{BarGroup::Places, {}}, media{BarGroup::Media, {}};
    std::size_t slotCount = 0;
    for (const auto& place : read::elements(cs, "places"))
        if (placeKind(place, p) == PlaceKind::ModuleSlot) ++slotCount;
    std::size_t slot = 0;
    for (const auto& place : read::elements(cs, "places")) {
        BarControl c;
        c.id = read::text(place, "place");
        c.entry = place;
        switch (placeKind(place, p)) {
        case PlaceKind::ModuleSlot:   // B1: LIB-7's picker, "n:" with more than one slot
            c.kind = BarControl::Kind::SlotPicker;
            ++slot;
            if (slotCount > 1) c.label = std::to_string(slot) + ":";
            c.choices = placeChoices(place, true);
            slots.controls.push_back(std::move(c));
            break;
        case PlaceKind::Button: {   // B3: one fits
            c.kind = BarControl::Kind::PlaceButton;
            const auto& f = read::elements(place, "fitting").front();
            c.label = read::text(f, "name");
            c.plugged = read::flag(f, "plugged");
            c.choices = placeChoices(place, false);
            places.controls.push_back(std::move(c));
            break;
        }
        case PlaceKind::Picker:   // B3: several fit
            c.kind = BarControl::Kind::PlacePicker;
            c.choices = placeChoices(place, true);
            places.controls.push_back(std::move(c));
            break;
        case PlaceKind::NoControl:   // B3, B4: no control, no gap
            break;
        }
    }
    for (const auto& m : read::elements(cs, "mediaPlaces")) {   // B1: the owner's controls
        BarControl c;
        c.kind = BarControl::Kind::Media;
        c.id = read::text(m, "id");
        c.mediumKind = read::text(m, "kind");
        c.entry = m;
        media.controls.push_back(std::move(c));
    }
    out.push_back(std::move(model));
    out.push_back(std::move(slots));
    out.push_back(std::move(places));
    out.push_back(std::move(media));
    out.push_back(BarGroupModel{BarGroup::End, {}});
    return out;
}

std::vector<Panel> panels(const ShellInputs& in) {
    std::vector<Panel> out;
    for (const auto& e : read::elements(in.controlState, "panels")) {
        Panel p;
        p.kind = read::text(e, "kind");
        if (const sessions::Value* o = e.find("owner")) {
            p.owner = read::text(*o, "peripheral");
            p.ownerPlace = read::text(*o, "place");
        }
        out.push_back(std::move(p));
    }
    return out;
}

} // namespace calcu::shell
