// This feature's keywords (LIB-6 design §3.5; spec AC-4, AC-20 to AC-28,
// AC-66): `keys:` is the reader's; here the steps key, type, wait,
// regular-speed, full-speed, elapsed-time, expect, not-expect, screenshot
// and sync-clock.
#include "files/Files.hpp"
#include "formats/Duration.hpp"
#include "formats/Flow.hpp"
#include "machine/Names.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "presets/Engine.hpp"
#include "presets/Expect.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/Awaitable.hpp"
#include "typing/KeyNames.hpp"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace calcu::presets {

using formats::YamlNode;
using machine::Ticks;
using sessions::Task;

namespace {

// The value of a raw step (`type`, `expect`, `not-expect`; D-4) as written,
// quotes and `#` included; from a command (not raw), formats::scalarText.
std::string rawOrScalarText(const YamlNode& n) { return n.rawLine ? n.scalar : formats::scalarText(n); }

ItemResult failed(std::string why) { return ItemResult{false, std::move(why), {}}; }

// ── key, type (LIB-5's; AC-28) ───────────────────────────────────────────

std::vector<std::string> words(const std::string& s) {
    std::vector<std::string> out;
    std::string w;
    for (char c : s + " ") {
        if (c == ' ') {
            if (!w.empty()) out.push_back(w);
            w.clear();
        } else {
            w += c;
        }
    }
    return out;
}

class KeyItem final : public Item {
public:
    KeyItem(std::vector<typing::Press> p, std::string w) : presses_(std::move(p)), written_(std::move(w)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        typing::TypingEngine* engine = run.typing();
        if (!engine) co_return failed("This product types no keys.");
        const typing::TypeResult r = co_await typing::typeAndWait(
            run.op(), *engine, presses_, typing::TypeOptions{typing::Origin::Preset, run.firstPressAtOnce(), true});
        co_return r.done ? ItemResult{} : failed(r.reason);
    }
    After after() const override { return After::Idle; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "key: " + written_; }

private:
    std::vector<typing::Press> presses_;
    std::string written_;
};

std::unique_ptr<Item> readKey(const YamlNode& v, ReadContext& ctx) {
    const std::string written = formats::scalarText(v);
    const std::vector<std::string> names = words(written);
    if (names.empty()) {
        ctx.error(v.line, "key: needs one or more key names.");
        return nullptr;
    }
    // LIB-5 AC-15: found when the preset is read
    auto presses = typing::pressesOfNames(ctx.typingProfile(), ctx.model(), names);
    if (const auto* no = std::get_if<typing::KeyNamesRefused>(&presses)) {
        ctx.error(v.line, no->together ? "The keys " + written + " cannot be pressed on the " + ctx.model().model.value + "."
                                       : no->name + " is not a key of the " + ctx.model().model.value + ".");
        return nullptr;
    }
    return std::make_unique<KeyItem>(std::get<std::vector<typing::Press>>(std::move(presses)), written);
}

class TypeItem final : public Item {
public:
    explicit TypeItem(std::string t) : text_(std::move(t)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        typing::TypingEngine* engine = run.typing();
        if (!engine) co_return failed("This product types no text.");
        const typing::TypeResult r = co_await typing::typeTextAndWait(
            run.op(), *engine, text_, true, typing::TypeOptions{typing::Origin::Preset, run.firstPressAtOnce()});
        co_return r.done ? ItemResult{} : failed(r.reason);
    }
    After after() const override { return After::Idle; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "type: " + text_; }

private:
    std::string text_;
};

std::unique_ptr<Item> readType(const YamlNode& v, ReadContext& ctx) {
    const typing::TypingProfile* p = ctx.typingProfile();
    if (!ctx.typesText() || !p || !p->charKeys || !p->enter) {   // LIB-5 AC-16; AC-40
        ctx.error(v.line, "type: is not available on the " + ctx.model().model.value + ".");
        return nullptr;
    }
    const std::string text = rawOrScalarText(v);
    for (char32_t c : typing::decodeUtf8(text))
        if (!p->charKeys(c, nullptr)) {
            ctx.error(v.line, "No key types the character \"" + typing::encodeUtf8(c) + "\" on the " +
                                  ctx.model().model.value + ".");
            return nullptr;
        }
    return std::make_unique<TypeItem>(text);
}

// ── wait, speed ──────────────────────────────────────────────────────────

Ticks ticksOfMicros(std::int64_t us, machine::TickRate rate) {
    const auto u = static_cast<std::uint64_t>(us);
    return (u / 1'000'000) * rate + ((u % 1'000'000) * rate + 999'999) / 1'000'000;
}

class WaitItem final : public Item {
public:
    WaitItem(std::int64_t us, std::string w) : us_(us), written_(std::move(w)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        machine::Machine* m = run.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        if (run.session().machineSession().paused()) {   // AC-19: the host's time, the machine still paused
            co_await run.op().hostWait(std::chrono::microseconds(us_));
            co_return ItemResult{};
        }
        const bool full = run.op().speed() == pacing::Speed::Full;
        if (full) run.op().setReportsWaiting(false);   // LIB-17 AC-16: as fast as the host allows
        co_await run.op().run(ticksOfMicros(us_, m->descriptor().tickRate));
        if (full) run.op().setReportsWaiting(true);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "wait: " + written_; }

private:
    std::int64_t us_;
    std::string written_;
};

std::unique_ptr<Item> readWait(const YamlNode& v, ReadContext& ctx) {
    std::string error;
    const std::string w = formats::scalarText(v);
    const auto us = formats::parseDuration(w, &error);   // AC-20: a unit
    if (!us) {
        ctx.error(v.line, error);
        return nullptr;
    }
    return std::make_unique<WaitItem>(*us, w);
}

class SpeedItem final : public Item {
public:
    explicit SpeedItem(pacing::Speed s) : speed_(s) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        run.op().setSpeed(speed_);   // AC-22
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return speed_ == pacing::Speed::Full ? "full-speed" : "regular-speed"; }

private:
    pacing::Speed speed_;
};

// ── elapsed-time, expect, not-expect, screenshot, sync-clock ─────────────

class ElapsedItem final : public Item {
public:
    explicit ElapsedItem(bool start) : start_(start) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        machine::Machine* m = run.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        ElapsedBase& base = run.elapsed();
        const std::uint64_t gen = run.session().machineSession().generation();
        if (start_) {
            base = ElapsedBase{gen, m->tickCount()};
            co_return ItemResult{};
        }
        // D-10: from tick 0 of the machine, unless a start was taken on it.
        const Ticks from = base.generation == gen && base.tick && *base.tick <= m->tickCount() ? *base.tick : 0;
        const Ticks since = m->tickCount() - from;
        const std::string text = "Elapsed: " + elapsedText(since, m->descriptor().tickRate);
        run.log(text);   // AC-23
        co_return ItemResult{true, {}, text};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return start_ ? "elapsed-time: start" : "elapsed-time: report"; }

private:
    bool start_;
};

