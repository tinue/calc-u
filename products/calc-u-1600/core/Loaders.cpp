#include "Loaders.hpp"

#include <cctype>

#include "Basic/BasicProgramSource.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "PC1500/PC1500BasicLoader.hpp"
#include "PC1500/PC1500MachineCodeLoader.hpp"
#include "PC1600/PC1600BasicLoader.hpp"
#include "PC1600/PC1600MachineCodeLoader.hpp"

namespace sharp::loaders {

using machinecode::hexAddress;

namespace {

// A loader's reason as a sentence: a capital first, a stop at the end.
std::string sentence(std::string s) {
    if (s.empty()) return s;
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    const char last = s.back();
    if (last != '.' && last != '!' && last != '?') s += '.';
    return s;
}

bool mode1(const LoaderMemory& m) { return m.mode() == LoaderMemory::Mode::Mode1; }

} // namespace

LoaderMemory* memoryOf(calcu::machine::Machine& m) { return calcu::machine::first<LoaderMemory>(m); }

std::string refusal(const std::string& name, const std::string& reason) {
    return name.empty() ? reason : name + ": " + reason;
}

// ---- What a file is ----

FileKind fileKind(const std::vector<std::uint8_t>& bytes) {
    using programfile::Kind;
    const programfile::ProgramFile pf = programfile::classify(bytes);
    FileKind k;
    k.kind = pf.kind;
    k.damaged = pf.damaged;
    if (pf.kind == Kind::Other) k.words = programfile::describe(pf.token);
    if (pf.kind == Kind::Headerless && pf.looksLikeCode)
        k.looksLike = pf.token == "raw-z80" ? machinecode::Cpu::Z80 : machinecode::Cpu::LH5803;
    const bool code = pf.kind == Kind::CodeLH5801 || pf.kind == Kind::CodeZ80;
    if (code || pf.kind == Kind::BasicPC1500 || pf.kind == Kind::BasicPC1600) {
        Header h;
        h.name = pf.name;
        h.length = pf.headerPayloadLen;
        if (code) {
            h.loadAddress = pf.loadAddr & 0xFFFF;
            h.bank = pf.kind == Kind::CodeZ80 ? static_cast<int>(pf.loadAddr >> 16) : 0;
            if (pf.autorunAddr != 0) h.start = pf.autorunAddr;
        }
        k.header = h;
    }
    if (pf.fromTape)
        for (const programfile::TapeFile& f : programfile::tapeFiles(bytes, nullptr)) {
            k.tapeFiles.push_back(f.name.empty() ? std::string("(no name)") : f.name);
            k.tapeKinds.push_back(programfile::classify(f.image).kind);
        }
    return k;
}

std::optional<std::vector<std::uint8_t>> image(const std::vector<std::uint8_t>& bytes, int n, std::string* why) {
    std::vector<programfile::TapeFile> files = programfile::tapeFiles(bytes, nullptr);
    if (files.empty()) {
        if (n <= 1) return bytes;   // not a WAV (or nothing on it can be read: the loader says why)
        if (why) *why = "The file is no cassette recording with " + std::to_string(n) + " files.";
        return std::nullopt;
    }
    if (n <= 0) n = 1;
    if (static_cast<std::size_t>(n) > files.size()) {
        if (why)
            *why = "The cassette recording holds " + std::to_string(files.size()) +
                   (files.size() == 1 ? " file." : " files.");
        return std::nullopt;
    }
    return std::move(files[static_cast<std::size_t>(n - 1)].image);
}

// ---- BASIC ----

BasicLoaded loadBasic(LoaderMemory& mem, const std::vector<std::uint8_t>& image) {
    BasicLoaded out;
    // AC-5: the PC-1500's words on a PC-1500 and in MODE 1, the PC-1600's in MODE 0.
    const bool pc1600 = mem.pc1600();
    const basic::TransferModel words =
        pc1600 && !mode1(mem) ? basic::TransferModel::PC1600 : basic::TransferModel::PC1500;
    const basic::BasicProgramSource src = basic::readBasicProgram(image, words);
    if (!src.ok) {
        out.refusal = sentence(src.error);
        if (src.listing && pc1600 && mode1(mem)) out.refusal += " (MODE 1: the listing is read as PC-1500 BASIC.)";
        return out;
    }
    if (!pc1600 && src.source == basic::TransferModel::PC1600) {
        out.refusal = "This is a PC-1600 tokenized BASIC program -- load it on a PC-1600.";
        return out;
    }
    // The ROM's LOAD takes a PC-1600 program in both MODEs; a PC-1500 one
    // only in MODE 1.
    if (pc1600 && !src.listing && src.source == basic::TransferModel::PC1500 && !mode1(mem)) {
        out.refusal = "This is a PC-1500 (CE-158) tokenized BASIC program -- the PC-1600 takes it in MODE 1 only "
                      "(type MODE1 first).";
        return out;
    }
    const BasicLoadResult r = pc1600 ? pc1600LoadBasicPayload(mem, src.payload) : pc1500LoadBasicPayload(mem, src.payload);
    out.ok = r.ok;
    out.start = r.baseAddr;
    out.end = r.endAddr;
    if (!r.ok) out.refusal = sentence(r.error);
    return out;
}

BasicLoaded loadBasic(calcu::machine::Machine& m, const std::vector<std::uint8_t>& image) {
    if (LoaderMemory* mem = memoryOf(m)) return loadBasic(*mem, image);
    BasicLoaded out;
    out.refusal = "No machine is running.";
    return out;
}

// ---- Machine code ----

CodePlan planCode(LoaderMemory& mem, const std::vector<std::uint8_t>& image, const CodeRequest& r) {
    CodePlan p;
    p.target = mem.pc1600() ? machinecode::Target::PC1600 : machinecode::Target::PC1500;
    p.file = machinecode::readFile(image);
    if (mem.pc1600()) p.state = pc1600LoadState(mem);
    p.options.target = p.target;
    p.options.hasAddress = r.address.has_value();
    p.options.address = r.address.value_or(0);
    p.options.hasLength = r.length.has_value();
    p.options.length = r.length.value_or(0);
    p.options.hasCpu = r.cpu.has_value();
    p.options.cpu = r.cpu.value_or(machinecode::Cpu::Z80);
    p.plan = machinecode::planLoad(p.file, p.options, p.state);
    p.refusal = machinecode::refusal(p.plan, p.file, p.options);
    return p;
}

CodePlan planCode(calcu::machine::Machine& m, const std::vector<std::uint8_t>& image, const CodeRequest& r) {
    if (LoaderMemory* mem = memoryOf(m)) return planCode(*mem, image, r);
    CodePlan p;
    p.refusal = "No machine is running.";
    p.plan.error = machinecode::LoadError::BadFile;
    return p;
}

std::string cpuNote(const CodePlan& p) {
    if (p.target != machinecode::Target::PC1600) return {};
    return p.plan.cpu == machinecode::Cpu::LH5803
               ? "MODE 1: LH5801 code is assumed. The address is in the PC-1500 (LH5803) address space, "
                 "where &4000-&7FFF is the internal RAM."
               : "MODE 0: Z-80 code is assumed. The address is a Z-80 address.";
}

std::string where(const CodePlan& p) {
    if (!p.holds()) return {};
    const machinecode::LoadPlan& lp = p.plan;
    const std::string range = hexAddress(lp.addr) + "-" + hexAddress(lp.addr + static_cast<std::uint32_t>(lp.len) - 1);
    if (p.target != machinecode::Target::PC1600) return "Goes to " + range + ".";
    std::string s;
    if (lp.slot == machinecode::Slot::S0) {
        s = "Goes to " + range + " in the internal RAM.";
    } else {
        const std::string n = lp.slot == machinecode::Slot::S1 ? "1" : "2";
        s = p.state.title != 0 ? "Goes into the slot " + n + " program module, the selected program area."
                               : "Goes into the slot " + n + " module, where BASIC's program area starts.";
    }
    const std::string work = machinecode::pc1600WorkAreaWarning(lp.busAddr, lp.len, lp.cpu);
    if (!work.empty()) s += " " + work;
    return s;
}

std::variant<CodeLoaded, std::string> loadCode(LoaderMemory& mem, const std::vector<std::uint8_t>& image,
                                               const CodeRequest& r) {
    const CodePlan p = planCode(mem, image, r);
    if (!p.holds()) return p.refusal;
    const machinecode::LoadPlan& lp = p.plan;
    const bool pc1600 = p.target == machinecode::Target::PC1600;
    std::string error;
    const bool ok = pc1600 ? loadPC1600MachineCode(mem, static_cast<int>(lp.slot), lp.busAddr, p.file.payload.data(),
                                                   lp.len, &error, lp.bank)
                           : loadPC1500MachineCode(mem, lp.busAddr, p.file.payload.data(), lp.len, &error);
    if (!ok) return sentence(error);

    CodeLoaded out;
    std::uint32_t ramStart = 0, ramEnd = 0;
    if (!pc1600) pc1500UserRam(mem, &ramStart, &ramEnd);
    out.advice = machinecode::advice(p.target, lp.slot, lp.bank, lp.addr, lp.len, p.file.autorunAddr, ramStart, ramEnd,
                                     p.state, lp.cpu);
    out.address = lp.addr;
    out.length = lp.len;
    out.header = p.file.header != machinecode::File::Header::None;
    out.range = hexAddress(lp.addr) + "-" + hexAddress(lp.addr + static_cast<std::uint32_t>(lp.len) - 1);
    out.cpu = lp.cpu;
    out.bank = lp.bank;
    out.headerStart = p.file.autorunAddr != 0;
    out.entry = out.headerStart ? p.file.autorunAddr : lp.addr;
    if (pc1600) {
        out.area = machinecode::slotName(lp.slot);
        if (lp.slot == machinecode::Slot::S0) {
            const std::string work = machinecode::pc1600WorkAreaWarning(lp.busAddr, lp.len, lp.cpu);
            if (!work.empty()) out.warnings.push_back(work);   // AC-12
        }
    }
    out.summary = "Loaded " + std::to_string(lp.len) + " bytes at " + hexAddress(lp.addr) + "–" +
                  hexAddress(lp.addr + static_cast<std::uint32_t>(lp.len) - 1);
    if (pc1600 && lp.cpu == machinecode::Cpu::LH5803)
        out.summary += " (LH5803 addresses; Z-80 " + hexAddress(lp.busAddr) + ", " + out.area + ")";
    else if (pc1600 && lp.bank != 0)
        out.summary += " (" + out.area + ", bank " + std::to_string(lp.bank) + ")";
    else if (pc1600)
        out.summary += " (" + out.area + ")";
    return out;
}

std::variant<CodeLoaded, std::string> loadCode(calcu::machine::Machine& m, const std::vector<std::uint8_t>& image,
                                               const CodeRequest& r) {
    if (LoaderMemory* mem = memoryOf(m)) return loadCode(*mem, image, r);
    return std::string("No machine is running.");
}

} // namespace sharp::loaders
