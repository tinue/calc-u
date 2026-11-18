#include "app/dap/DapConfig.hpp"

#include <memory>
#include <utility>
#include <variant>

#include "Debug/Listing/Listing.hpp"
#include "app/SharpDebugBlock.hpp"
#include "formats/Number.hpp"
#include "formats/Yaml.hpp"

namespace cu1600::dap {

namespace {

// A `debug:` block value as the configuration's JSON. Bare `true`/`false`
// become booleans; every other scalar stays text, read later by numberOf()
// with the preset's rule.
Value fromYaml(const calcu::formats::YamlNode& n) {
    using Type = calcu::formats::YamlNode::Type;
    switch (n.type) {
    case Type::Map: {
        Value::Object o;
        for (const auto& kv : n.map) o.push_back(Value::Member{kv.first, fromYaml(kv.second)});
        return Value(std::move(o));
    }
    case Type::Sequence: {
        Value::Array a;
        for (const auto& item : n.seq) a.push_back(fromYaml(item));
        return Value(std::move(a));
    }
    case Type::Scalar: {
        std::string text, ignored;
        n.asString(&text, &ignored);
        if (n.scalar != text) return Value(std::move(text));   // quoted
        if (text == "true") return Value(true);
        if (text == "false") return Value(false);
        return Value(std::move(text));
    }
    case Type::Null: break;
    }
    return Value();
}

} // namespace

bool effectiveConfig(const Value& args, const calcu::presets::ReadSetup& read, const calcu::presets::PresetBundle* bundle,
                     Value* out, std::string* error) {
    const std::string project = textOf(args, "project");
    if (project.empty()) {
        *out = args;
        return true;
    }
    // The block as the library's reader keeps it.
    const std::string path = debug::absolutePath(project);
    const auto block = debugBlockOf(path, read, bundle);
    if (const auto* why = std::get_if<std::string>(&block)) {
        *error = "Project " + project + ": " + *why;
        return false;
    }
    const auto& debugBlock = std::get<std::shared_ptr<const DebugBlock>>(block);
    Value::Object merged;
    if (debugBlock && debugBlock->node.isMap()) merged = membersOf(fromYaml(debugBlock->node));
    put(merged, "preset", Value(path));
    for (const Value::Member& m : membersOf(args)) {
        const Value* base = nullptr;
        for (const Value::Member& b : merged)
            if (b.key == m.key) base = &b.value;
        if ((m.key == "listings" || m.key == "symbols") && isArray(base) && isArray(&m.value)) {
            Value::Array both = *base->elements();
            for (const Value& v : *m.value.elements()) both.push_back(v);
            put(merged, m.key, Value(std::move(both)));
        } else if (m.key == "program" && isObject(base)) {
            Value::Object program = membersOf(*base);
            for (const Value::Member& p : membersOf(m.value)) put(program, p.key, p.value);
            put(merged, m.key, Value(std::move(program)));
        } else {
            put(merged, m.key, m.value);
        }
    }
    *out = Value(std::move(merged));
    return true;
}

bool numberOf(const Value& v, std::uint32_t max, std::uint32_t* out) {
    std::int64_t n = 0;
    if (const std::int64_t* i = v.integer()) {
        n = *i;
    } else if (const double* d = v.real()) {
        if (*d < 0 || *d != static_cast<double>(static_cast<std::int64_t>(*d))) return false;
        n = static_cast<std::int64_t>(*d);
    } else if (const std::string* s = v.string()) {
        const auto parsed = calcu::formats::parseNumber(*s);
        if (!parsed) return false;
        n = *parsed;
    } else {
        return false;
    }
    if (n < 0 || n > static_cast<std::int64_t>(max)) return false;
    *out = static_cast<std::uint32_t>(n);
    return true;
}

debug::BankKey bankKeyOf(const Value& o) {
    const auto qualifier = [&o](const char* name) {
        const Value* v = o.find(name);
        std::uint32_t n = 0;
        const std::uint32_t max = debug::attach::find(debug::attach::kListingKeys, name)->max;
        return v && numberOf(*v, max, &n) ? static_cast<int>(n) : -1;
    };
    debug::BankKey k;
    k.bank = qualifier("bank");
    k.me = qualifier("me");
    k.pu = qualifier("pu");
    k.pv = qualifier("pv");
    return k;
}

bool checkAttach(const Value& o, const debug::attach::Table& table, std::string* error) {
    using namespace debug::attach;
    for (const Key& key : table) {
        const Value* v = o.find(key.name);
        if (!v || v->isNull()) continue;
        const auto bad = [&](const std::string& what) {
            *error = std::string("Bad \"") + key.name + "\": " + what;
            return false;
        };
        std::string why;
        switch (key.kind) {
        case Kind::Map:
            if (!v->members()) return bad("a block of settings");
            if (!checkAttach(*v, kProgramKeys, error)) return false;
            break;
        case Kind::FileList:
            if (!v->elements()) return bad("a list of paths or {path, ...} objects");
            for (const Value& item : *v->elements()) {
                if (item.string()) continue;
                if (!item.members()) return bad("a list of paths or {path, ...} objects");
                if (!checkAttach(item, itemKeys(key), error)) return false;
            }
            break;
        case Kind::PathList:
            if (!v->elements()) return bad("a list of paths");
            for (const Value& item : *v->elements())
                if (!item.string()) return bad("a list of paths");
            break;
        case Kind::Bool:
            if (!v->boolean()) return bad("true or false");
            break;
        case Kind::Number: {
            std::uint32_t n = 0;
            if (!numberOf(*v, key.max, &n)) return bad(describe(key));
            break;
        }
        case Kind::Choice:
            if (!v->string()) return bad("text");
            if (!checkScalar(key, *v->string(), &why)) return bad(why);
            break;
        case Kind::Text:   // `entry` may be a number
            if (!v->string() && !v->integer() && !v->real()) return bad("text");
            break;
        case Kind::Path:
            if (!v->string()) return bad("a path");
            break;
        }
    }
    return true;
}

} // namespace cu1600::dap
