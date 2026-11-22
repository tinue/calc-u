// The menu model.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/MenuModel.hpp"
#include <string>
#include <vector>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;
using sessions::Value;

namespace {

const Traits kQt{true, true, false};
const Traits kWeb{false, false, true};

std::vector<std::string> ids(const std::vector<MenuItem>& items) {
    std::vector<std::string> out;
    for (const auto& i : items) out.push_back(i.kind == MenuItem::Kind::Separator ? "-" : i.id);
    return out;
}
std::vector<std::string> menuIds(const std::vector<Menu>& ms) {
    std::vector<std::string> out;
    for (const auto& m : ms) out.push_back(m.id);
    return out;
}
std::string join(const std::vector<std::string>& xs) {
    std::string s;
    for (const auto& x : xs) s += (s.empty() ? "" : " ") + x;
    return s;
}

} // namespace

TEST(menus_and_items_in_order) {
    auto p = st::product();
    p.copyScreen = [] {};
    p.pasteText = true;
    const auto ms = menus(st::inputs(), p, kQt);
    CHECK_EQ(join(menuIds(ms)), std::string("file edit machine help"));
    CHECK_EQ(join(ids(findMenu(ms, "file")->items)),
             std::string("load-preset examples - load-program - mount-drive - settings quit"));
    CHECK_EQ(join(ids(findMenu(ms, "edit")->items)), std::string("copy-screen paste-text"));
    CHECK_EQ(join(ids(findMenu(ms, "machine")->items)),
             std::string("model variants-model peripherals - reset cold-boot resume disk-thing"));
    CHECK_EQ(join(ids(findMenu(ms, "help")->items)), std::string("help about"));
    CHECK_EQ(findItem(ms, "load-preset")->label, std::string("Load Preset…"));
    CHECK_EQ(findItem(ms, "settings")->label, std::string("Settings…"));
    CHECK_EQ(findItem(ms, "cold-boot")->label, std::string("Cold boot"));
    CHECK_EQ(findItem(ms, "help")->label, std::string("Test-Product Help"));
    CHECK_EQ(findItem(ms, "about")->label, std::string("About Test-Product"));
    // Examples ▸: one submenu per group of the preset list, in order.
    const MenuItem* ex = findItem(ms, "examples");
    CHECK_EQ(join(ids(ex->children)), std::string("examples-Games examples-Math"));
    const MenuItem* one = findItem(ms, "example-examples:games/one.test");
    CHECK(one != nullptr);
    CHECK_EQ(one->command.value, std::string("load-preset"));
    CHECK(one->args == Value::object({{"ref", "examples:games/one.test"}}));
}

TEST(item_without_function_and_empty_menu_left_out) {
    auto in = st::inputs();
    in.presetList = Value();   // no examples: no Examples ▸
    auto cs = st::controlState();
    cs.actions.erase(cs.actions.begin() + 1);   // no Cold boot action: no item
    in.controlState = cs.toValue();
    auto p = st::product();
    p.items.clear();
    const auto ms = menus(in, p, kQt);
    CHECK_EQ(join(menuIds(ms)), std::string("file machine help"));   // no Edit: neither Copy Screen nor Paste Text
    CHECK(findItem(ms, "examples") == nullptr);
    CHECK(findItem(ms, "cold-boot") == nullptr);
    CHECK(findItem(ms, "reset") != nullptr);
    CHECK_EQ(join(ids(findMenu(ms, "file")->items)), std::string("load-preset - settings quit"));
    // A product without a help address: no Help item, About stays.
    p.helpUrl.clear();
    CHECK_EQ(join(ids(findMenu(menus(in, p, kQt), "help")->items)), std::string("about"));
}

