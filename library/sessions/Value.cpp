#include "sessions/Value.hpp"
#include <cstdio>

namespace calcu::sessions {

Value::Value() : v_(nullptr) {}
Value::Value(std::nullptr_t) : v_(nullptr) {}
Value::Value(bool b) : v_(b) {}
Value::Value(Int i) : v_(i.v) {}
Value::Value(double d) : v_(d) {}
Value::Value(std::string s) : v_(std::move(s)) {}
Value::Value(const char* s) : v_(std::string(s)) {}
Value::Value(Bytes b) : v_(std::move(b)) {}
Value::Value(Array a) : v_(std::move(a)) {}
Value::Value(Object o) : v_(std::move(o)) {}
Value Value::object(std::initializer_list<Member> members) { return Value(Object(members)); }

Value::Value(const Value&) = default;
Value::Value(Value&&) noexcept = default;
Value& Value::operator=(const Value&) = default;
Value& Value::operator=(Value&&) noexcept = default;
Value::~Value() = default;

bool Value::isNull() const { return std::holds_alternative<std::nullptr_t>(v_); }
const bool* Value::boolean() const { return std::get_if<bool>(&v_); }
const std::int64_t* Value::integer() const { return std::get_if<std::int64_t>(&v_); }
const double* Value::real() const { return std::get_if<double>(&v_); }
const std::string* Value::string() const { return std::get_if<std::string>(&v_); }
const Value::Bytes* Value::bytes() const { return std::get_if<Bytes>(&v_); }
const Value::Array* Value::elements() const { return std::get_if<Array>(&v_); }
const Value::Object* Value::members() const { return std::get_if<Object>(&v_); }

const Value* Value::find(std::string_view key) const {
    if (const Object* o = members())
        for (const Member& m : *o)
            if (m.key == key) return &m.value;
    return nullptr;
}

bool operator==(const Value& a, const Value& b) {
    if (a.v_.index() != b.v_.index()) return false;
    if (const auto* o = a.members()) {
        const auto& p = *b.members();
        if (o->size() != p.size()) return false;
        for (std::size_t i = 0; i < o->size(); ++i)
            if ((*o)[i].key != p[i].key || !((*o)[i].value == p[i].value)) return false;
        return true;
    }
    if (const auto* x = a.elements()) {
        const auto& y = *b.elements();
        if (x->size() != y.size()) return false;
        for (std::size_t i = 0; i < x->size(); ++i)
            if (!((*x)[i] == y[i])) return false;
        return true;
    }
    return std::visit(
        [&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Value::Array> || std::is_same_v<T, Value::Object>) return false;
            else return x == std::get<T>(b.v_);
        },
        a.v_);
}

namespace {
void quote(std::string& out, const std::string& s) {
    out += '"';
    for (char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
            out += buf;
        } else {
            out += c;
        }
    }
    out += '"';
}

void write(std::string& out, const Value& v) {
    if (v.isNull()) out += "null";
    else if (const bool* b = v.boolean()) out += *b ? "true" : "false";
    else if (const std::int64_t* i = v.integer()) out += std::to_string(*i);
    else if (const double* d = v.real()) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.17g", *d);
        out += buf;
    } else if (const std::string* s = v.string()) quote(out, *s);
    else if (const Value::Bytes* by = v.bytes()) {
        static const char* kHex = "0123456789abcdef";
        out += "\"0x";
        for (std::uint8_t x : *by) {
            out += kHex[x >> 4];
            out += kHex[x & 15];
        }
        out += '"';
    } else if (const Value::Array* a = v.elements()) {
        out += '[';
        for (std::size_t k = 0; k < a->size(); ++k) {
            if (k) out += ',';
            write(out, (*a)[k]);
        }
        out += ']';
    } else if (const Value::Object* o = v.members()) {
        out += '{';
        for (std::size_t k = 0; k < o->size(); ++k) {
            if (k) out += ',';
            quote(out, (*o)[k].key);
            out += ':';
            write(out, (*o)[k].value);
        }
        out += '}';
    }
}
} // namespace

std::string toText(const Value& v) {
    std::string out;
    write(out, v);
    return out;
}

} // namespace calcu::sessions
