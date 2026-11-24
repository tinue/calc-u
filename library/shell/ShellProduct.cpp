#include "shell/ShellProduct.hpp"
#include "shell/Inputs.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

namespace calcu::shell {

AboutModel about(const ShellProduct& p, const SupportRows& host, const DisplayFacts& display) {
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
    SupportRows support;
    if (!p.package.empty()) support.emplace_back("Package", p.package);
    if (!p.buildTime.empty()) support.emplace_back("Built", p.buildTime);
    for (const auto& row : host)
        if (!row.second.empty()) support.push_back(row);
    for (AboutSection s : {AboutSection{"Support Information", std::move(support)},
                           AboutSection{"Display", displayRows(display)}})
        if (!s.rows.empty()) a.sections.push_back(std::move(s));
    return a;
}

namespace {

std::string sizeText(DisplaySize s) {
    return std::to_string(s.width) + " \xC3\x97 " + std::to_string(s.height);   // U+00D7 ×
}

bool known(DisplaySize s) { return s.width > 0 && s.height > 0; }

// "2", "1.25": at most two decimals, no trailing zeros.
std::string scaleText(double scale) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2f", scale);
    std::string out = buf;
    while (out.back() == '0') out.pop_back();
    if (out.back() == '.') out.pop_back();
    return out;
}

} // namespace

SupportRows displayRows(const DisplayFacts& d) {
    SupportRows out;
    if (!d.windowing.empty()) out.emplace_back("Windowing", d.windowing);
    if (!d.screen.empty()) out.emplace_back("Screen", d.screen);
    if (known(d.reported)) out.emplace_back("Reported", sizeText(d.reported));
    DisplaySize actual;
    if (d.scale > 0) {
        out.emplace_back("Scale", scaleText(d.scale));
        if (known(d.reported)) {
            actual = {static_cast<int>(std::lround(d.reported.width * d.scale)),
                      static_cast<int>(std::lround(d.reported.height * d.scale))};
            out.emplace_back("Actual", sizeText(actual));
        }
    }
    if (known(d.panel) && (d.panel.width != actual.width || d.panel.height != actual.height))
        out.emplace_back("Panel", sizeText(d.panel));
    if (known(d.window)) out.emplace_back("Window", sizeText(d.window));
    return out;
}

std::string aboutText(const AboutModel& a) {
    std::string out = a.name + "\n" + a.version + "\n";
    for (const auto& line : a.lines) out += line + "\n";
    for (const auto& s : a.sections) {
        out += "\n" + s.heading + "\n";
        for (const auto& [label, value] : s.rows) out += label + ": " + value + "\n";
    }
    return out;
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
    eachItem(p.items, [&](const ProductItem& i) {
        if (!ids.insert(i.id).second) problems.push_back("The item id " + i.id + " is given twice.");
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
