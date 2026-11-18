#include "TIMnemonics.hpp"
#include "KeyNames.hpp"
#include "TITyping.hpp"
#include "machine/Names.hpp"
#include <cstdio>
#include <map>
#include <optional>
#include <set>
#include <sstream>

namespace cu59 {

namespace {

// The key IDs of preset.md §8.1 by keycode: a key at position
// row × 10 + column, its second function at row × 10 + column + 5 (column 5:
// the next ten's 0), the digits 00-09, the merged codes as key and Ind.
// Empty: no key name (26, 27, 82), written otherwise (assemblyMnemonic).
const char* const kIds[100] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "E'", "A", "B", "C", "D", "E", "A'", "B'", "C'", "D'",
    "2nd CLR", "2nd", "INV", "lnx", "CE", "CLR", "", "", "log", "CP",
    "tan", "LRN", "x\xE2\x87\x84t", "x\xC2\xB2", "\xE2\x88\x9Ax", "1/x", "Pgm", "P/R", "sin", "cos",
    "Ind", "SST", "STO", "RCL", "SUM", "y\xCB\xA3", "Ins", "CMs", "Exc", "Prd",
    "|x|", "BST", "EE", "(", ")", "\xC3\xB7", "Del", "Eng", "Fix", "Int",
    "Deg", "GTO", "Pgm Ind", "Exc Ind", "Prd Ind", "\xC3\x97", "Pause", "x=t", "Nop", "Op",
    "Rad", "SBR", "STO Ind", "RCL Ind", "SUM Ind", "-", "Lbl", "x\xE2\x89\xA5t", "\xCE\xA3+", "x\xCC\x84",
    "Grad", "RST", "", "GTO Ind", "Op Ind", "+", "STF", "IFF", "D.MS", "\xCF\x80",
    "List", "R/S", "RTN", ".", "+/-", "=", "Write", "Dsz", "Adv", "Prt",
};

bool merged(std::uint8_t k) { return k == 62 || k == 63 || k == 64 || k == 72 || k == 73 || k == 74 || k == 83 || k == 84; }
// One register, flag, program or digit number (Ind allowed: not merged).
bool oneNumber(std::uint8_t k) { return k == 36 || k == 42 || k == 43 || k == 44 || k == 48 || k == 49 || k == 58 || k == 69 || k == 82 || k == 86; }
bool branch(std::uint8_t k) { return k == 61 || k == 67 || k == 71 || k == 77; }
bool numberThenBranch(std::uint8_t k) { return k == 87 || k == 97; }
bool inv(std::uint8_t k) { return k == 22 || k == 27; }

std::string digits(int v, int width) {
    char b[8];
    std::snprintf(b, sizeof b, "%0*d", width, v);
    return b;
}

// What the presses of `key` for a name are, as text (the lookup key).
std::string pressesText(const std::vector<calcu::typing::Press>& p) {
    std::string s;
    for (const auto& x : p) s += x.key.value + "\x1f";
    return s;
}

// The keycode of each `key` press sequence of the table: a name is
// read by what `key` presses for it.
const std::map<std::string, std::uint8_t>& keycodeOfPresses() {
    static const std::map<std::string, std::uint8_t> t = [] {
        std::map<std::string, std::uint8_t> m;
        for (int k = 0; k < 100; ++k) {
            if (!*kIds[k]) continue;
            std::vector<std::string> names;
            std::istringstream in(kIds[k]);
            for (std::string w; in >> w;) names.push_back(w);
            if (auto p = tiKeyNames(names)) m.emplace(pressesText(*p), static_cast<std::uint8_t>(k));
        }
        return m;
    }();
    return t;
}

// The name the PC-100C's List prints for a keycode: app/KeyNames' names, but
// for × (65), which the printer prints `x` and release 2.3.0's debug panel
// table names `|x|`, keycode 50's name (tests/golden/keynames.txt keeps
// that table).
std::string printedName(std::uint8_t k) { return k == 65 ? std::string("x") : mnemonic(k); }

} // namespace

std::string assemblyMnemonic(std::uint8_t k) {
    if (k < 100 && *kIds[k]) return kIds[k];
    if (k == 82) return "HIR";
    return digits(k, 2);
}

