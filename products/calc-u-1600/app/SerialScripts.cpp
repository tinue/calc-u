#include "app/SerialScripts.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <span>
#include <variant>

#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "files/Files.hpp"

namespace shared {

namespace sessions = calcu::sessions;
using calcu::machine::Machine;
using calcu::machine::Ticks;
using calcu::peripherals::SerialPort;

SerialScriptsPart::SerialScriptsPart(std::function<bool(const std::string&)> linked) : linked_(std::move(linked)) {}

SerialPort* SerialScriptsPart::port(const std::string& word) const {
    Machine* m = ctx_ ? ctx_->machine() : nullptr;
    if (!m) return nullptr;
    for (const auto& e : m->capabilities(SerialPort::kKind)) {
        bool match = false;
        if (const auto* c = std::get_if<calcu::machine::ComponentOwner>(&e.owner))
            match = word == "rs232c" && c->component.value == sharp::kRs232cComponent;
        else if (const auto* p = std::get_if<calcu::machine::PeripheralOwner>(&e.owner))
            match = word == "ce158" && p->peripheral.value == sharp::kCE158;
        if (match) return calcu::machine::capabilityAs<SerialPort>(*m, e.id);
    }
    return nullptr;
}

Ticks SerialScriptsPart::charTicks(const Machine& m, SerialPort& p) const {
    const std::uint32_t baud = p.lineSettings().baud.value_or(300);
    return std::max<Ticks>(1, static_cast<Ticks>(m.descriptor().tickRate) * 10 / (baud ? baud : 300));
}

void SerialScriptsPart::feed(const std::string& word, std::vector<std::uint8_t> bytes, std::uint64_t hold) {
    drain();
    Port& p = ports_[word];
    p.feed = std::move(bytes);
    p.taken = 0;
    Machine* m = ctx_ ? ctx_->machine() : nullptr;
    SerialPort* sp = port(word);
    // The first byte at the first character time after `hold` of them.
    p.due = m && sp ? m->tickCount() + charTicks(*m, *sp) * hold : 0;
}

std::optional<std::string> SerialScriptsPart::capture(const std::string& word, const std::string& path) {
    drain();   // what was sent until now belongs to the file before
    std::ofstream out(calcu::files::toPath(path), std::ios::binary | std::ios::trunc);
    if (!out) return "The file " + path + " cannot be written.";
    ports_[word].capturePath = path;
    return std::nullopt;
}

SerialScriptsPart::Report SerialScriptsPart::report(const std::string& word) {
    drain();
    Report r;
    const Port& p = ports_[word];
    if (SerialPort* sp = port(word)) {
        const calcu::peripherals::LineSettings l = sp->lineSettings();
        r.baud = l.baud.value_or(0);
        r.bitsPerChar = l.bitsPerChar;
        r.used = sp->used();
    }
    r.fed = p.feed.size();
    r.taken = p.taken;
    r.sent = p.sent;
    r.printer = p.printer;
    return r;
}

void SerialScriptsPart::append(const std::string& path, const std::uint8_t* bytes, std::size_t n) {
    std::ofstream out(calcu::files::toPath(path), std::ios::binary | std::ios::app);
    out.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(n));
}

void SerialScriptsPart::drain() {
    if (!ctx_ || !ctx_->machine()) return;
    // The CE-158's printer port (the INSPECTOR's GUI pane drains it in a frontend).
    for (const auto& e : ctx_->machine()->capabilities(sharp::Ce158PrinterPort::kKind))
        if (auto* pp = calcu::machine::capabilityAs<sharp::Ce158PrinterPort>(*ctx_->machine(), e.id)) {
            const std::vector<std::uint8_t> b = pp->drain();
            Port& p = ports_["ce158"];
            p.printer.insert(p.printer.end(), b.begin(), b.end());
        }
    for (const char* word : {"ce158", "rs232c"}) {
        SerialPort* sp = port(word);
        if (!sp || (linked_ && linked_(word))) continue;   // a host link takes the bytes (AC-7 link)
        Port& p = ports_[word];
        std::uint8_t buf[1024];
        for (std::size_t n; (n = sp->takeSent(buf)) > 0;) {
            p.sent.insert(p.sent.end(), buf, buf + n);
            if (!p.capturePath.empty()) append(p.capturePath, buf, n);
        }
    }
}

// ── SessionPart ─────────────────────────────────────────────────────────

void SerialScriptsPart::attach(sessions::SessionSetup& setup) {
    ctx_ = &setup.context();
    ctx_->machineSession().addHook(*this);
}

void SerialScriptsPart::afterCall(sessions::SessionContext&, calcu::pacing::WallTime) { drain(); }

void SerialScriptsPart::beforeMachineChange(sessions::SessionContext&) { drain(); }

void SerialScriptsPart::machineChanged(sessions::SessionContext& c) {
    const std::uint64_t g = c.machineSession().generation();
    if (g == generation_) return;
    generation_ = g;
    // A new machine: what its ports sent starts afresh (AC-7 report); the
    // feeds and capture files stay with the port words.
    for (auto& [word, p] : ports_) {
        p.sent.clear();
        p.printer.clear();
    }
}

void SerialScriptsPart::closing(sessions::SessionContext&) { drain(); }   // the capture files complete (AC-7)

// ── SliceHook ───────────────────────────────────────────────────────────

Ticks SerialScriptsPart::nextDue(Ticks) const {
    Ticks due = sessions::kNever;
    for (const auto& [word, p] : ports_)
        if (p.taken < p.feed.size()) due = std::min(due, p.due);
    return due;
}

void SerialScriptsPart::act(Machine& m, Ticks now, calcu::pacing::WallTime) {
    for (auto& [word, p] : ports_) {
        if (p.taken >= p.feed.size() || p.due > now) continue;
        SerialPort* sp = port(word);
        if (!sp || (linked_ && linked_(word))) {
            p.due = now + 1000;   // no port now: try again later, nothing lost
            continue;
        }
        // One byte in each character time in which the port takes it (AC-7).
        p.taken += sp->putReceived(std::span<const std::uint8_t>(p.feed.data() + p.taken, 1));
        p.due = now + charTicks(m, *sp);
    }
}

} // namespace shared
