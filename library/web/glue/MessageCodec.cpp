#include "web/glue/MessageCodec.hpp"
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

using emscripten::val;

namespace calcu::web {

namespace {

val bytesToJs(const std::uint8_t* data, std::size_t n, val& transfer) {
    val array = val::global("Uint8Array").new_(n);
    if (n) array.call<void>("set", val(emscripten::typed_memory_view(n, data)));
    transfer.call<void>("push", array["buffer"]);
    return array;
}

val payloadToJs(const sessions::PayloadPtr& p, val& transfer) {
    return p ? toJs(p->toValue(), transfer) : val::null();
}

} // namespace

val toJs(const sessions::Value& v, val transfer) {
    if (v.isNull()) return val::null();
    if (const bool* b = v.boolean()) return val(*b);
    if (const std::int64_t* i = v.integer()) return val(static_cast<double>(*i));
    if (const double* d = v.real()) return val(*d);
    if (const std::string* s = v.string()) return val(*s);
    if (const auto* b = v.bytes()) return bytesToJs(b->data(), b->size(), transfer);
    if (const auto* a = v.elements()) {
        val out = val::array();
        for (const auto& e : *a) out.call<void>("push", toJs(e, transfer));
        return out;
    }
    val out = val::object();
    if (const auto* m = v.members())
        for (const auto& e : *m) out.set(e.key, toJs(e.value, transfer));
    return out;
}

sessions::Value fromJs(const val& v) {
    if (v.isNull() || v.isUndefined()) return sessions::Value();
    const std::string type = v.typeOf().as<std::string>();
    if (type == "boolean") return sessions::Value(v.as<bool>());
    if (type == "number") {
        const double d = v.as<double>();
        if (std::isfinite(d) && std::floor(d) == d && std::fabs(d) < 9.0e15)
            return sessions::Value(static_cast<std::int64_t>(d));
        return sessions::Value(d);
    }
    if (type == "string") return sessions::Value(v.as<std::string>());
    if (type != "object") return sessions::Value();
    if (v.instanceof(val::global("Uint8Array")))
        return sessions::Value(emscripten::convertJSArrayToNumberVector<std::uint8_t>(v));
    if (v.instanceof(val::global("ArrayBuffer")))
        return sessions::Value(emscripten::convertJSArrayToNumberVector<std::uint8_t>(val::global("Uint8Array").new_(v)));
    if (val::global("Array").call<bool>("isArray", v)) {
        sessions::Value::Array out;
        const int n = v["length"].as<int>();
        for (int i = 0; i < n; ++i) out.push_back(fromJs(v[i]));
        return sessions::Value(std::move(out));
    }
    sessions::Value::Object out;
    val keys = val::global("Object").call<val>("keys", v);
    const int n = keys["length"].as<int>();
    for (int i = 0; i < n; ++i) {
        const std::string key = keys[i].as<std::string>();
        out.push_back(sessions::Value::Member{key, fromJs(v[key])});
    }
    return sessions::Value(std::move(out));
}

val encode(const sessions::Message& message, val transfer) {
    struct V {
        val& transfer;
        val operator()(const sessions::StateMessage& m) const {
            val o = val::object();
            o.set("kind", "state");
            o.set("topic", m.topic.value);
            o.set("value", payloadToJs(m.state, transfer));
            return o;
        }
        val operator()(const sessions::StreamMessage& m) const {
            val o = val::object();
            o.set("kind", "stream");
            o.set("topic", m.topic.value);
            o.set("reset", m.reset);
            val items = val::array();
            for (const auto& p : m.appended) items.call<void>("push", payloadToJs(p, transfer));
            o.set("value", items);
            return o;
        }
        val operator()(const sessions::ReplyMessage& m) const {
            val o = val::object();
            o.set("kind", "reply");
            o.set("call", static_cast<double>(m.call));
            o.set("done", m.done);
            o.set("reason", m.reason);
            o.set("value", payloadToJs(m.result, transfer));
            return o;
        }
        val operator()(const sessions::ServiceAsk& m) const {
            val o = val::object();
            o.set("kind", "ask");
            o.set("call", static_cast<double>(m.ask));
            o.set("name", std::string(sessions::serviceName(m.service)));
            o.set("value", toJs(m.args, transfer));
            return o;
        }
        val operator()(const sessions::SoundStart& m) const {
            val o = val::object();
            o.set("kind", "sound-start");
            o.set("value", m.sampleRate);
            return o;
        }
        val operator()(const sessions::SoundSamples& m) const {
            val pcm = val::global("Int16Array").new_(m.mono.size());
            if (!m.mono.empty())
                pcm.call<void>("set", val(emscripten::typed_memory_view(m.mono.size(), m.mono.data())));
            transfer.call<void>("push", pcm["buffer"]);
            val o = val::object();
            o.set("kind", "sound");
            o.set("value", pcm);
            return o;
        }
        val operator()(const sessions::SoundStop&) const {
            val o = val::object();
            o.set("kind", "sound-stop");
            return o;
        }
    };
    return std::visit(V{transfer}, message);
}

} // namespace calcu::web
