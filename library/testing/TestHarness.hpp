#pragma once
#include <cstdio>
#include <cstring>
#include <functional>
#include <type_traits>
#include <string>
#include <vector>

// The one C++ test runner of the repository: a TEST() registry
// with assert-and-tally checks, no dependency. Each test file registers its
// cases with TEST(); TestMain.cpp (target calcu_test_runner) runs them
// and returns non-zero on any failure. A product's test program links
// calcu_test_runner and needs no main() of its own.
//
// Every test is in a group of docs/test-groups.md: TEST(name) is Frequent,
// TEST_IN(longer, name) and TEST_IN(checkpoint, name) are in the group they
// name. A program runs one group, Frequent unless `--group longer`,
// `--group checkpoint` or `--group all` says otherwise; CTest has one entry
// per program and group (`<program>.longer`). A test of another group is
// reported as skipped, so a run by hand says where it is.
//
//   <program> [--group frequent|longer|checkpoint|all] [<text>]
//
// <text> runs only the tests whose name contains it.

namespace testharness {

enum class Group { frequent, longer, checkpoint };

struct Case { const char* name; std::function<void()> fn; Group group = Group::frequent; };

inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& passCount() { static int n = 0; return n; }
inline int& failCount() { static int n = 0; return n; }
inline int& skipCount() { static int n = 0; return n; }
inline const char*& currentName() { static const char* n = ""; return n; }

struct Registrar {
    Registrar(const char* name, std::function<void()> fn, Group group = Group::frequent) {
        registry().push_back({name, std::move(fn), group});
    }
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

// A test of the Longer or the Checkpoint group: TEST_IN(longer, name).
#define TEST_IN(group, name) \
    static void name(); \
    static testharness::Registrar TH_CAT(reg_, name)(#name, name, testharness::Group::group); \
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

// The group's argument ("longer") and its name ("Longer").
inline const char* groupArg(Group g) {
    switch (g) {
    case Group::frequent: return "frequent";
    case Group::longer: return "longer";
    case Group::checkpoint: return "checkpoint";
    }
    return "";
}
inline const char* groupName(Group g) {
    switch (g) {
    case Group::frequent: return "Frequent";
    case Group::longer: return "Longer";
    case Group::checkpoint: return "Checkpoint";
    }
    return "";
}

// The runners' loop (TestMain.cpp, the Qt runner): the arguments above,
// then the tests of the group in the order they were registered. The exit
// status: 0 when no check failed, 1 when one did, 2 for unknown arguments.
inline int runTests(int argc, char** argv) {
    const char* filter = nullptr;
    std::string group = "frequent";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--group") == 0 && i + 1 < argc) group = argv[++i];
        else if (!filter && argv[i][0] != '-') filter = argv[i];
        else {
            std::fprintf(stderr, "usage: %s [--group frequent|longer|checkpoint|all] [<text>]\n", argv[0]);
            return 2;
        }
    }
    const bool all = group == "all";
    if (!all && group != "frequent" && group != "longer" && group != "checkpoint") {
        std::fprintf(stderr, "unknown group %s: frequent, longer, checkpoint or all\n", group.c_str());
        return 2;
    }
    int ran = 0;
    for (auto& c : registry()) {
        if (filter && !std::strstr(c.name, filter)) continue;
        currentName() = c.name;
        if (!all && group != groupArg(c.group)) {
            if (c.group != Group::frequent) {   // the few of another group, so a run by hand says where they are
                skipCount()++;
                std::printf("SKIP %s: in the %s group (--group %s)\n", c.name, groupName(c.group), groupArg(c.group));
            }
            continue;
        }
        int before = failCount();
        c.fn();
        ++ran;
        if (failCount() != before) std::fprintf(stderr, "  in %s\n", c.name);
    }
    std::printf("%d tests (%d skipped), %d checks passed, %d failed\n", ran, skipCount(), passCount(), failCount());
    return failCount() == 0 ? 0 : 1;
}
} // namespace testharness
