#pragma once
#include <cstdio>
#include <functional>
#include <type_traits>
#include <string>
#include <vector>

// The one C++ test runner of the repository: a TEST() registry
// with assert-and-tally checks, no dependency. Each test file registers its
// cases with TEST(); TestMain.cpp (target calcu_test_runner) runs them all
// and returns non-zero on any failure. A product's test program links
// calcu_test_runner and needs no main() of its own.

namespace testharness {

struct Case { const char* name; std::function<void()> fn; };

inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& passCount() { static int n = 0; return n; }
inline int& failCount() { static int n = 0; return n; }
inline int& skipCount() { static int n = 0; return n; }
inline const char*& currentName() { static const char* n = ""; return n; }

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

// Marks the running test as skipped (a missing ROM image, say); the test
// returns after it. A skip is neither a pass nor a failure.
inline void skip(const char* why) {
    skipCount()++;
    std::printf("SKIP %s: %s\n", currentName(), why);
}

} // namespace testharness

#define TH_CAT2(a, b) a##b
#define TH_CAT(a, b) TH_CAT2(a, b)

#define TEST(name) \
    static void name(); \
    static testharness::Registrar TH_CAT(reg_, name)(#name, name); \
    static void name()

#define CHECK(cond) do { \
    if (cond) { testharness::passCount()++; } \
    else { testharness::failCount()++; std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ(a, b) do { \
    auto&& th_a = (a); auto&& th_b = (b); \
    if (th_a == th_b) { testharness::passCount()++; } \
    else { testharness::failCount()++; \
           std::fprintf(stderr, "FAIL %s:%d: %s == %s (%s vs %s)\n", __FILE__, __LINE__, #a, #b, \
                        testharness::show(th_a).c_str(), testharness::show(th_b).c_str()); } \
} while (0)

// Inside a TEST: skips the rest of it unless `cond` holds.
#define SKIP_UNLESS(cond, why) do { \
    if (!(cond)) { testharness::skip(why); return; } \
} while (0)

namespace testharness {
inline std::string show(const std::string& s) { return "\"" + s + "\""; }
inline std::string show(const char* s) { return show(std::string(s)); }
inline std::string show(bool b) { return b ? "true" : "false"; }
template <class T> std::string show(const T& v) {
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) return std::to_string(static_cast<long long>(v));
    else return "<value>";
}
} // namespace testharness