int lastUsedStep(const std::vector<std::uint8_t>& steps, int first) {
    for (int i = static_cast<int>(steps.size()) - 1; i >= first; --i)
        if (steps[static_cast<std::size_t>(i)] != 0) return i;
    return first - 1;
}

std::vector<ListedLine> listing(const std::vector<std::uint8_t>& steps, int first, int last) {
    const std::set<int> args = argumentSteps(steps);
    std::vector<ListedLine> out;
    for (int i = first; i <= last && i < static_cast<int>(steps.size()); ++i) {
        const std::uint8_t k = steps[static_cast<std::size_t>(i)];
        out.push_back({i, digits(i, 3) + " " + digits(k, 2) + " " + (args.count(i) ? digits(k, 2) : printedName(k))});
    }
    return out;
}

std::vector<ListedLine> disassemble(const std::vector<std::uint8_t>& p, int first, int last) {
    std::vector<ListedLine> out;
    const int n = static_cast<int>(p.size());
    auto at = [&](int j) -> std::optional<std::uint8_t> {
        if (j < n) return p[static_cast<std::size_t>(j)];
        return std::nullopt;
    };
    auto number = [&](int& j, std::string& s) {   // NN or Ind NN
        const auto a = at(j);
        if (!a) { s += " ?"; ++j; return; }
        if (*a == 40) {
            const auto b = at(j + 1);
            s += " Ind " + (b ? digits(*b, 2) : std::string("?"));
            j += 2;
            return;
        }
        s += " " + digits(*a, 2);
        ++j;
    };
    auto destination = [&](int& j, std::string& s) {   // nnn, Ind NN or a label
        const auto a = at(j);
        if (!a) { s += " ?"; ++j; return; }
        if (*a <= 9) {
            const auto b = at(j + 1);
            s += " " + (b ? digits(*a * 100 + *b, 3) : std::string("?"));
            j += 2;
        } else if (*a == 40) {
            number(j, s);
        } else {
            s += " " + assemblyMnemonic(*a);
            ++j;
        }
    };
    // The grammar runs from step 0, so that `first` falls on an instruction
    // as the calculator reads the program.
    int i = 0;
    bool prevInv = false;
    while (i < n && i <= last) {
        const std::uint8_t k = p[static_cast<std::size_t>(i)];
        std::string s = assemblyMnemonic(k);
        int j = i + 1;
        if (merged(k)) {
            const auto a = at(j);
            s += " " + (a ? digits(*a, 2) : std::string("?"));
            ++j;
        } else if (oneNumber(k)) {
            number(j, s);
        } else if (k == 76) {   // Lbl: the label
            const auto a = at(j);
            s += " " + (a ? assemblyMnemonic(*a) : std::string("?"));
            ++j;
        } else if (branch(k) && !(k == 71 && prevInv)) {   // INV SBR: no operand
            destination(j, s);
        } else if (numberThenBranch(k)) {
            number(j, s);
            destination(j, s);
        }
        if (i >= first) out.push_back({i, digits(i, 3) + "  " + s});
        prevInv = inv(k);
        i = j;
    }
    return out;
}

namespace {

struct Reader {
    std::vector<std::string> t;
    std::size_t i = 0;
    bool more() const { return i < t.size(); }
};

std::string lower(const std::string& s) { return calcu::machine::lowerAscii(s); }

bool allDigits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

// A mnemonic at the reader (one or two words: `2nd CLR`, `STO Ind`), its
// keycode; nullopt: not one.
std::optional<std::uint8_t> readMnemonic(Reader& r, bool allowMerged) {
    if (!r.more()) return std::nullopt;
    const std::string w = r.t[r.i];
    const auto& table = keycodeOfPresses();
    // Two words first: a second function written `2nd <key>`, a merged code.
    if (r.i + 1 < r.t.size()) {
        const std::string w2 = r.t[r.i + 1];
        const bool second = lower(w) == "2nd";
        const bool ind = allowMerged && lower(w2) == "ind";
        if (second || ind) {
            if (auto p = tiKeyNames({w, w2})) {
                const auto f = table.find(pressesText(*p));
                if (f != table.end() && (second || merged(f->second))) {
                    r.i += 2;
                    return f->second;
                }
            }
        }
    }
    if (lower(w) == "hir") { ++r.i; return std::uint8_t{82}; }
    if (w.size() == 2 && allDigits(w)) { ++r.i; return static_cast<std::uint8_t>(std::stoi(w)); }   // 26, 27
    if (auto p = tiKeyName(w)) {
        const auto f = table.find(pressesText(*p));
        if (f != table.end()) {
            ++r.i;
            return f->second;
        }
    }
    return std::nullopt;
}

} // namespace

