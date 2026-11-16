#include "app/dap/DapJson.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace shared::dap {

namespace {

class Parser {
public:
    explicit Parser(std::string_view s) : s_(s) {}

    std::optional<Value> document() {
        std::optional<Value> v = value(0);
        space();
        if (!v || at_ != s_.size()) return std::nullopt;
        return v;
    }

private:
    void space() {
        while (at_ < s_.size() && (s_[at_] == ' ' || s_[at_] == '\t' || s_[at_] == '\n' || s_[at_] == '\r')) ++at_;
    }
    bool take(char c) {
        space();
        if (at_ < s_.size() && s_[at_] == c) {
            ++at_;
            return true;
        }
        return false;
    }
    bool word(std::string_view w) {
        if (s_.substr(at_, w.size()) != w) return false;
        at_ += w.size();
        return true;
    }

    std::optional<Value> value(int depth) {
        if (depth > 64) return std::nullopt;
        space();
        if (at_ >= s_.size()) return std::nullopt;
        const char c = s_[at_];
        if (c == '{') return object(depth);
        if (c == '[') return array(depth);
        if (c == '"') {
            std::optional<std::string> t = text();
            if (!t) return std::nullopt;
            return Value(std::move(*t));
        }
        if (word("true")) return Value(true);
        if (word("false")) return Value(false);
        if (word("null")) return Value();
        return number();
    }

    std::optional<Value> object(int depth) {
        ++at_;   // {
        Value::Object o;
        if (take('}')) return Value(std::move(o));
        for (;;) {
            space();
            std::optional<std::string> key = text();
            if (!key || !take(':')) return std::nullopt;
            std::optional<Value> v = value(depth + 1);
            if (!v) return std::nullopt;
            o.push_back(Value::Member{std::move(*key), std::move(*v)});
            if (take(',')) continue;
            if (take('}')) return Value(std::move(o));
            return std::nullopt;
        }
    }

    std::optional<Value> array(int depth) {
        ++at_;   // [
        Value::Array a;
        if (take(']')) return Value(std::move(a));
        for (;;) {
            std::optional<Value> v = value(depth + 1);
            if (!v) return std::nullopt;
            a.push_back(std::move(*v));
            if (take(',')) continue;
            if (take(']')) return Value(std::move(a));
            return std::nullopt;
        }
    }

    static void utf8(std::string& out, std::uint32_t cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    std::optional<std::uint32_t> hex4() {
        if (at_ + 4 > s_.size()) return std::nullopt;
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = s_[at_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<std::uint32_t>(c - 'A' + 10);
            else return std::nullopt;
        }
        return v;
    }

    std::optional<std::string> text() {
        if (at_ >= s_.size() || s_[at_] != '"') return std::nullopt;
        ++at_;
        std::string out;
        while (at_ < s_.size()) {
            const char c = s_[at_++];
            if (c == '"') return out;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (at_ >= s_.size()) return std::nullopt;
            const char e = s_[at_++];
            switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                std::optional<std::uint32_t> cp = hex4();
                if (!cp) return std::nullopt;
                if (*cp >= 0xD800 && *cp < 0xDC00 && word("\\u")) {   // a surrogate pair
                    std::optional<std::uint32_t> low = hex4();
                    if (!low) return std::nullopt;
                    *cp = 0x10000 + ((*cp - 0xD800) << 10) + (*low - 0xDC00);
                }
                utf8(out, *cp);
                break;
            }
            default: return std::nullopt;
            }
        }
        return std::nullopt;
    }

    std::optional<Value> number() {
        const std::size_t start = at_;
        bool real = false;
        if (at_ < s_.size() && s_[at_] == '-') ++at_;
        while (at_ < s_.size()) {
            const char c = s_[at_];
            if (c >= '0' && c <= '9') {
                ++at_;
            } else if (c == '.' || c == 'e' || c == 'E' || c == '+' || (c == '-' && at_ > start)) {
                real = true;
                ++at_;
            } else {
                break;
            }
        }
        const std::string t(s_.substr(start, at_ - start));
        if (t.empty() || t == "-") return std::nullopt;
        char* end = nullptr;
        if (!real) {
            const long long v = std::strtoll(t.c_str(), &end, 10);
            if (end && *end == '\0') return Value(static_cast<std::int64_t>(v));
            return std::nullopt;
        }
        const double d = std::strtod(t.c_str(), &end);
        if (!end || *end != '\0') return std::nullopt;
        return Value(d);
    }

