#include "harness/Reply.hpp"
#include <cmath>
#include <cstdio>

namespace calcu::harness {

std::string toText(const Decimal& d) {
    if (d.places <= 0) return std::to_string(d.units);
    const bool negative = d.units < 0;
    std::uint64_t u = negative ? static_cast<std::uint64_t>(-(d.units + 1)) + 1 : static_cast<std::uint64_t>(d.units);
    std::uint64_t scale = 1;
    for (int i = 0; i < d.places; ++i) scale *= 10;
    std::string frac = std::to_string(u % scale);
    frac.insert(0, static_cast<std::size_t>(d.places) - frac.size(), '0');
    return (negative ? "-" : "") + std::to_string(u / scale) + "." + frac;
}

Decimal secondsOf(std::uint64_t ticks, std::uint32_t rate) {
    if (rate == 0) return Decimal{0, 9};
    constexpr std::uint64_t kNano = 1'000'000'000;
    const std::uint64_t whole = ticks / rate, rest = ticks % rate;
    // rest < rate < 2^32, so rest × 10^9 fits in 64 bits.
    return Decimal{static_cast<std::int64_t>(whole * kNano + rest * kNano / rate), 9};
}

Decimal fraction3(double v) { return Decimal{static_cast<std::int64_t>(std::llround(v * 1000.0)), 3}; }

Value::Value() = default;
Value::Value(std::nullptr_t) {}
Value::Value(bool b) : v_(b) {}
Value::Value(int i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(long i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(long long i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(unsigned i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(unsigned long i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(unsigned long long i) : v_(static_cast<std::int64_t>(i)) {}
Value::Value(Decimal d) : v_(d) {}
Value::Value(std::string s) : v_(std::move(s)) {}
Value::Value(const char* s) : v_(std::string(s)) {}
Value::Value(Array a) : v_(std::move(a)) {}
Value::Value(Object o) : v_(std::move(o)) {}
Value::Value(const Value&) = default;
Value::Value(Value&&) noexcept = default;
Value& Value::operator=(const Value&) = default;
Value& Value::operator=(Value&&) noexcept = default;
Value::~Value() = default;
bool Value::isNull() const { return std::holds_alternative<std::nullptr_t>(v_); }
const std::string* Value::string() const { return std::get_if<std::string>(&v_); }
const std::int64_t* Value::integer() const { return std::get_if<std::int64_t>(&v_); }
const bool* Value::boolean() const { return std::get_if<bool>(&v_); }
const Value::Array* Value::elements() const { return std::get_if<Array>(&v_); }
const Value::Object* Value::members() const { return std::get_if<Object>(&v_); }
Value Value::object() { return Value(Object{}); }
Value Value::array() { return Value(Array{}); }

Value& Value::set(std::string key, Value v) {
    if (!std::holds_alternative<Object>(v_)) v_ = Object{};
    auto& o = std::get<Object>(v_);
    for (auto& m : o)
        if (m.key == key) {
            m.value = std::move(v);
            return *this;
        }
    o.push_back(Member{std::move(key), std::move(v)});
    return *this;
}

Value& Value::push(Value v) {
    if (!std::holds_alternative<Array>(v_)) v_ = Array{};
    std::get<Array>(v_).push_back(std::move(v));
    return *this;
}

const Value* Value::find(const std::string& key) const {
    if (const auto* o = members())
        for (const auto& m : *o)
            if (m.key == key) return &m.value;
    return nullptr;
}

bool operator==(const Value& a, const Value& b) { return a.v_ == b.v_; }

namespace {
void escape(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", c);
                out += buf;
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    out += '"';
}

struct JsonWriter {
    std::string& out;
    void operator()(std::nullptr_t) const { out += "null"; }
    void operator()(bool b) const { out += b ? "true" : "false"; }
    void operator()(std::int64_t i) const { out += std::to_string(i); }
    void operator()(const Decimal& d) const { out += toText(d); }
    void operator()(const std::string& s) const { escape(out, s); }
    void operator()(const Value::Array& a) const {
        out += '[';
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (i) out += ',';
            out += writeJson(a[i]);
        }
        out += ']';
    }
    void operator()(const Value::Object& o) const {
        out += '{';
        for (std::size_t i = 0; i < o.size(); ++i) {
            if (i) out += ',';
            escape(out, o[i].key);
            out += ':';
            out += writeJson(o[i].value);
        }
        out += '}';
    }
};

// A reason is one line.
std::string oneLine(std::string s) {
    for (char& c : s)
        if (c == '\n' || c == '\r') c = ' ';
    return s;
}
} // namespace

std::string writeJson(const Value& v) {
    std::string out;
    std::visit(JsonWriter{out}, v.v_);
    return out;
}

Reply doneReply(Value result, std::vector<std::string> lines) {
    Reply r;
    r.done = true;
    r.result = std::move(result);
    r.lines = std::move(lines);
    return r;
}

Reply refusedReply(std::string reason) {
    Reply r;
    r.done = false;
    r.reason = std::move(reason);
    return r;
}

std::string writePlain(const Reply& r) {
    if (!r.done) return "refused: " + oneLine(r.reason) + "\n";
    if (r.lines.empty()) return "done\n";
    std::string out = "done " + std::to_string(r.lines.size()) + "\n";
    for (const auto& l : r.lines) out += oneLine(l) + "\n";
    return out;
}

std::string writeJson(const Reply& r) {
    Value v = Value::object();
    v.set("done", r.done);
    v.set("result", r.done ? r.result : Value());
    v.set("reason", r.done ? Value() : Value(oneLine(r.reason)));
    return writeJson(v) + "\n";
}

} // namespace calcu::harness
