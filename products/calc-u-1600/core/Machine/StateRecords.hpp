#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The product-internal form of the machine state: tagged, length-prefixed records, little-endian, one record
// per part of the core, in the machine's order. The form is not
// documented outside this file; the library writes the
// header in front of it and refuses a file of another configuration.
//
// A record is a four-character tag, a 32-bit length and the fields. Each
// part of the core that holds state names its fields once, in a member
// template `stateIo(Io&)`, for both directions: StateWriter takes them,
// StateReader puts them back. `Io::kRestoring` tells the template which
// way it runs, for the few parts that rebuild something after a restore.
//
// Fields: integers, enums, bool and double (by their bits); std::array
// and std::vector of them (a vector as a count and its items), std::deque,
// std::string. `fixed(v)` reads a vector only into the size it has now: a
// part whose memory size comes from its definition (a module's RAM).
//
// A reader that meets another tag, a record of another length or the end
// of the bytes stops: every later field reads as zero and ok() is false
// with the reason. The caller then puts the state from before back
// (SharpMachineState): a refused restore changes nothing.

namespace sharp {

class StateWriter {
public:
    static constexpr bool kRestoring = false;

    void begin(const char (&tag)[5]) {
        m_out.insert(m_out.end(), tag, tag + 4);
        m_open.push_back(m_out.size());
        put(std::uint32_t{0});
    }
    void end() {
        const std::size_t at = m_open.back();
        m_open.pop_back();
        const std::uint32_t n = static_cast<std::uint32_t>(m_out.size() - at - 4);
        for (int i = 0; i < 4; ++i) m_out[at + std::size_t(i)] = static_cast<std::uint8_t>(n >> (8 * i));
    }

    template <class T>
    StateWriter& operator()(const T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            put(static_cast<std::uint8_t>(v ? 1 : 0));
        } else if constexpr (std::is_enum_v<T>) {
            put(static_cast<std::int64_t>(v));
        } else if constexpr (std::is_same_v<T, double>) {
            std::uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof bits);
            put(bits);
        } else if constexpr (std::is_integral_v<T>) {
            put(v);
        } else if constexpr (std::is_array_v<T>) {
            for (const auto& x : v) (*this)(x);
        } else {
            container(v);
        }
        return *this;
    }
    template <class T>
    StateWriter& fixed(const std::vector<T>& v) { return (*this)(v); }

    std::vector<std::uint8_t> take() { return std::move(m_out); }

private:
    template <class T>
    void put(T v) {
        using U = std::make_unsigned_t<T>;
        const U u = static_cast<U>(v);
        for (std::size_t i = 0; i < sizeof(T); ++i) m_out.push_back(static_cast<std::uint8_t>(u >> (8 * i)));
    }
    template <class T, std::size_t N>
    void container(const std::array<T, N>& a) {
        for (const T& x : a) (*this)(x);
    }
    template <class C>
    void container(const C& c) {   // vector, deque, string
        put(static_cast<std::uint64_t>(c.size()));
        if constexpr (sizeof(typename C::value_type) == 1 && std::is_integral_v<typename C::value_type>) {
            for (auto x : c) m_out.push_back(static_cast<std::uint8_t>(x));
        } else {
            for (const auto& x : c) (*this)(x);
        }
    }

    std::vector<std::uint8_t> m_out;
    std::vector<std::size_t> m_open;
};

class StateReader {
public:
    static constexpr bool kRestoring = true;

    explicit StateReader(const std::vector<std::uint8_t>& in) : m_in(in) {}

    void begin(const char (&tag)[5]) {
        if (!m_ok) return;
        if (m_pos + 8 > m_in.size() || std::memcmp(m_in.data() + m_pos, tag, 4) != 0) {
            fail(std::string("the state has no record ") + tag + " where this machine has one");
            return;
        }
        m_pos += 4;
        std::uint32_t n = 0;
        get(n);
        m_open.push_back({m_pos + n, tag});
        if (m_open.back().end > m_in.size()) fail(std::string("the record ") + tag + " is cut short");
    }
    void end() {
        if (m_open.empty()) return;
        const Open o = m_open.back();
        m_open.pop_back();
        if (m_ok && m_pos != o.end) fail("the record " + o.tag + " does not fit this machine's part");
    }

    template <class T>
    StateReader& operator()(T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            std::uint8_t b = 0;
            get(b);
            v = b != 0;
        } else if constexpr (std::is_enum_v<T>) {
            std::int64_t e = 0;
            get(e);
            v = static_cast<T>(e);
        } else if constexpr (std::is_same_v<T, double>) {
            std::uint64_t bits = 0;
            get(bits);
            std::memcpy(&v, &bits, sizeof bits);
        } else if constexpr (std::is_integral_v<T>) {
            get(v);
        } else if constexpr (std::is_array_v<T>) {
            for (auto& x : v) (*this)(x);
        } else {
            container(v);
        }
        return *this;
    }
    // A vector whose size this machine gives: a state of another size is refused.
    template <class T>
    StateReader& fixed(std::vector<T>& v) {
        const std::size_t want = v.size();
        if (peekCount() != want) {
            fail("a memory of the state has another size than this machine's");
            return *this;
        }
        return (*this)(v);
    }

    bool ok() const { return m_ok; }
    bool atEnd() const { return m_pos == m_in.size(); }
    const std::string& error() const { return m_error; }
    void fail(std::string why) {
        if (!m_ok) return;
        m_ok = false;
        m_error = std::move(why);
    }

private:
    struct Open {
        std::size_t end;
        std::string tag;
    };
    std::size_t limit() const { return m_open.empty() ? m_in.size() : m_open.back().end; }
    template <class T>
    void get(T& v) {
        using U = std::make_unsigned_t<T>;
        U u = 0;
        if (!m_ok || m_pos + sizeof(T) > limit()) {
            fail("the state ends inside a record");
            v = T{};
            return;
        }
        for (std::size_t i = 0; i < sizeof(T); ++i) u = static_cast<U>(u | (U(m_in[m_pos + i]) << (8 * i)));
        m_pos += sizeof(T);
        v = static_cast<T>(u);
    }
    std::uint64_t peekCount() {
        if (!m_ok || m_pos + 8 > limit()) return ~std::uint64_t{0};
        std::uint64_t n = 0;
        for (std::size_t i = 0; i < 8; ++i) n |= std::uint64_t(m_in[m_pos + i]) << (8 * i);
        return n;
    }
    template <class T, std::size_t N>
    void container(std::array<T, N>& a) {
        for (T& x : a) (*this)(x);
    }
    template <class C>
    void container(C& c) {   // vector, deque, string
        std::uint64_t n = 0;
        get(n);
        if (!m_ok) return;
        using V = typename C::value_type;
        if (n > limit() - m_pos) {   // every item takes a byte at least
            fail("the state ends inside a record");
            return;
        }
        c.clear();
        c.resize(static_cast<std::size_t>(n));
        if constexpr (sizeof(V) == 1 && std::is_integral_v<V>) {
            for (auto& x : c) x = static_cast<V>(m_in[m_pos++]);
        } else {
            for (auto& x : c) (*this)(x);
        }
    }

    const std::vector<std::uint8_t>& m_in;
    std::size_t m_pos = 0;
    std::vector<Open> m_open;
    bool m_ok = true;
    std::string m_error;
};

} // namespace sharp
