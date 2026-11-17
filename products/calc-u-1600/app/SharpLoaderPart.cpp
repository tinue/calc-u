#include "app/SharpLoaderPart.hpp"

#include <cctype>
#include <variant>

#include "BasicTyping.hpp"
#include "Loaders.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "app/SharpTopics.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/Awaitable.hpp"
#include "typing/TypingSupport.hpp"

namespace shared {

using calcu::sessions::CommandOutcome;
using calcu::sessions::Done;
using calcu::sessions::OperationContext;
using calcu::sessions::OperationResult;
using calcu::sessions::Refused;
using calcu::sessions::SessionContext;
using calcu::sessions::Value;
using machinecode::hexAddress;

namespace {

Value strings(const std::vector<std::string>& v) {
    Value::Array a;
    for (const std::string& s : v) a.push_back(Value(s));
    return Value(std::move(a));
}

const char* kindName(programfile::Kind k) {
    using programfile::Kind;
    switch (k) {
        case Kind::Empty: return "empty";
        case Kind::BasicListing: return "basic-listing";
        case Kind::BasicPC1500: return "basic-pc1500";
        case Kind::BasicPC1600: return "basic-pc1600";
        case Kind::CodeLH5801: return "code-lh5801";
        case Kind::CodeZ80: return "code-z80";
        case Kind::Headerless: return "headerless";
        case Kind::Other: return "other";
    }
    return "other";
}

std::vector<std::uint8_t> bytesOf(const Value& a) {
    return calcu::sessions::bytesArg(a, "bytes").value_or(std::vector<std::uint8_t>{});
}

// The image of the file the call names (`file`, from 1), or the reason.
std::variant<std::vector<std::uint8_t>, std::string> imageOf(const Value& a) {
    std::string why;
    auto img = sharp::loaders::image(bytesOf(a), static_cast<int>(calcu::sessions::truncatedIntegerArg(a, "file", 0)), &why);
    if (!img) return why;
    return std::move(*img);
}

// What the caller gives by hand (design §3.7); the reason when the address
// is no hex address (today's hint).
std::variant<sharp::loaders::CodeRequest, std::string> requestOf(const Value& a) {
    sharp::loaders::CodeRequest r;
    const std::string text = calcu::sessions::textArg(a, "address");
    if (text.find_first_not_of(" \t") != std::string::npos) {
        std::uint32_t addr = 0;
        if (!machinecode::parseHexAddress(text, &addr)) return std::string("Not a hex address (&0000-&FFFF).");
        r.address = addr;
    }
    if (a.find("length")) {
        const std::int64_t n = calcu::sessions::truncatedIntegerArg(a, "length", -1);
        if (n < 0) return std::string("The length is no number.");
        r.length = static_cast<std::size_t>(n);
    }
    const std::string cpu = calcu::sessions::textArg(a, "cpu");
    if (!cpu.empty()) {
        std::string lower;
        for (char c : cpu) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "lh5803") r.cpu = machinecode::Cpu::LH5803;
        else if (lower == "z80") r.cpu = machinecode::Cpu::Z80;
        else return "There is no processor \"" + cpu + "\" (lh5803, z80).";
    }
    return r;
}

std::shared_ptr<CodePlanReply> planReply(const sharp::loaders::CodePlan& p) {
    auto r = std::make_shared<CodePlanReply>();
    r->refusal = p.refusal;
    r->needsAddress = p.needsAddress();
    r->length = p.file.payload.size();
    const bool pc1600 = p.target == machinecode::Target::PC1600;
    if (r->needsAddress && p.plan.defaultAddr != 0) r->proposed = hexAddress(p.plan.defaultAddr);
    r->placeholder = !pc1600 ? "hex, e.g. &7C01"
                     : p.plan.cpu == machinecode::Cpu::LH5803 ? "hex, e.g. &40C5"
                                                              : "hex, e.g. &C0C5";
    if (pc1600) r->cpu = p.plan.cpu == machinecode::Cpu::LH5803 ? "LH5803" : "Z-80";
    r->cpuNote = sharp::loaders::cpuNote(p);
    r->where = sharp::loaders::where(p);
    return r;
}

std::shared_ptr<CodeLoadedReply> loadedReply(const sharp::loaders::CodeLoaded& c) {
    auto r = std::make_shared<CodeLoadedReply>();
    r->summary = c.summary;
    r->range = c.range;
    r->area = c.area;
    r->bank = c.bank;
    r->newCommand = c.advice.newCommand;
    r->newNote = c.advice.newNote;
    r->callCommand = c.advice.callCommand;
    r->callNote = c.advice.callNote;
    r->warnings = c.warnings;
    return r;
}

} // namespace

