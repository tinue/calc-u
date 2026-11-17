#include "TIFileKinds.hpp"
#include "Keycodes.hpp"
#include "TIDebugPart.hpp"
#include "TIPresetKeys.hpp"
#include "TextUtil.hpp"
#include "../core/TIProductKinds.hpp"
#include "sessions/LibraryTopics.hpp"
#include <memory>

namespace cu59 {

using calcu::opening::FileKind;
using calcu::opening::OpenedFile;
using calcu::opening::Way;
using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;

std::vector<std::string> tiPresetExtensions() { return {".ti59", ".ti58", ".ti58c"}; }

std::vector<std::string> programArguments(const std::vector<std::string>& argv) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < argv.size(); ++i) {
        const std::string& a = argv[i];
        if (i > 0 && (a == "--grab" || a == "--after" || a == "--pacing-probe")) {
            ++i;   // and its value
            continue;
        }
        out.push_back(a);
    }
    return out;
}

namespace {

bool endsWith(std::string_view lowerName, std::string_view suffix) {
    return lowerName.size() > suffix.size() && lowerName.substr(lowerName.size() - suffix.size()) == suffix;
}

std::optional<std::string> textOf(const OpenedFile& f) {
    if (!f.bytes) return std::nullopt;
    return std::string(f.bytes->begin(), f.bytes->end());
}

// C59-4 AC-19: an .asm file is an ASM overlay, read as by Select File.
class AsmOverlayKind final : public FileKind {
public:
    explicit AsmOverlayKind(TIDebugPart& d) : m_debug(d) {}
    std::string_view name() const override { return "asm-overlay"; }
    bool takesName(std::string_view n) const override { return endsWith(n, ".asm"); }
    bool takes(const OpenedFile& f) const override { return takesName(toLower(f.name)); }
    CommandOutcome open(const OpenedFile& f, Way, SessionContext& c) override {
        m_debug.attachContext(c);
        if (auto refusal = m_debug.loadAsm(f.name, textOf(f), false)) return Refused{*refusal};
        return Done{};
    }

private:
    TIDebugPart& m_debug;
};

// LIB-13 AC-23: written into the running machine from step 000, as a
// preset's `program:` writes it, without a cold boot; the partition, the
// data registers and the steps it does not list stay (D-23: also paused).
class KeystrokesKind final : public FileKind {
public:
    std::string_view name() const override { return "keystroke-program"; }
    bool takesName(std::string_view n) const override { return endsWith(n, ".keystrokes.yaml"); }
    bool takes(const OpenedFile& f) const override { return takesName(toLower(f.name)); }
    CommandOutcome open(const OpenedFile& f, Way, SessionContext& c) override {
        calcu::machine::Machine* m = c.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) return Refused{calcu::sessions::kNoMachine};
        const auto text = textOf(f);
        if (!text) return Refused{f.name + " could not be read."};
        auto r = readKeystrokeProgramFile(*text, pm->lastStep());
        if (auto* why = std::get_if<std::string>(&r)) return Refused{f.name + ": " + *why};
        for (const ProgramStep& s : std::get<KeystrokeProgram>(r).steps) pm->writeStep(s.step, s.keycode);
        return Done{};
    }
};

// LIB-13 AC-23, D-17: refused with a sentence of their own.
class RefusedKind final : public FileKind {
public:
    RefusedKind(std::string_view name, std::function<bool(std::string_view)> rule, std::string why)
        : m_name(name), m_rule(std::move(rule)), m_why(std::move(why)) {}
    std::string_view name() const override { return m_name; }
    bool takesName(std::string_view n) const override { return m_rule(n); }
    bool takes(const OpenedFile& f) const override { return takesName(toLower(f.name)); }
    CommandOutcome open(const OpenedFile& f, Way, SessionContext&) override {
        return Refused{f.name + " was not opened: " + m_why};
    }

private:
    std::string_view m_name;
    std::function<bool(std::string_view)> m_rule;
    std::string m_why;
};

} // namespace

struct TIFileKinds::Kinds {
    AsmOverlayKind asmKind;
    KeystrokesKind keystrokes;
    RefusedKind romModule{"rom-module-file", [](std::string_view n) { return endsWith(n, ".rom.yaml"); },
                          "this release takes a Solid State ROM module by its ID only."};
    RefusedKind constantMemory{"constant-memory-file", [](std::string_view n) {
                                   const std::string_view base = n.substr(n.find_last_of("/\\") == std::string_view::npos ? 0 : n.find_last_of("/\\") + 1);
                                   return base == "ti58c.memory.yaml";
                               },
                               "it is Calc-U-59's own constant memory file."};
    explicit Kinds(TIDebugPart& d) : asmKind(d) {}
};

TIFileKinds::TIFileKinds(TIDebugPart& debug) : m_kinds(std::make_unique<Kinds>(debug)) {}

TIFileKinds::~TIFileKinds() = default;

std::vector<FileKind*> TIFileKinds::kinds() {
    return {&m_kinds->asmKind, &m_kinds->keystrokes, &m_kinds->romModule, &m_kinds->constantMemory};
}

} // namespace cu59
