#include "app/SharpFileKinds.hpp"

#include <memory>
#include <variant>

#include "Loaders.hpp"
#include "ProgramFile.hpp"
#include "opening/Classify.hpp"
#include "sessions/LibraryTopics.hpp"

namespace cu1600 {

using calcu::opening::OpenedFile;
using calcu::opening::Way;
using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;
using calcu::sessions::Value;

std::vector<std::string> presetExtensions() { return {".pc1500", ".pc1500a", ".pc1600"}; }

std::map<Way, std::uint64_t> openLimits() {
    return {{Way::Drop, kMaxOpenBytes}, {Way::OpenWith, kMaxOpenBytes}, {Way::Start, kMaxOpenBytes}};
}

ProgramKind programKindOf(const std::vector<std::uint8_t>& bytes, std::string_view name) {
    using programfile::Kind;
    const programfile::ProgramFile f = programfile::classify(bytes);
    switch (f.kind) {
        case Kind::BasicListing:
        case Kind::BasicPC1500:
        case Kind::BasicPC1600: return ProgramKind::Basic;
        case Kind::CodeLH5801:
        case Kind::CodeZ80: return ProgramKind::MachineCode;
        // The classifier's guess alone also takes JPEGs, PDFs and fonts for
        // code, hence the name; `.rom` is none.
        case Kind::Headerless:
            return f.looksLikeCode && calcu::opening::endsWith(calcu::opening::lowerAscii(name), ".bin")
                       ? ProgramKind::MachineCode
                       : ProgramKind::None;
        case Kind::Empty: return ProgramKind::None;
        // A cassette WAV without a file that can be read goes to the BASIC
        // loader, which says why; other files go nowhere.
        case Kind::Other: return f.fromTape ? ProgramKind::Basic : ProgramKind::None;
    }
    return ProgramKind::None;
}

std::string_view programKindName(ProgramKind k) {
    switch (k) {
        case ProgramKind::Basic: return "basic-program";
        case ProgramKind::MachineCode: return "machine-code-program";
        case ProgramKind::None: break;
    }
    return "";
}

Value ProgramOpenedPayload::toValue() const {
    return Value::object({{"name", name}, {"kind", kind}, {"bytes", Value(Value::Bytes(bytes))}, {"path", path}});
}

bool ProgramFileKind::takes(const OpenedFile& f) const {
    return f.bytes && programKindOf(*f.bytes, calcu::opening::fileNameOf(f)) == m_which;
}

CommandOutcome ProgramFileKind::open(const OpenedFile& f, Way way, SessionContext& ctx) {
    const std::string name = calcu::opening::fileNameOf(f);
    const std::vector<std::uint8_t>& bytes = *f.bytes;   // takes() saw them
    if (way != Way::Harness && m_topic) {
        // The frontend's program flow (its dialogs: which file of a
        // WAV, the address of headerless code).
        auto p = std::make_shared<ProgramOpenedPayload>();
        p->name = name;
        p->kind = std::string(programKindName(m_which));
        p->bytes = bytes;
        p->path = f.path.value_or(std::string());
        m_topic->append(std::move(p));
        return Done{};
    }
    // The harness has no dialogs: a WAV's first file, by its own
    // loader; headerless code is refused with the plan's sentence.
    if (ctx.busy()) return Refused{calcu::sessions::kAnotherOperationRuns};
    if (!ctx.machine()) return Refused{calcu::sessions::kNoMachine};
    std::string why;
    const auto image = sharp::loaders::image(bytes, 0, &why);
    if (!image) return Refused{sharp::loaders::refusal(name, why)};
    if (m_which == ProgramKind::Basic) {
        const sharp::loaders::BasicLoaded r = sharp::loaders::loadBasic(*ctx.machine(), *image);
        if (!r.ok) return Refused{sharp::loaders::refusal(name, r.refusal)};
        return Done{};
    }
    auto loaded = sharp::loaders::loadCode(*ctx.machine(), *image, {});
    if (auto* refusal = std::get_if<std::string>(&loaded)) return Refused{sharp::loaders::refusal(name, *refusal)};
    return Done{};
}

} // namespace cu1600
