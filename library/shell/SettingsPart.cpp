#include "shell/SettingsPart.hpp"
#include "settings/OpenFolders.hpp"
#include "shell/Inputs.hpp"
#include <cmath>
#include <utility>

namespace calcu::shell {

namespace {

using sessions::Value;
using settings::KeyDecl;
using settings::ValueKind;


const char* kindName(const KeyDecl& d) {
    if (d.section == "folders") return "folder";
    switch (d.kind) {
    case ValueKind::Text: return "text";
    case ValueKind::Path: return "path";
    case ValueKind::Boolean: return "boolean";
    case ValueKind::Integer: return "integer";
    case ValueKind::Choice: return "choice";
    case ValueKind::ModelOrLast: return "model-or-last";
    case ValueKind::ModelName: return "model-name";
    }
    return "text";
}

Value textList(const std::vector<std::string>& xs) {
    Value::Array a;
    for (const auto& x : xs) a.push_back(Value(x));
    return Value(std::move(a));
}

// A value as the file writes it: a page's numbers and flags as text.
std::optional<std::string> valueText(const Value& v) {
    if (const auto* s = v.string()) return *s;
    if (const auto* i = v.integer()) return std::to_string(*i);
    if (const auto* b = v.boolean()) return std::string(*b ? "true" : "false");
    if (const auto* r = v.real(); r && std::floor(*r) == *r) return std::to_string(static_cast<long long>(*r));
    return std::nullopt;
}

} // namespace

SettingsPart::SettingsPart(SettingsPartSetup s) : setup_(std::move(s)) {}

void SettingsPart::attach(sessions::SessionSetup& s) {
    topic_ = &s.stateTopic(kSettingsTopic);
    topic_->set(sessions::valuePayload("settings", state()));
    s.command(commands::kSetSetting, [this](const Value& a, sessions::SessionContext& c) { return set(a, c); });
    s.command(commands::kRememberFolder, [this](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
        const Value* kind = a.find("kind");
        const Value* folder = a.find("folder");
        if (!kind || !kind->string() || !folder || !folder->string())
            return sessions::Refused{"remember-folder needs kind and folder."};
        settings::rememberOpenFolder(*setup_.file, *kind->string(), *folder->string());
        topic_->set(sessions::valuePayload("settings", state()));
        return sessions::Done{};
    });
}

void SettingsPart::beforeFlush(sessions::SessionContext&) {
    if (topic_) topic_->set(sessions::valuePayload("settings", state()));
}

bool SettingsPart::shown(const KeyDecl& d) const {   // every declared key but [window] and the app's last model
    for (const auto& k : setup_.alsoShown)
        if (k == d.section + "." + d.key) return true;
    return d.section != "window" && !(d.section == "startup" && d.key == "last-model");
}

std::string SettingsPart::inUse(const KeyDecl& d) const {
    if (setup_.inUse)
        if (auto v = setup_.inUse(d.section, d.key)) return *v;
    const auto& f = *setup_.file;
    const std::string home = setup_.paths ? setup_.paths->hostDirs().home : std::string();
    if (d.section == "folders") return settings::openStartFolder(f, d.key, home);
    if (setup_.paths) {
        const auto& p = *setup_.paths;
        if (d.section == "storage" && d.key == "save-folder") return p.saveFolder();
        if (d.section == "storage" && d.key == "cassette-folder") return p.cassetteFolder();
        if (d.section == "trace" && d.key == "folder") return p.traceFolder().value_or("");
        if (d.section == "serial" && d.key == "link-folder") return p.serialLinkFolder().value_or("");
    }
    return f.text(d.section, d.key);
}

Value SettingsPart::state() const {
    Value::Array rows;
    const auto& f = *setup_.file;
    const auto& decl = f.product();
    for (const auto& d : decl.keys) {
        if (!shown(d)) continue;
        const bool folder = d.section == "folders";
        const std::string value = folder ? settings::fixedOpenFolder(f, d.key) : f.text(d.section, d.key);
        const std::string def = folder ? std::string() : d.defaultValue;
        Value::Object row;
        row.push_back({"section", Value(d.section)});
        row.push_back({"key", Value(d.key)});
        row.push_back({"kind", Value(kindName(d))});
        row.push_back({"value", Value(value)});
        row.push_back({"default", Value(def)});
        std::vector<std::string> choices;
        if (d.kind == ValueKind::Choice) choices = d.choices;
        if (d.kind == ValueKind::Boolean) choices = {"true", "false"};
        if (d.kind == ValueKind::ModelOrLast) {
            choices.push_back("last");
            for (const auto& m : decl.models) choices.push_back(m);
        }
        if (d.kind == ValueKind::ModelName) choices = decl.models;
        row.push_back({"choices", textList(choices)});
        if (d.min) row.push_back({"min", Value(static_cast<std::int64_t>(*d.min))});
        if (d.max) row.push_back({"max", Value(static_cast<std::int64_t>(*d.max))});
        row.push_back({"inUse", Value(inUse(d))});
        row.push_back({"isDefault", Value(folder ? !settings::hasFixedOpenFolder(f, d.key) : value == def)});
        rows.push_back(Value(std::move(row)));
    }
    const std::string home = setup_.paths ? setup_.paths->hostDirs().home : std::string();
    return Value::object({{"home", home}, {"rows", Value(std::move(rows))}});
}

sessions::CommandOutcome SettingsPart::set(const Value& args, sessions::SessionContext& c) {
    const Value* sec = args.find("section");
    const Value* key = args.find("key");
    if (!sec || !sec->string() || !key || !key->string()) return sessions::Refused{"set-setting needs section and key."};
    auto& f = *setup_.file;
    const KeyDecl* d = f.product().find(*sec->string(), *key->string());
    if (!d || !shown(*d)) return sessions::Refused{"There is no setting [" + *sec->string() + "] " + *key->string() + "."};
    const Value* v = args.find("value");
    std::string text;
    if (v && !v->isNull()) {
        auto t = valueText(*v);
        if (!t) return sessions::Refused{"The value for [" + d->section + "] " + d->key + " must be text."};
        text = *t;
    } else if (d->section != "folders") {
        text = d->defaultValue;   // null: the default
    }
    settings::SettingsFile::WriteResult w;
    if (d->section == "folders") {
        w = settings::setFixedOpenFolder(f, d->key, text);
    } else {
        if (!settings::accepts(f.product(), *d, text))
            return sessions::Refused{"\"" + text + "\" is not a value for [" + d->section + "] " + d->key + "."};
        w = f.set(d->section, d->key, text);
    }
    if (!w.ok)   // kept for this run, and the user is told
        c.notice("The setting holds until you quit; it was " + w.problem + ".");
    if (setup_.changed) setup_.changed(d->section, d->key);
    topic_->set(sessions::valuePayload("settings", state()));
    return sessions::Done{};
}

} // namespace calcu::shell