Value FileKindReply::toValue() const {
    return Value::object({{"kind", kind},
                          {"words", words},
                          {"looksLike", looksLike},
                          {"damaged", damaged},
                          {"hasHeader", hasHeader},
                          {"headerName", headerName},
                          {"loadAddress", static_cast<std::int64_t>(loadAddress)},
                          {"bank", bank},
                          {"length", static_cast<std::int64_t>(length)},
                          {"start", static_cast<std::int64_t>(start)},
                          {"tapeFiles", strings(tapeFiles)},
                          {"tapeKinds", strings(tapeKinds)}});
}

Value CodePlanReply::toValue() const {
    return Value::object({{"refusal", refusal},
                          {"needsAddress", needsAddress},
                          {"proposed", proposed},
                          {"placeholder", placeholder},
                          {"length", static_cast<std::int64_t>(length)},
                          {"cpu", cpu},
                          {"cpuNote", cpuNote},
                          {"where", where}});
}

Value CodeLoadedReply::toValue() const {
    return Value::object({{"summary", summary},
                          {"range", range},
                          {"area", area},
                          {"bank", bank},
                          {"newCommand", newCommand},
                          {"newNote", newNote},
                          {"callCommand", callCommand},
                          {"callNote", callNote},
                          {"warnings", strings(warnings)},
                          {"started", started}});
}

void SharpLoaderPart::attach(calcu::sessions::SessionSetup& setup) {
    namespace c = commands;
    namespace q = requests;
    const char* busy = calcu::sessions::kAnotherOperationRuns;

    setup.request(q::kFileKind, [busy](const Value& a, SessionContext& ctx) {
        if (ctx.busy()) return calcu::sessions::replyRefused(busy);
        const sharp::loaders::FileKind k = sharp::loaders::fileKind(bytesOf(a));
        auto r = std::make_shared<FileKindReply>();
        r->kind = kindName(k.kind);
        r->words = k.words;
        if (k.looksLike) r->looksLike = *k.looksLike == machinecode::Cpu::Z80 ? "z80" : "lh5803";
        r->damaged = k.damaged;
        if (k.header) {
            r->hasHeader = true;
            r->headerName = k.header->name;
            r->loadAddress = k.header->loadAddress;
            r->bank = k.header->bank;
            r->length = k.header->length;
            r->start = k.header->start.value_or(0);
        }
        r->tapeFiles = k.tapeFiles;
        for (programfile::Kind t : k.tapeKinds) r->tapeKinds.push_back(kindName(t));
        return calcu::sessions::replyGiven(std::move(r));
    });

    setup.command(c::kLoadBasic, [busy](const Value& a, SessionContext& ctx) -> CommandOutcome {
        if (ctx.busy()) return Refused{busy};
        const std::string name = calcu::sessions::textArg(a, "name");
        auto img = imageOf(a);
        if (auto* why = std::get_if<std::string>(&img)) return Refused{sharp::loaders::refusal(name, *why)};
        if (!ctx.machine()) return Refused{calcu::sessions::kNoMachine};
        const sharp::loaders::BasicLoaded r = sharp::loaders::loadBasic(*ctx.machine(), std::get<0>(img));
        if (!r.ok) return Refused{sharp::loaders::refusal(name, r.refusal)};
        return Done{};
    });

    setup.command(c::kTypeBasic, [this, busy](const Value& a, SessionContext& ctx) -> CommandOutcome {
        if (ctx.busy()) return Refused{busy};
        const std::string name = calcu::sessions::textArg(a, "name");
        const std::vector<std::uint8_t> bytes = bytesOf(a);
        const sharp::loaders::FileKind k = sharp::loaders::fileKind(bytes);
        // A listing, or text the classifier calls plain (a line without a
        // number is still typed, and then not stored: AC-7).
        const bool text = k.kind == programfile::Kind::BasicListing ||
                          (k.kind == programfile::Kind::Other && k.words == "plain text");
        if (!text) {
            const std::string why = k.kind == programfile::Kind::Empty ? "The file is empty."
                                                                     : "Only a BASIC listing (text) can be typed in.";
            return Refused{sharp::loaders::refusal(name, why)};
        }
        std::string listing(bytes.begin(), bytes.end());
        return calcu::sessions::StartOperation{[this, listing, name](OperationContext& op) {
            return typeOperation(this, op, listing, name);
        }};
    });

    setup.request(q::kPlanCode, [busy](const Value& a, SessionContext& ctx) {
        if (ctx.busy()) return calcu::sessions::replyRefused(busy);
        if (!ctx.machine()) return calcu::sessions::replyRefused(calcu::sessions::kNoMachine);
        auto img = imageOf(a);
        if (auto* why = std::get_if<std::string>(&img)) return calcu::sessions::replyRefused(*why);
        auto req = requestOf(a);
        if (auto* why = std::get_if<std::string>(&req)) {
            // The text is no address: the plan without one, refused with today's hint.
            sharp::loaders::CodePlan p = sharp::loaders::planCode(*ctx.machine(), std::get<0>(img), {});
            auto r = planReply(p);
            r->refusal = *why;
            r->where.clear();
            r->needsAddress = false;   // an address was given; it is no address
            return calcu::sessions::replyGiven(std::move(r));
        }
        return calcu::sessions::replyGiven(
            planReply(sharp::loaders::planCode(*ctx.machine(), std::get<0>(img), std::get<0>(req))));
    });

    setup.command(c::kLoadCode, [this, busy](const Value& a, SessionContext& ctx) -> CommandOutcome {
        if (ctx.busy()) return Refused{busy};
        const std::string name = calcu::sessions::textArg(a, "name");
        if (!ctx.machine()) return Refused{calcu::sessions::kNoMachine};
        auto img = imageOf(a);
        if (auto* why = std::get_if<std::string>(&img)) return Refused{sharp::loaders::refusal(name, *why)};
        auto req = requestOf(a);
        if (auto* why = std::get_if<std::string>(&req)) return Refused{sharp::loaders::refusal(name, *why)};
        auto loaded = sharp::loaders::loadCode(*ctx.machine(), std::get<0>(img), std::get<0>(req));
        if (auto* why = std::get_if<std::string>(&loaded)) return Refused{sharp::loaders::refusal(name, *why)};
        const sharp::loaders::CodeLoaded& code = std::get<sharp::loaders::CodeLoaded>(loaded);
        auto reply = loadedReply(code);
        // AC-13: `run` starts code whose header names an address to start
        // at; without one nothing is started.
        if (!calcu::sessions::flagArg(a, "run") || !code.headerStart) return Done{std::move(reply)};
        const std::string command = code.advice.callCommand;
        reply->started = true;
        calcu::sessions::PayloadPtr payload = std::move(reply);
        return calcu::sessions::StartOperation{[this, payload, command](OperationContext& op) {
            return startOperation(this, op, payload, command);
        }};
    });
}