std::unique_ptr<Item> readElapsed(const YamlNode& v, ReadContext& ctx) {
    const std::string w = machine::lowerAscii(formats::scalarText(v));
    if (w != "start" && w != "report") {
        ctx.error(v.line, "elapsed-time: takes start or report.");
        return nullptr;
    }
    return std::make_unique<ElapsedItem>(w == "start");
}

class ExpectItem final : public Item {
public:
    ExpectItem(ExpectText t, bool negate) : text_(std::move(t)), negate_(negate) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        machine::Machine* m = run.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        const machine::DisplayOutput d = run.display();
        const ExpectReading r = run.rules().compare(*m, d, text_);
        if (negate_) {   // AC-66; D-14: a display that is off shows nothing
            if (!r.found) co_return ItemResult{};
            co_return failed("Not expected: " + text_.written + "; the display shows " + r.shows + ".");
        }
        if (r.displayOff) co_return failed("Expected " + text_.written + ", but the display is off.");   // EC-7
        if (r.found) co_return ItemResult{};
        co_return failed("Expected " + text_.written + "; the display shows " + r.shows + ".");   // AC-24
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return (negate_ ? "not-expect: " : "expect: ") + text_.written; }

private:
    ExpectText text_;
    bool negate_;
};

std::function<std::unique_ptr<Item>(const YamlNode&, ReadContext&)> readExpect(bool negate) {
    return [negate](const YamlNode& v, ReadContext& ctx) -> std::unique_ptr<Item> {
        auto parsed = parseExpectText(rawOrScalarText(v));
        if (auto* why = std::get_if<std::string>(&parsed)) {
            ctx.error(v.line, *why);
            return nullptr;
        }
        ExpectText t = std::get<ExpectText>(std::move(parsed));
        if (auto why = ctx.rules().checkExpect(t, negate ? "not-expect:" : "expect:")) {   // AC-68
            ctx.error(v.line, *why);
            return nullptr;
        }
        return std::make_unique<ExpectItem>(std::move(t), negate);
    };
}