TEST(model_menu_marks_running_and_sends_model) {
    const auto ms = menus(st::inputs(), st::product(), kQt);
    const MenuItem* model = findItem(ms, "model");
    CHECK_EQ(join(ids(model->children)), std::string("model-Model-A model-Model-B"));
    const MenuItem* a = findItem(ms, "model-Model-A");
    const MenuItem* b = findItem(ms, "model-Model-B");
    CHECK(a->checkable && a->checked);
    CHECK(a->command.value.empty());   // the running one sends nothing
    CHECK(!b->checked);
    CHECK_EQ(b->command.value, std::string("model"));
    CHECK(b->args == Value::object({{"name", "Model-B"}}));
}

TEST(variant_submenus_for_model_and_peripheral) {
    auto cs = st::controlState();
    st::plug(cs, "aux", "test-plotter");
    const auto ms = menus(st::inputs(cs.toValue()), st::product(), kQt);
    const MenuItem* vm = findItem(ms, "variants-model");
    CHECK_EQ(vm->label, std::string("Revision"));   // the product's label
    CHECK_EQ(join(ids(vm->children)), std::string("variant-model-v1 variant-model-v2"));
    CHECK(vm->children[0].checked);
    CHECK_EQ(vm->children[1].command.value, std::string("variant"));
    CHECK(vm->children[1].args == Value::object({{"name", "v2"}}));
    const MenuItem* vp = findItem(ms, "variants-aux-test-plotter");
    CHECK(vp != nullptr);
    CHECK_EQ(vp->label, std::string("Pen"));
    CHECK(vp->children[0].checked);
    CHECK_EQ(vp->children[1].command.value, std::string("attach"));   // the replace by that variant
    CHECK(vp->children[1].args ==
          Value::object({{"place", "aux"}, {"peripheral", "test-plotter"}, {"variant", "pen-2"}}));
    // A peripheral that is not plugged in has no submenu.
    CHECK(findItem(menus(st::inputs(), st::product(), kQt), "variants-aux-test-plotter") == nullptr);
    // A model with one variant or none has none (Model-B).
    CHECK(findItem(menus(st::inputs(st::controlValue("Model-B")), st::product(), kQt), "variants-model") == nullptr);
}

TEST(peripherals_menu_attach_replace_and_detach) {
    auto cs = st::controlState();
    st::plug(cs, "aux", "test-plotter");
    const auto ms = menus(st::inputs(cs.toValue()), st::product(), kQt);
    // Not the module slots (their picker is the media components'), not the host drive.
    CHECK_EQ(join(ids(findItem(ms, "peripherals")->children)),
             std::string("peripheral-printer-test-dotter - peripheral-aux-test-plotter peripheral-aux-test-dotter-2"));
    const MenuItem* td20 = findItem(ms, "peripheral-printer-test-dotter");
    CHECK(!td20->checked);
    CHECK_EQ(td20->command.value, std::string("attach"));
    CHECK(td20->args == Value::object({{"place", "printer"}, {"peripheral", "test-dotter"}}));
    const MenuItem* tp4 = findItem(ms, "peripheral-aux-test-plotter");
    CHECK(tp4->checked);
    CHECK_EQ(tp4->command.value, std::string("detach"));
    CHECK(tp4->args == Value::object({{"place", "aux"}}));
    const MenuItem* td21 = findItem(ms, "peripheral-aux-test-dotter-2");   // the replace: attach, which replaces
    CHECK_EQ(td21->command.value, std::string("attach"));
    CHECK(td21->args == Value::object({{"place", "aux"}, {"peripheral", "test-dotter-2"}}));
}

TEST(menu_shortcuts_qt_and_none_on_web) {
    auto p = st::product();
    p.copyScreen = [] {};
    p.pasteText = true;
    const auto qt = menus(st::inputs(), p, kQt);
    CHECK(findItem(qt, "load-preset")->shortcut == MenuItem::Shortcut::LoadPreset);
    CHECK(findItem(qt, "reset")->shortcut == MenuItem::Shortcut::Reset);
    CHECK(findItem(qt, "cold-boot")->shortcut == MenuItem::Shortcut::ColdBoot);
    CHECK(findItem(qt, "settings")->shortcut == MenuItem::Shortcut::Preferences);
    CHECK(findItem(qt, "quit")->shortcut == MenuItem::Shortcut::Quit);
    CHECK(findItem(qt, "copy-screen")->shortcut == MenuItem::Shortcut::Copy);
    CHECK(findItem(qt, "paste-text")->shortcut == MenuItem::Shortcut::Paste);
    CHECK(findItem(qt, "resume")->shortcut == MenuItem::Shortcut::None);
    const auto web = menus(st::inputs(), p, kWeb);
    for (const char* id : {"load-preset", "reset", "cold-boot", "settings", "copy-screen", "paste-text"})
        CHECK(findItem(web, id)->shortcut == MenuItem::Shortcut::None);
}

