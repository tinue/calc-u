#include "shell/ShellProduct.hpp"
#include "shell/Inputs.hpp"
#include <algorithm>

namespace calcu::shell {

AboutModel about(const ShellProduct& p) {
    AboutModel a;
    a.title = p.aboutLabel.empty() ? "About " + p.name : p.aboutLabel;
    // "About Calc-U-1600…" as a menu label: the dialog's title has no ellipsis.
    const std::string ellipsis = "\xE2\x80\xA6";
    if (a.title.size() >= ellipsis.size() && a.title.compare(a.title.size() - ellipsis.size(), ellipsis.size(), ellipsis) == 0)
        a.title.resize(a.title.size() - ellipsis.size());
    a.name = p.name;
    a.version = "Version " + p.version + (p.build.empty() ? std::string() : " (" + p.build + ")");
    a.lines = p.aboutLines;
    a.links = p.aboutLinks;
    return a;
}

std::vector<std::string> checkShellProduct(const ShellProduct& p, const sessions::Value& cs) {
    std::vector<std::string> problems;
    if (p.name.empty()) problems.push_back("The product has no name.");
    if (p.helpUrl.empty()) problems.push_back("The product has no help address.");
    auto needsLabel = [&](const std::string& id) {
        if (!p.variantLabels.count(id)) problems.push_back(id + " has variants and no menu label.");
    };
    if (read::elements(cs, "variants").size() > 1) needsLabel(read::text(cs, "model"));
    for (const auto& place : read::elements(cs, "places"))
        for (const auto& f : read::elements(place, "fitting"))
            if (read::elements(f, "variants").size() > 1) needsLabel(read::text(f, "id"));
    std::set<std::string> ids;
    for (const auto& item : p.items)
        if (!ids.insert(item.id).second) problems.push_back("The item id " + item.id + " is given twice.");
    return problems;
}

std::vector<std::string> checkShellProduct(const ShellProduct& p, const std::vector<sessions::Value>& controlStates) {
    std::vector<std::string> problems;
    for (const auto& cs : controlStates)
        for (auto& problem : checkShellProduct(p, cs))
            if (std::find(problems.begin(), problems.end(), problem) == problems.end()) problems.push_back(std::move(problem));
    for (const auto& item : p.items) {   // a media place no model lists
        if (item.mediaPlace.empty()) continue;
        bool listed = false;
        for (const auto& cs : controlStates)
            for (const auto& m : read::elements(cs, "mediaPlaces")) listed = listed || read::text(m, "id") == item.mediaPlace;
        if (!listed) problems.push_back("The item " + item.id + " names the media place " + item.mediaPlace + ", which no model lists.");
    }
    return problems;
}

} // namespace calcu::shell
