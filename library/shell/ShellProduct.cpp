#include "shell/ShellProduct.hpp"
#include "shell/Inputs.hpp"

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

} // namespace calcu::shell