class ScreenshotItem final : public Item {
public:
    explicit ScreenshotItem(std::string name) : name_(std::move(name)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        machine::Machine* m = run.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        const auto png = run.rules().screenshot(*m, run.display());
        if (!png) co_return failed("There is no picture of the display.");
        if (const auto folder = run.screenshotFolder()) {   // AC-25: the folder for screenshots
            const std::filesystem::path p = files::toPath(*folder) / files::toPath(name_);
            std::ofstream out(p, std::ios::binary | std::ios::trunc);
            out.write(reinterpret_cast<const char*>(png->data()), static_cast<std::streamsize>(png->size()));
            if (!out) co_return failed("The screenshot " + name_ + " cannot be written to " + *folder + ".");
            co_return ItemResult{};
        }
        sessions::SessionContext& s = run.session();
        if (!s.hostOffers(sessions::HostService::KeepFile))
            co_return failed("There is no folder for screenshots.");
        // The Web: the picture handed to the user to keep (D-13).
        s.askHost(sessions::HostService::KeepFile,
                  sessions::Value::object({{"name", name_}, {"bytes", sessions::Value(*png)}}), nullptr);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "screenshot: " + name_; }

private:
    std::string name_;
};

std::unique_ptr<Item> readScreenshot(const YamlNode& v, ReadContext& ctx) {
    const std::string n = formats::scalarText(v);
    if (n.empty() || n.find('/') != std::string::npos || n.find('\\') != std::string::npos) {
        ctx.error(v.line, "screenshot: takes a file name without a folder.");
        return nullptr;
    }
    return std::make_unique<ScreenshotItem>(n);
}

class SyncClockItem final : public Item {
public:
    Task<ItemResult> carryOut(RunContext& run) override {
        machine::Machine* m = run.machine();
        if (!m) co_return failed(sessions::kNoMachine);
        const peripherals::HostDateTime t = peripherals::hostDateTime(run.session().hostTime());
        peripherals::setClocksFromHost(*m, t);   // AC-27
        char buf[64];
        std::snprintf(buf, sizeof buf, "Clock set to %04d-%02d-%02d %02d:%02d:%02d.", t.year, t.month, t.day, t.hour,
                      t.minute, t.second);
        run.log(buf);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "sync-clock"; }
};

} // namespace

void addLibraryKeywords(Vocabulary& v) {
    v.add(StepDef{"key", StepDef::Value::Scalar, readKey, false, false});
    v.add(StepDef{"type", StepDef::Value::Raw, readType, false, false});
    v.add(StepDef{"wait", StepDef::Value::Scalar, readWait, false, false});
    v.add(StepDef{"regular-speed", StepDef::Value::None,
                  [](const YamlNode&, ReadContext&) -> std::unique_ptr<Item> {
                      return std::make_unique<SpeedItem>(pacing::Speed::Regular);
                  },
                  false, false});
    v.add(StepDef{"full-speed", StepDef::Value::None,
                  [](const YamlNode&, ReadContext&) -> std::unique_ptr<Item> {
                      return std::make_unique<SpeedItem>(pacing::Speed::Full);
                  },
                  false, false});
    v.add(StepDef{"elapsed-time", StepDef::Value::Scalar, readElapsed, true, false});
    v.add(StepDef{"expect", StepDef::Value::Raw, readExpect(false), true, false});
    v.add(StepDef{"not-expect", StepDef::Value::Raw, readExpect(true), true, false});
    v.add(StepDef{"screenshot", StepDef::Value::Scalar, readScreenshot, true, false});
    v.add(StepDef{"sync-clock", StepDef::Value::None,
                  [](const YamlNode&, ReadContext&) -> std::unique_ptr<Item> { return std::make_unique<SyncClockItem>(); },
                  true, false});
}

} // namespace calcu::presets