std::variant<std::vector<ProgramStep>, KeycodeError> assemble(std::string_view text, int lastStep) {
    std::vector<ProgramStep> out;
    std::set<int> used;
    std::istringstream in{std::string(text)};
    int lineIndex = -1;
    for (std::string line; std::getline(in, line);) {
        ++lineIndex;
        if (const auto h = line.find('#'); h != std::string::npos) line.erase(h);
        Reader r;
        std::istringstream ws(line);
        for (std::string w; ws >> w;) r.t.push_back(w);
        if (r.t.empty()) continue;
        auto fail = [&](const std::string& why) { return KeycodeError{lineIndex, why}; };
        if (r.t[0].size() < 3 || !allDigits(r.t[0])) return fail("A line starts with its step, three digits or more: " + r.t[0] + ".");
        const int step = std::stoi(r.t[0].size() > 6 ? std::string("9999999") : r.t[0]);
        r.i = 1;
        const auto k = readMnemonic(r, true);
        if (!k) return fail(r.more() ? "Unknown mnemonic: " + r.t[r.i] + "." : "The step has no mnemonic.");
        std::vector<std::uint8_t> codes{*k};
        auto number = [&](bool allowInd) -> std::optional<std::string> {
            if (!r.more()) return std::string("missing");
            std::string w = r.t[r.i];
            if (allowInd && lower(w) == "ind") {
                codes.push_back(40);
                ++r.i;
                if (!r.more()) return std::string("missing");
                w = r.t[r.i];
            }
            if (w.size() > 2 || !allDigits(w)) return w;
            codes.push_back(static_cast<std::uint8_t>(std::stoi(w)));
            ++r.i;
            return std::nullopt;
        };
        auto destination = [&]() -> std::optional<std::string> {
            if (!r.more()) return std::string("missing");
            const std::string w = r.t[r.i];
            if (lower(w) == "ind") return number(true);
            if (w.size() == 3 && allDigits(w)) {
                const int a = std::stoi(w);
                codes.push_back(static_cast<std::uint8_t>(a / 100));
                codes.push_back(static_cast<std::uint8_t>(a % 100));
                ++r.i;
                return std::nullopt;
            }
            const auto l = readMnemonic(r, true);
            if (!l || *l <= 9 || *l == 40) return w;
            codes.push_back(*l);
            return std::nullopt;
        };
        std::optional<std::string> bad;
        const bool afterInv = !out.empty() && used.count(step - 1) && inv(out.back().keycode) && out.back().step == step - 1;
        if (merged(*k)) bad = number(false);
        else if (oneNumber(*k)) bad = number(true);
        else if (*k == 76) {
            const auto l = readMnemonic(r, true);
            if (!l) bad = r.more() ? r.t[r.i] : std::string("missing");
            else codes.push_back(*l);
        } else if (branch(*k) && !(*k == 71 && afterInv)) bad = destination();
        else if (numberThenBranch(*k)) {
            bad = number(true);
            if (!bad) bad = destination();
        }
        if (bad) {
            const std::string m = assemblyMnemonic(*k);
            return fail(*bad == "missing" ? m + " needs its operand." : m + ": not an operand: " + *bad + ".");
        }
        if (r.more()) return fail("More than the instruction takes: " + r.t[r.i] + ".");
        if (step + static_cast<int>(codes.size()) - 1 > lastStep)
            return fail("Beyond the last step, " + digits(lastStep, 3) + ".");
        for (std::size_t c = 0; c < codes.size(); ++c) {
            const int s = step + static_cast<int>(c);
            if (!used.insert(s).second) return fail("Step " + digits(s, 3) + " is written twice.");
            out.push_back({s, codes[c]});
        }
    }
    return out;
}

} // namespace cu59