    std::string_view s_;
    std::size_t at_ = 0;
};

void quote(std::string& out, const std::string& s) {
    out += '"';
    for (const char ch : s) {
        const auto c = static_cast<unsigned char>(ch);
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
                out += ch;
            }
        }
    }
    out += '"';
}

void write(std::string& out, const Value& v) {
    if (v.isNull()) {
        out += "null";
    } else if (const bool* b = v.boolean()) {
        out += *b ? "true" : "false";
    } else if (const std::int64_t* i = v.integer()) {
        out += std::to_string(*i);
    } else if (const double* d = v.real()) {
        if (!std::isfinite(*d)) {
            out += "null";
        } else {
            char buf[40];
            std::snprintf(buf, sizeof buf, "%.17g", *d);
            out += buf;
        }
    } else if (const std::string* s = v.string()) {
        quote(out, *s);
    } else if (const Value::Bytes* bytes = v.bytes()) {
        quote(out, base64(std::string_view(reinterpret_cast<const char*>(bytes->data()), bytes->size())));
    } else if (const Value::Array* a = v.elements()) {
        out += '[';
        bool first = true;
        for (const Value& e : *a) {
            if (!first) out += ',';
            first = false;
            write(out, e);
        }
        out += ']';
    } else if (const Value::Object* o = v.members()) {
        out += '{';
        bool first = true;
        for (const Value::Member& m : *o) {
            if (!first) out += ',';
            first = false;
            quote(out, m.key);
            out += ':';
            write(out, m.value);
        }
        out += '}';
    }
}

constexpr const char* kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

} // namespace

std::optional<Value> parseJson(std::string_view text) { return Parser(text).document(); }

std::string writeJson(const Value& value) {
    std::string out;
    write(out, value);
    return out;
}

const Value* member(const Value& v, std::string_view key) { return v.find(key); }

std::string textOf(const Value& v, std::string_view key, std::string fallback) {
    const Value* m = v.find(key);
    return m && m->string() ? *m->string() : fallback;
}

std::int64_t intOf(const Value& v, std::string_view key, std::int64_t fallback) {
    const Value* m = v.find(key);
    if (!m) return fallback;
    if (const std::int64_t* i = m->integer()) return *i;
    if (const double* d = m->real()) return static_cast<std::int64_t>(*d);
    return fallback;
}

bool boolOf(const Value& v, std::string_view key, bool fallback) {
    const Value* m = v.find(key);
    return m && m->boolean() ? *m->boolean() : fallback;
}

bool isObject(const Value* v) { return v && v->members(); }
bool isArray(const Value* v) { return v && v->elements(); }

Value::Object membersOf(const Value& v) {
    const Value::Object* o = v.members();
    return o ? *o : Value::Object{};
}

void put(Value::Object& o, std::string_view key, Value value) {
    for (Value::Member& m : o)
        if (m.key == key) {
            m.value = std::move(value);
            return;
        }
    o.push_back(Value::Member{std::string(key), std::move(value)});
}

std::string base64(std::string_view bytes) {
    std::string out;
    std::size_t i = 0;
    for (; i + 2 < bytes.size(); i += 3) {
        const std::uint32_t n = (static_cast<std::uint8_t>(bytes[i]) << 16) | (static_cast<std::uint8_t>(bytes[i + 1]) << 8) |
                                static_cast<std::uint8_t>(bytes[i + 2]);
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += kAlphabet[(n >> 6) & 63];
        out += kAlphabet[n & 63];
    }
    if (i < bytes.size()) {
        std::uint32_t n = static_cast<std::uint8_t>(bytes[i]) << 16;
        if (i + 1 < bytes.size()) n |= static_cast<std::uint8_t>(bytes[i + 1]) << 8;
        out += kAlphabet[(n >> 18) & 63];
        out += kAlphabet[(n >> 12) & 63];
        out += i + 1 < bytes.size() ? kAlphabet[(n >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

std::string unbase64(std::string_view text) {
    std::string out;
    std::uint32_t acc = 0;
    int bits = 0;
    for (const char c : text) {
        int v = -1;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
        else if (c >= '0' && c <= '9') v = c - '0' + 52;
        else if (c == '+') v = 62;
        else if (c == '/') v = 63;
        else continue;   // '=', white space
        acc = (acc << 6) | static_cast<std::uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += static_cast<char>((acc >> bits) & 0xFF);
        }
    }
    return out;
}

} // namespace shared::dap