calcu::sessions::Task<OperationResult> SharpLoaderPart::typeOperation(SharpLoaderPart* self, OperationContext& ctx,
                                                                      std::string listing, std::string name) {
    calcu::machine::Machine* m = ctx.session().machine();
    if (!m) co_return OperationResult{false, calcu::sessions::kNoMachine};
    const BasicTypeResult r = co_await sharp::loaders::typeProgram(ctx, self->m_typing.engine(), *m, listing);
    if (r.ok) co_return OperationResult{};
    std::string reason = r.error;
    if (!r.rejectedLines.empty()) {
        reason += ":";
        for (std::size_t i = 0; i < r.rejectedLines.size(); ++i)
            reason += (i == 0 ? " \"" : ", \"") + r.rejectedLines[i] + "\"";
    }
    if (!reason.empty() && reason.back() != '.') reason += '.';
    reason[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(reason[0])));
    co_return OperationResult{false, sharp::loaders::refusal(name, reason)};
}

calcu::sessions::Task<OperationResult> SharpLoaderPart::startOperation(SharpLoaderPart* self, OperationContext& ctx,
                                                                       calcu::sessions::PayloadPtr loaded,
                                                                       std::string command) {
    calcu::machine::Machine* m = ctx.session().machine();
    if (!m) co_return OperationResult{false, calcu::sessions::kNoMachine};
    calcu::typing::TypeOptions o;
    o.origin = calcu::typing::Origin::Preset;
    const calcu::typing::TypeResult t = co_await calcu::typing::typeTextAndWait(ctx, self->m_typing.engine(), command,
                                                                               /*enter=*/true, o);
    if (!t.done) co_return OperationResult{false, t.reason};
    // D-4: idle is prompt readiness held for 20 frames in a row; D-5: at
    // most the bound (an hour), then done all the same with a log line.
    const calcu::machine::TickRate rate = m->descriptor().tickRate;
    const calcu::machine::Ticks frame = OperationContext::runPiece(rate);
    const auto bound = static_cast<calcu::machine::Ticks>(self->m_idleBound * rate);
    calcu::machine::Ticks spent = 0;
    int held = 0;
    while (held < 20) {
        if (spent >= bound) {
            ctx.session().log("load-code: " + command + " was not idle after " +
                              (self->m_idleBound >= 3600 ? std::string("an hour") : std::to_string(static_cast<int>(self->m_idleBound)) + " s") +
                              " of emulated time; done all the same.");
            break;
        }
        spent += co_await ctx.run(frame);
        calcu::machine::Machine* now = ctx.session().machine();
        auto* support = now ? calcu::machine::first<calcu::typing::TypingSupport>(*now) : nullptr;
        if (support && support->promptReady().value_or(false)) held++;
        else held = 0;
    }
    co_return OperationResult{true, {}, loaded};
}

} // namespace shared
