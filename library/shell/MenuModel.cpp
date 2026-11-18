#include "shell/MenuModel.hpp"
#include <algorithm>
#include <functional>

namespace calcu::shell {

namespace {

using sessions::Value;

MenuItem separator() {
    MenuItem s;
    s.kind = MenuItem::Kind::Separator;
    return s;
}

MenuItem action(std::string id, std::string label) {
    MenuItem i;
    i.id = std::move(id);
    i.label = std::move(label);
    return i;
}

MenuItem submenu(std::string id, std::string label, std::vector<MenuItem> children) {
    MenuItem i = action(std::move(id), std::move(label));
    i.kind = MenuItem::Kind::Submenu;
    i.children = std::move(children);
    return i;
}

void applyAvailability(MenuItem& i, const Availability& a) {
    i.enabled = a.now;
    i.reason = a.now ? std::string() : a.reason;
}

// The control state's action entry, or nullptr: an item without one does
// not appear.
const Value* actionEntry(const Value& cs, const std::string& command) {
    for (const auto& a : read::elements(cs, "actions"))
        if (read::text(a, "command") == command) return &a;
    return nullptr;
}

// Groups with a separator between those that have items.
void appendGroup(std::vector<MenuItem>& into, std::vector<MenuItem> group) {
    if (group.empty()) return;
    if (!into.empty()) into.push_back(separator());
    for (auto& i : group) into.push_back(std::move(i));
}

std::vector<MenuItem> productItems(const ShellInputs& in, const ShellProduct& p, MenuGroup g) {
    std::vector<MenuItem> out;
    // Without a control state the product could not start; only its
    // Help items stay.
    if (!in.started() && g != MenuGroup::Help) return out;
    const Value* entries = in.controlState.find("product");
    for (const auto& item : p.items) {
        if (item.group != g) continue;
        MenuItem i = action(item.id, item.label);
        if (!item.mediaPlace.empty()) {   // only with its media place, as its `insert`
            const Value* place = nullptr;
            for (const auto& m : read::elements(in.controlState, "mediaPlaces"))
                if (read::text(m, "id") == item.mediaPlace) place = &m;
            if (!place) continue;
            applyAvailability(i, availabilityOf(*place, "insert"));
        }
        if (!item.entry.empty() && i.enabled) {   // a product item follows its product entry
            const Value* e = entries ? entries->find(item.entry) : nullptr;
            applyAvailability(i, e ? availabilityOf(*e) : Availability{false, ""});
        }
        out.push_back(std::move(i));
    }
    return out;
}

std::vector<MenuItem> variantItems(const Value& variants, const std::string& running, const std::string& owner,
                                   const std::function<void(MenuItem&, const std::string&)>& command) {
    std::vector<MenuItem> out;
    if (!variants.elements()) return out;
    for (const auto& v : *variants.elements()) {
        const std::string id = read::text(v, "id");
        MenuItem i = action("variant-" + owner + "-" + id, id);
        i.checkable = true;
        i.checked = id == running;
        if (!i.checked) command(i, id);
        applyAvailability(i, availabilityOf(v));
        out.push_back(std::move(i));
    }
    return out;
}

std::string variantLabel(const ShellProduct& p, const std::string& id) {
    auto it = p.variantLabels.find(id);
    return it != p.variantLabels.end() ? it->second : "Variant";   // a missing label: checkShellProduct reports it
}

std::vector<MenuItem> fileMenu(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<MenuItem> items;
    // FileOpen: Load Preset…, Examples ▸.
    std::vector<MenuItem> open;
    if (const Value* lp = in.started() ? actionEntry(in.controlState, actions::kLoadPreset) : nullptr) {
        const Availability a = availabilityOf(*lp);
        MenuItem load = action(actions::kLoadPreset, "Load Preset…");
        load.shortcut = t.shortcuts ? MenuItem::Shortcut::LoadPreset : MenuItem::Shortcut::None;
        applyAvailability(load, a);
        open.push_back(std::move(load));
        // One submenu per group of the preset list's examples, in order.
        std::vector<std::pair<std::string, std::vector<MenuItem>>> groups;
        for (const auto& e : read::elements(in.presetList, "entries")) {
            if (read::text(e, "kind") != "example") continue;
            const std::string group = read::text(e, "group");
            const std::string ref = read::text(e, "bundlePath");
            MenuItem i = action("example-" + ref, read::text(e, "name"));
            i.command = commands::kLoadPreset;
            i.args = Value::object({{"ref", ref}});
            applyAvailability(i, a);
            auto g = std::find_if(groups.begin(), groups.end(), [&](const auto& x) { return x.first == group; });
            if (g == groups.end()) {
                groups.emplace_back(group, std::vector<MenuItem>{});
                g = groups.end() - 1;
            }
            g->second.push_back(std::move(i));
        }
        if (!groups.empty()) {
            std::vector<MenuItem> sub;
            for (auto& [group, list] : groups) {
                if (group.empty())
                    for (auto& i : list) sub.push_back(std::move(i));
                else
                    sub.push_back(submenu("examples-" + group, group, std::move(list)));
            }
            open.push_back(submenu("examples", "Examples", std::move(sub)));
        }
    }
    for (auto& i : productItems(in, p, MenuGroup::FileOpen)) open.push_back(std::move(i));
    appendGroup(items, std::move(open));
    appendGroup(items, productItems(in, p, MenuGroup::FileKinds));
    // FileMedia: Import Media…, Saved Media… where the frontend has the list
    // and the product has kinds of medium.
    std::vector<MenuItem> media;
    if (t.savedMedia && !read::elements(in.media, "kinds").empty()) {
        MenuItem import = action("import-media", "Import Media…");
        import.command = commands::kMediaImport;
        media.push_back(std::move(import));
        media.push_back(action(actions::kSavedMedia, "Saved Media…"));
    }
    for (auto& i : productItems(in, p, MenuGroup::FileMedia)) media.push_back(std::move(i));
    appendGroup(items, std::move(media));
    std::vector<MenuItem> end = productItems(in, p, MenuGroup::FileEnd);
    if (t.settings) {
        MenuItem settings = action(actions::kSettings, "Settings…");
        settings.role = MenuItem::Role::Preferences;
        settings.shortcut = t.shortcuts ? MenuItem::Shortcut::Preferences : MenuItem::Shortcut::None;
        end.push_back(std::move(settings));
    }
    if (t.hasQuit) {
        MenuItem quit = action(actions::kQuit, "Quit");
        quit.role = MenuItem::Role::Quit;
        quit.shortcut = t.shortcuts ? MenuItem::Shortcut::Quit : MenuItem::Shortcut::None;
        end.push_back(std::move(quit));
    }
    appendGroup(items, std::move(end));
    return items;
}

std::vector<MenuItem> editMenu(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<MenuItem> items;
    if (!in.started()) return items;   // not started: nothing to copy or paste into
    if (p.copyScreen) {
        MenuItem c = action(actions::kCopyScreen, "Copy Screen");
        c.shortcut = t.shortcuts ? MenuItem::Shortcut::Copy : MenuItem::Shortcut::None;
        items.push_back(std::move(c));
    }
    if (p.pasteText) {
        MenuItem v = action(actions::kPasteText, "Paste Text");
        v.shortcut = t.shortcuts ? MenuItem::Shortcut::Paste : MenuItem::Shortcut::None;
        items.push_back(std::move(v));
    }
    for (auto& i : productItems(in, p, MenuGroup::Edit)) items.push_back(std::move(i));
    return items;
}

std::vector<MenuItem> machineMenu(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<MenuItem> items;
    const Value& cs = in.controlState;
    std::vector<MenuItem> choose;
    // Model ▸.
    std::vector<MenuItem> models = modelItems(cs);
    if (!models.empty()) choose.push_back(submenu("model", "Model", std::move(models)));
    // A submenu for the running model's variants and for each plugged-in
    // peripheral's.
    const std::string model = read::text(cs, "model");
    if (const Value* vs = cs.find("variants"); vs && vs->elements() && vs->elements()->size() > 1) {
        auto list = variantItems(*vs, read::text(cs, "variant"), "model", [](MenuItem& i, const std::string& id) {
            i.command = commands::kVariant;
            i.args = Value::object({{"name", id}});
        });
        choose.push_back(submenu("variants-model", variantLabel(p, model), std::move(list)));
    }
    for (const auto& place : read::elements(cs, "places")) {
        const std::string where = read::text(place, "place");
        for (const auto& f : read::elements(place, "fitting")) {
            if (!read::flag(f, "plugged")) continue;
            const Value* vs = f.find("variants");
            if (!vs || !vs->elements() || vs->elements()->size() < 2) continue;
            const std::string id = read::text(f, "id");
            const std::string owner = where + "-" + id;
            auto list = variantItems(*vs, read::text(f, "variant"), owner, [&](MenuItem& i, const std::string& v) {
                i.command = commands::kAttach;   // attaching to a plugged place replaces what is there
                i.args = Value::object({{"place", where}, {"peripheral", id}, {"variant", v}});
            });
            choose.push_back(submenu("variants-" + owner, variantLabel(p, id), std::move(list)));
        }
    }
    // Peripherals ▸, one flat list, place after place.
    std::vector<MenuItem> peripherals;
    for (const auto& place : read::elements(cs, "places")) {
        const PlaceKind k = placeKind(place, p);
        if (k == PlaceKind::ModuleSlot || k == PlaceKind::NoControl) continue;
        std::vector<MenuItem> list = placeChoices(place, false);
        if (list.empty()) continue;
        if (!peripherals.empty()) peripherals.push_back(separator());
        for (auto& i : list) peripherals.push_back(std::move(i));
    }
    if (!peripherals.empty()) choose.push_back(submenu("peripherals", "Peripherals", std::move(peripherals)));
    appendGroup(items, std::move(choose));
    // Reset, Cold boot, Resume; then the product's.
    std::vector<MenuItem> run;
    auto commandItem = [&](const char* command, const char* label, MenuItem::Shortcut s) {
        const Value* e = actionEntry(cs, command);
        if (!e) return;
        MenuItem i = action(command, label);
        i.command = sessions::CommandName{command};
        i.shortcut = t.shortcuts ? s : MenuItem::Shortcut::None;
        applyAvailability(i, availabilityOf(*e));
        run.push_back(std::move(i));
    };
    commandItem("reset", "Reset", MenuItem::Shortcut::Reset);
    commandItem("cold-boot", "Cold boot", MenuItem::Shortcut::ColdBoot);
    MenuItem resume = action("resume", "Resume");
    resume.command = commands::kResume;
    if (!read::flag(in.runState, "paused")) applyAvailability(resume, {false, "The machine is not paused."});
    run.push_back(std::move(resume));
    for (auto& i : productItems(in, p, MenuGroup::MachineEnd)) run.push_back(std::move(i));
    appendGroup(items, std::move(run));
    return items;
}

std::vector<MenuItem> helpMenu(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<MenuItem> items;
    if (!p.helpUrl.empty()) items.push_back(action(actions::kHelp, p.name + " Help"));
    for (auto& i : productItems(in, p, MenuGroup::Help)) items.push_back(std::move(i));
    if (t.about) {
        MenuItem about = action(actions::kAbout, p.aboutLabel.empty() ? "About " + p.name : p.aboutLabel);
        about.role = MenuItem::Role::About;
        items.push_back(std::move(about));
    }
    return items;
}

const MenuItem* findIn(const std::vector<MenuItem>& items, const std::string& id) {
    for (const auto& i : items) {
        if (i.id == id && i.kind != MenuItem::Kind::Separator) return &i;
        if (const MenuItem* c = findIn(i.children, id)) return c;
    }
    return nullptr;
}

} // namespace

std::vector<MenuItem> modelItems(const sessions::Value& cs) {
    std::vector<MenuItem> out;
    const std::string running = read::text(cs, "model");
    for (const auto& m : read::elements(cs, "models")) {
        const std::string id = read::text(m, "id");
        MenuItem i = action("model-" + id, id);
        i.checkable = true;
        i.checked = id == running;
        if (!i.checked) {
            i.command = commands::kModel;
            i.args = Value::object({{"name", id}});
        }
        applyAvailability(i, availabilityOf(m));
        out.push_back(std::move(i));
    }
    return out;
}

PlaceKind placeKind(const sessions::Value& place, const ShellProduct& p) {
    if (p.moduleSlotPinOuts.count(read::text(place, "pinOut"))) return PlaceKind::ModuleSlot;
    if (p.placesWithoutControl.count(read::text(place, "place"))) return PlaceKind::NoControl;
    const auto n = read::elements(place, "fitting").size();
    if (n == 0) return PlaceKind::NoControl;   // nothing fits, nothing to show
    return n == 1 ? PlaceKind::Button : PlaceKind::Picker;
}

std::vector<MenuItem> placeChoices(const sessions::Value& place, bool withNone) {
    std::vector<MenuItem> out;
    const std::string where = read::text(place, "place");
    bool anyPlugged = false;
    for (const auto& f : read::elements(place, "fitting")) anyPlugged = anyPlugged || read::flag(f, "plugged");
    if (withNone) {
        MenuItem none = action("peripheral-" + where + "-none", "None");
        none.checkable = true;
        none.checked = !anyPlugged;
        if (anyPlugged) {
            none.command = commands::kDetach;
            none.args = Value::object({{"place", where}});
        }
        applyAvailability(none, availabilityOf(place, "empty"));
        out.push_back(std::move(none));
    }
    for (const auto& f : read::elements(place, "fitting")) {
        const std::string id = read::text(f, "id");
        MenuItem i = action("peripheral-" + where + "-" + id, read::text(f, "name"));
        i.checkable = true;
        i.checked = read::flag(f, "plugged");
        if (i.checked && withNone) {
            // A picker's plugged choice sends nothing (it is what runs).
        } else if (i.checked) {
            i.command = commands::kDetach;
            i.args = Value::object({{"place", where}});
        } else {
            i.command = commands::kAttach;
            i.args = Value::object({{"place", where}, {"peripheral", id}});
        }
        applyAvailability(i, availabilityOf(f));
        out.push_back(std::move(i));
    }
    return out;
}

std::vector<Menu> menus(const ShellInputs& in, const ShellProduct& p, Traits t) {
    std::vector<Menu> out;
    auto add = [&](const char* id, const char* label, std::vector<MenuItem> items) {
        if (!items.empty()) out.push_back(Menu{id, label, std::move(items)});   // a menu without items is left out
    };
    add("file", "File", fileMenu(in, p, t));
    add("edit", "Edit", editMenu(in, p, t));
    if (in.started()) add("machine", "Machine", machineMenu(in, p, t));   // not started: no Machine menu
    add("help", "Help", helpMenu(in, p, t));
    return out;
}

std::vector<MenuItem> moreItems(const std::vector<Menu>& ms, const ShellProduct& p) {
    auto onBar = [&](const MenuItem& i) {
        if (i.id == actions::kLoadPreset || i.id == actions::kSettings || i.id == actions::kQuit || i.id == "reset" ||
            i.id == "model" || i.id == "peripherals")
            return true;
        for (const auto& item : p.items)
            if (item.id == i.id && item.onBar) return true;
        return false;
    };
    std::vector<MenuItem> out;
    bool pending = false;
    for (const auto& m : ms) {
        pending = true;   // a new menu is a new group
        for (const auto& i : m.items) {
            if (i.kind == MenuItem::Kind::Separator) {
                pending = true;
                continue;
            }
            if (onBar(i)) continue;
            if (pending && !out.empty()) out.push_back(separator());
            pending = false;
            MenuItem copy = i;
            if (copy.id == "cold-boot") {
                copy.confirm = kColdBootQuestion;
                copy.destructive = true;
            }
            out.push_back(std::move(copy));
        }
    }
    return out;
}

const MenuItem* findItem(const std::vector<Menu>& ms, const std::string& id) {
    for (const auto& m : ms)
        if (const MenuItem* i = findIn(m.items, id)) return i;
    return nullptr;
}

const Menu* findMenu(const std::vector<Menu>& ms, const std::string& id) {
    for (const auto& m : ms)
        if (m.id == id) return &m;
    return nullptr;
}

} // namespace calcu::shell