TEST(menu_unavailable_disabled_with_reason) {
    auto cs = st::controlState();
    cs.models[1].available = st::unavailable("Model-B needs a module.");
    cs.actions[0].available = st::unavailable("Reset is not possible now.");
    cs.places[2].fitting[0].available = st::unavailable("TD-20 does not fit now.");
    const auto ms = menus(st::inputs(cs.toValue()), st::product(), kQt);
    const MenuItem* b = findItem(ms, "model-Model-B");
    CHECK(!b->enabled);
    CHECK_EQ(b->reason, std::string("Model-B needs a module."));
    CHECK(!findItem(ms, "reset")->enabled);
    CHECK_EQ(findItem(ms, "reset")->reason, std::string("Reset is not possible now."));
    CHECK(!findItem(ms, "peripheral-printer-test-dotter")->enabled);
    CHECK_EQ(findItem(ms, "peripheral-printer-test-dotter")->reason, std::string("TD-20 does not fit now."));
    CHECK(findItem(ms, "cold-boot")->enabled);
    CHECK(findItem(ms, "cold-boot")->reason.empty());
    // Resume can be used while the machine is paused.
    CHECK(!findItem(ms, "resume")->enabled);
    auto in = st::inputs(cs.toValue());
    in.runState = Value::object({{"paused", true}});
    CHECK(findItem(menus(in, st::product(), kQt), "resume")->enabled);
}

TEST(menu_roles_and_no_quit_on_web) {
    const auto qt = menus(st::inputs(), st::product(), kQt);
    CHECK(findItem(qt, "settings")->role == MenuItem::Role::Preferences);
    CHECK(findItem(qt, "about")->role == MenuItem::Role::About);
    CHECK(findItem(qt, "quit")->role == MenuItem::Role::Quit);
    int others = 0;   // every other item has no role
    for (const char* id : {"load-preset", "examples", "load-program", "model", "reset", "cold-boot", "resume", "help"})
        others += findItem(qt, id)->role == MenuItem::Role::None ? 1 : 0;
    CHECK_EQ(others, 8);
    const auto web = menus(st::inputs(), st::product(), kWeb);
    CHECK(findItem(web, "quit") == nullptr);
    CHECK(findItem(web, "settings") != nullptr);
}

TEST(product_item_in_its_group) {
    auto p = st::product();
    p.items.push_back(ProductItem{MenuGroup::FileOpen, "open-thing", "Open Thing…", "", {}});
    p.items.push_back(ProductItem{MenuGroup::FileEnd, "end-thing", "End Thing", "", {}});
    p.items.push_back(ProductItem{MenuGroup::Edit, "copy-log", "Copy Log", "", {}});
    p.items.push_back(ProductItem{MenuGroup::Help, "tips", "Tips", "", {}});
    const auto ms = menus(st::inputs(), p, kQt);
    CHECK_EQ(join(ids(findMenu(ms, "file")->items)),
             std::string("load-preset examples open-thing - load-program - mount-drive - end-thing settings quit"));
    CHECK_EQ(join(ids(findMenu(ms, "edit")->items)), std::string("copy-log"));
    CHECK_EQ(join(ids(findMenu(ms, "help")->items)), std::string("help tips about"));
    // Shown and disabled like the shell's own, from its product entry.
    const MenuItem* disk = findItem(ms, "disk-thing");
    CHECK(!disk->enabled);
    CHECK_EQ(disk->reason, std::string("No disk drive."));
    CHECK(disk->command.value.empty());   // the product's act
}

