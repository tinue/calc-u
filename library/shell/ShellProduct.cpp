#include "shell/ShellProduct.hpp"
#include "shell/Inputs.hpp"
#include <algorithm>
#include <functional>

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

std::string nameAndSavePrompt(const ShellProduct& p, const std::string& place) {
    auto it = p.nameAndSave.prompts.find(place);
    return it != p.nameAndSave.prompts.end() ? it->second : p.nameAndSave.prompt;
}

namespace {

void eachItem(const std::vector<ProductItem>& items, const std::function<void(const ProductItem&)>& f) {
    for (const auto& i : items) {
        f(i);
        eachItem(i.children, f);
    }
}

} // namespace

std::vector<sessions::TopicName> productTopics(const ShellProduct& p) {
    std::vector<sessions::TopicName> out;
    eachItem(p.items, [&](const ProductItem& i) {
        if (i.checkedBy && std::find(out.begin(), out.end(), i.checkedBy->topic) == out.end()) out.push_back(i.checkedBy->topic);
    });
    return out;
}

void followProductTopics(ShellInputs& in, const ShellProduct& p) {
    for (const auto& t : productTopics(p)) in.productTopics.try_emplace(t.value);
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
    eachItem(p.items, [&](const ProductItem& i) {
        if (i.checkedBy && i.command.value.empty()) problems.push_back("The item " + i.id + " follows a topic and sends no command.");
    });
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
