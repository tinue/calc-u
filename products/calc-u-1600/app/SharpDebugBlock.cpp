#include "app/SharpDebugBlock.hpp"

#include "Debug/AttachKeys.hpp"
#include "presets/Source.hpp"

namespace shared {

namespace {

using calcu::formats::YamlNode;
using calcu::presets::ReadContext;
using namespace debug::attach;

struct Checker {
    ReadContext& ctx;
    int blockLine;
    bool ok = true;

    void error(const YamlNode& at, const std::string& what) {
        ctx.error(at.line ? at.line : blockLine, "debug: " + what);
        ok = false;
    }

    void path(YamlNode& n) {
        std::string value, e;
        if (!n.isScalar() || !n.asString(&value, &e)) return error(n, "expected a path.");
        n.scalar = ctx.source().resolvedName(value);
    }

    void map(YamlNode& m, const Table& table) {
        for (auto& [name, v] : m.map) {
            const Key* key = find(table, name);
            if (!key) {
                error(v.line ? v : m, "unknown key " + name + ".");
                continue;
            }
            value(name, *key, v);
        }
    }

    void value(const std::string& name, const Key& key, YamlNode& v) {
        switch (key.kind) {
            case Kind::Map:
                if (!v.isMap()) return error(v, name + ": takes a block of settings.");
                map(v, kProgramKeys);
                if (!v.has("bin")) error(v, name + ": needs at least bin.");
                return;
            case Kind::FileList:
                if (!v.isSeq()) return error(v, name + ": expects a list.");
                for (YamlNode& item : v.seq) {
                    if (item.isScalar()) path(item);
                    else if (!item.isMap() || !item.has("path")) error(item, "expected a path, or a map with path.");
                    else map(item, itemKeys(key));
                }
                return;
            case Kind::PathList:
                if (!v.isSeq()) return error(v, name + ": expects a list of paths.");
                for (YamlNode& item : v.seq) path(item);
                return;
            case Kind::Path: return path(v);
            default: break;
        }
        std::string text, e, why;
        if (!v.isScalar()) return error(v, name + " takes a single value.");
        v.asString(&text, &e);
        // A quoted "true" would reach the debugger as text, not a truth value.
        const bool quotedBool = key.kind == Kind::Bool && text != v.scalar;
        if (quotedBool || !checkScalar(key, text, &why)) {
            if (quotedBool) why = "true or false, without quotes";
            error(v, name + " is " + why + ", not " + text + ".");
        }
    }
};

} // namespace

std::shared_ptr<const DebugBlock> readDebugBlock(const YamlNode& block, int line, ReadContext& ctx) {
    auto out = std::make_shared<DebugBlock>();
    out->node = block;
    Checker c{ctx, line};
    if (!block.isMap()) {
        ctx.error(line, "debug: takes a block of settings.");
        return nullptr;
    }
    c.map(out->node, kBlockKeys);
    return c.ok ? out : nullptr;
}

std::variant<std::shared_ptr<const DebugBlock>, std::string> debugBlockOf(const std::string& ref,
                                                                          const calcu::presets::ReadSetup& setup,
                                                                          const calcu::presets::PresetBundle* bundle) {
    const auto source = calcu::presets::sourceFor(ref, bundle, {});
    auto read = calcu::presets::readPreset(*source, setup);
    if (auto* e = std::get_if<calcu::presets::ReadError>(&read)) return e->message;
    const auto& plan = std::get<calcu::presets::LoadPlan>(read);
    const auto it = plan.productHeader.find(kDebugKey);
    if (it == plan.productHeader.end()) return std::shared_ptr<const DebugBlock>{};
    return std::dynamic_pointer_cast<const DebugBlock>(it->second);
}

} // namespace shared