TEST(media_items_where_saved_media_and_kinds) {
    auto in = st::inputs();
    in.media = st::mediaWithKind();
    const auto web = menus(in, st::product(), kWeb);
    CHECK_EQ(join(ids(findMenu(web, "file")->items)),
             std::string("load-preset examples - load-program - import-media saved-media mount-drive - settings"));
    CHECK_EQ(findItem(web, "import-media")->label, std::string("Import Media…"));
    CHECK_EQ(findItem(web, "import-media")->command.value, std::string("media.import"));
    CHECK_EQ(findItem(web, "saved-media")->label, std::string("Saved Media…"));
    CHECK(findItem(menus(in, st::product(), kQt), "import-media") == nullptr);   // Qt6: no saved media list
    in.media = Value::object({{"kinds", Value(Value::Array{})}});
    CHECK(findItem(menus(in, st::product(), kWeb), "import-media") == nullptr);   // no kinds of medium
}

TEST(no_machine_menu_without_control_state) {
    ShellInputs in;   // the product could not start
    in.romsMissing = Value::object({{"text", "ROM files are missing."}});
    const auto ms = menus(in, st::product(), kQt);
    CHECK_EQ(join(menuIds(ms)), std::string("file help"));
    CHECK_EQ(join(ids(findMenu(ms, "file")->items)), std::string("settings quit"));
    CHECK_EQ(join(ids(findMenu(ms, "help")->items)), std::string("help about"));
}

TEST(model_switch_changes_menu_entries) {
    const auto a = menus(st::inputs(st::controlValue("Model-A")), st::product(), kQt);
    const auto b = menus(st::inputs(st::controlValue("Model-B")), st::product(), kQt);
    CHECK(findItem(a, "model-Model-A")->checked);
    CHECK(findItem(b, "model-Model-B")->checked);
    CHECK(findItem(a, "peripheral-aux-test-plotter") != nullptr);
    CHECK(findItem(b, "peripheral-aux-test-plotter") == nullptr);
    CHECK_EQ(join(ids(findItem(b, "peripherals")->children)), std::string("peripheral-printer-test-dotter"));
}

TEST(check_shell_product_reports_missing_labels) {
    auto p = st::product();
    CHECK(checkShellProduct(p, st::controlValue()).empty());
    p.variantLabels.erase("Model-A");
    p.items.push_back(p.items.front());
    const auto problems = checkShellProduct(p, st::controlValue());
    CHECK_EQ(problems.size(), std::size_t{2});
}

// ── The More button ──

namespace {

Traits mobile() {
    Traits t = kWeb;
    t.mobile = true;
    return t;
}

} // namespace

TEST(more_items_are_every_menu_under_its_heading) {
    auto in = st::inputs();
    in.media = st::mediaWithKind();
    auto p = st::product();
    const auto ms = menus(in, p, mobile());
    // The menus are computed without a menu bar too.
    CHECK_EQ(join(menuIds(ms)), std::string("file machine help"));
    // Every item of every menu, each menu after its heading; submenus stay submenus.
    const auto more = moreItems(ms);
    CHECK_EQ(join(ids(more)),
             std::string("heading-file load-preset examples - load-program - import-media saved-media mount-drive - settings "
                         "heading-machine model variants-model peripherals - reset cold-boot resume disk-thing "
                         "heading-help help about"));
    CHECK(more[0].kind == MenuItem::Kind::Heading);
    CHECK_EQ(more[0].label, std::string("FILE"));
    for (const auto& i : more)
        if (i.id == "model") CHECK(i.kind == MenuItem::Kind::Submenu && !i.children.empty());
    // Quit is in More too where the menus have it.
    bool quit = false;
    for (const auto& i : moreItems(menus(in, p, kQt))) quit = quit || i.id == "quit";
    CHECK(quit);
}

