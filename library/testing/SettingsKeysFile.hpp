#pragma once
#include "settings/Declaration.hpp"
#include "settings/WindowPlace.hpp"
#include <set>
#include <sstream>
#include <string>
#include <string_view>

// The text of a product's help/settings-keys.txt, which tells `tools/help`
// which settings keys chapter 8 describes where: one line per key,
// `family [section] key` for the family's (a section whose names the
// product gives, [preset] <model>, [folders] <kind>, [window] <key>, once
// in its general form), `product [section] key` for the product's own and
// for the names it gives such a section. Each product's help test compares
// its file with it.

namespace calcu::testing {

inline std::string settingsKeysFileText(const settings::ProductDecl& decl, std::string_view writtenBy) {
    namespace family = settings::family;
    std::set<std::string> fixed;   // "[section] key"
    auto add = [&](const settings::KeyDecl& d) { fixed.insert("[" + d.section + "] " + d.key); };
    for (const settings::KeyDecl& d :
         {family::startupModel(), family::startupLastModel(), family::saveFolder(), family::cassetteFolder(),
          family::traceFolder(), family::traceMaxFileSizeMb(), family::traceLocation(), family::appLogLevel(),
          family::serialLinkFolder()})
        add(d);
    for (const settings::KeyDecl& d : family::paperWindow()) add(d);
    const std::set<std::string> named{family::presetFor("x").section, family::openFolder("x").section,
                                     family::windowFlag("x", "").section};
    auto general = [](const std::string& section) {
        if (section == "preset") return "<model>";
        if (section == "folders") return "<kind>";
        return "<key>";
    };

    std::ostringstream out;
    out << "# The settings keys " << decl.productName << " declares (app/Settings.cpp, settingsDecl()),\n"
        << "# for tools/help's settings check: each `[section] key` is described\n"
        << "# in chapter 8, the family's in library/help/08-settings.md, the product's in\n"
        << "# help/08-settings.md. Written by " << writtenBy << ".\n";
    std::set<std::string> generalDone;
    for (const settings::KeyDecl& d : decl.keys) {
        const std::string name = "[" + d.section + "] " + d.key;
        if (fixed.count(name)) {
            out << "family " << name << "\n";
            continue;
        }
        if (named.count(d.section) && generalDone.insert(d.section).second)
            out << "family [" << d.section << "] " << general(d.section) << "\n";
        out << "product " << name << "\n";
    }
    return out.str();
}

} // namespace calcu::testing