// Mobile: a media place the bar leaves out has a submenu in File: its
// picker's entries to choose, then Name & Save….
TEST(mobile_media_submenu_chooses_and_names_and_saves) {
    auto in = st::inputs();
    in.media = st::mediaWithPlace();
    auto p = st::product();
    auto ms = menus(in, p, mobile());
    const MenuItem* sub = findItem(ms, "media-card");
    CHECK(sub != nullptr);
    CHECK(sub->kind == MenuItem::Kind::Submenu);
    CHECK_EQ(sub->label, std::string("Test card"));
    CHECK_EQ(join(ids(sub->children)), std::string("media-card-0 media-card-1 media-card-2 - media-name-and-save-card"));
    const auto& empty = sub->children[0];
    CHECK_EQ(empty.label, std::string("(empty)"));
    CHECK(empty.checked && empty.command.value.empty());   // chosen: sends nothing
    const auto& blank = sub->children[1];
    CHECK_EQ(blank.label, std::string("Blank"));
    CHECK_EQ(blank.command.value, std::string("media.choose"));
    CHECK(blank.args == Value::object({{"place", "card"}, {"entry", "bundled:Blank.test-card.yaml"}}));
    CHECK(blank.enabled);
    CHECK(!sub->children[2].enabled);   // cannot be chosen now: its hint
    CHECK_EQ(sub->children[2].reason, std::string("In the other reader."));
    const auto& save = sub->children[4];
    CHECK_EQ(save.label, std::string("Name & Save…"));
    CHECK(save.command.value.empty());   // the shell's action
    CHECK(save.args == Value::object({{"place", "card"}}));
    CHECK(!save.enabled);
    CHECK_EQ(save.reason, std::string("Nothing to save."));
    // In More too, in File's media group.
    bool inMore = false;
    for (const auto& i : moreItems(ms)) inMore = inMore || i.id == "media-card";
    CHECK(inMore);
    // The place's `insert` not available: every entry disabled with its reason.
    auto cs = st::controlState();
    cs.mediaPlaces[0].insert = st::unavailable("A card passes.");
    in.controlState = cs.toValue();
    sub = findItem(menus(in, p, mobile()), "media-card");
    CHECK(!sub->children[1].enabled);
    CHECK_EQ(sub->children[1].reason, std::string("A card passes."));
    // A medium the bar keeps, or desktop: no submenu.
    in = st::inputs();
    in.media = st::mediaWithPlace();
    CHECK(findItem(menus(in, p, kWeb), "media-card") == nullptr);
    p.mobileBarMedia = {"test-card"};
    CHECK(findItem(menus(in, p, mobile()), "media-card") == nullptr);
}

TEST(more_cold_boot_asks_first) {
    const auto p = st::product();
    const auto ms = menus(st::inputs(), p, mobile());
    const auto more = moreItems(ms);
    const MenuItem* cold = nullptr;
    for (const auto& i : more)
        if (i.id == "cold-boot") cold = &i;
    CHECK(cold != nullptr);
    CHECK_EQ(cold->confirm, std::string("Cold boot the machine? What it holds in memory is lost."));
    CHECK(cold->destructive);
    CHECK_EQ(cold->command.value, std::string("cold-boot"));
    // In the menus themselves the item asks nothing.
    CHECK(findItem(ms, "cold-boot")->confirm.empty());
    CHECK(!findItem(ms, "cold-boot")->destructive);
    for (const auto& i : more)
        if (i.id != "cold-boot") CHECK(i.confirm.empty());
}

TEST(more_follows_availability) {
    auto cs = st::controlState();
    for (auto& a : cs.actions)
        if (a.command == control::kColdBootAction) a.available = st::unavailable("An operation runs.");
    const auto p = st::product();
    auto more = moreItems(menus(st::inputs(cs.toValue()), p, mobile()));
    auto byId = [&](const char* id) -> const MenuItem* {
        for (const auto& i : more)
            if (i.id == id) return &i;
        return nullptr;
    };
    CHECK(!byId("cold-boot")->enabled);
    CHECK_EQ(byId("cold-boot")->reason, std::string("An operation runs."));
    CHECK(!byId("resume")->enabled);   // not paused
    CHECK(!byId("disk-thing")->enabled);
    CHECK_EQ(byId("disk-thing")->reason, std::string("No disk drive."));
    // Computed again from new inputs: available now, and Resume while paused.
    auto in = st::inputs();
    in.runState = Value::object({{"paused", true}});
    more = moreItems(menus(in, p, mobile()));
    CHECK(byId("cold-boot")->enabled);
    CHECK(byId("resume")->enabled);
}

TEST(media_place_item_only_with_its_media_place) {
    auto p = st::product();
    ProductItem fit{MenuGroup::FileMedia, "arm-card", "Arm Card…", "", {}};
    fit.mediaPlace = "card";
    p.items.push_back(fit);
    // Model-A lists the media place `card`: the item, enabled.
    auto ms = menus(st::inputs(), p, kQt);
    CHECK(findItem(ms, "arm-card") != nullptr);
    CHECK(findItem(ms, "arm-card")->enabled);
    // Its `insert` not available: disabled, with the reason.
    auto cs = st::controlState();
    cs.mediaPlaces[0].insert = st::unavailable("A card passes.");
    ms = menus(st::inputs(cs.toValue()), p, kQt);
    CHECK(!findItem(ms, "arm-card")->enabled);
    CHECK_EQ(findItem(ms, "arm-card")->reason, std::string("A card passes."));
    // Model-B lists none: no item.
    ms = menus(st::inputs(st::controlValue("Model-B")), p, kQt);
    CHECK(findItem(ms, "arm-card") == nullptr);
    // checkShellProduct names a media place that no model lists.
    CHECK(checkShellProduct(p, std::vector<Value>{st::controlValue("Model-A"), st::controlValue("Model-B")}).empty());
    p.items.back().mediaPlace = "cassette";
    const auto problems = checkShellProduct(p, std::vector<Value>{st::controlValue("Model-A"), st::controlValue("Model-B")});
    CHECK_EQ(problems.size(), std::size_t{1});
}

TEST(no_settings_item_without_settings) {   // and Traits::about
    Traits t = kWeb;
    t.settings = false;
    auto ms = menus(st::inputs(), st::product(), t);
    CHECK(findItem(ms, "settings") == nullptr);
    CHECK(findItem(ms, "about") != nullptr);
    t.about = false;
    ms = menus(st::inputs(), st::product(), t);
    CHECK(findItem(ms, "about") == nullptr);
    CHECK_EQ(join(ids(findMenu(ms, "help")->items)), std::string("help"));
}

// A native frontend's own toggles and submenus among the product's items.
TEST(product_toggle_and_submenu) {
    auto p = st::product();
    bool tracing = true;
    ProductItem trace{MenuGroup::MachineEnd, "trace", "Trace", "", {}};
    trace.checked = [&tracing] { return tracing; };
    ProductItem log{MenuGroup::MachineEnd, "log", "Log", "", {}};
    for (const char* id : {"log-off", "log-info"}) {
        ProductItem level{MenuGroup::MachineEnd, id, id, "", {}};
        level.checked = [id] { return std::string(id) == "log-info"; };
        log.children.push_back(level);
    }
    p.items.push_back(trace);
    p.items.push_back(log);
    auto ms = menus(st::inputs(), p, kQt);
    const MenuItem* t = findItem(ms, "trace");
    CHECK(t && t->checkable && t->checked);
    const MenuItem* l = findItem(ms, "log");
    CHECK(l && l->kind == MenuItem::Kind::Submenu);
    CHECK_EQ(join(ids(l->children)), std::string("log-off log-info"));
    CHECK(!findItem(ms, "log-off")->checked);
    CHECK(findItem(ms, "log-info")->checked);
    tracing = false;
    ms = menus(st::inputs(), p, kQt);
    CHECK(!findItem(ms, "trace")->checked);
}
