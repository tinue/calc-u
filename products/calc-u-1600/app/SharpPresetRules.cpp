#include "app/SharpPresetRules.hpp"

#include "Display/DisplayOutputText.hpp"
#include "Display/LcdScreenshot.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "app/SharpDisplayLines.hpp"
#include "formats/Png.hpp"
#include "presets/Engine.hpp"
#include "presets/Expect.hpp"
#include "presets/IdleWatch.hpp"
#include "typing/Awaitable.hpp"

namespace shared {

using calcu::presets::Applies;
using calcu::presets::ItemResult;

std::vector<std::uint8_t> screenPng(const calcu::machine::DisplayOutput& o, bool pc1600) {
    const GrayImage image = renderLcdImage(lcdBitmapOf(o), pc1600 ? kPC1600ScreenMm : kPC1500ScreenMm);
    calcu::formats::PngWriter png(image.width, image.height, image.pixelsPerMeter());
    std::vector<std::uint8_t> row(static_cast<std::size_t>(image.width) * 3);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::uint8_t g = image.pixels[static_cast<std::size_t>(y) * image.width + x];
            row[static_cast<std::size_t>(x) * 3] = row[static_cast<std::size_t>(x) * 3 + 1] =
                row[static_cast<std::size_t>(x) * 3 + 2] = g;
        }
        png.addRows(row.data(), 1);
    }
    return png.finish();
}

bool SharpModelRules::pc1600() const { return m_model == sharp::kPC1600; }

calcu::presets::IdleRule SharpModelRules::idle() const {
    // AC-45, AC-53: key readiness without a break for 20 frames, asked every
    // frame (a frame: 1/60 s of emulated time, as the cores count it).
    calcu::presets::IdleRule r;
    // Key readiness as C1600-1 AC-21 built it (readyNow unset). On the
    // PC-1600 a frame in which the key matrix is read counts; the Z-80's
    // BASIC reads it for BREAK while a program runs, so a BASIC program that
    // does not wait for the user counts as idle there too (M-46 finding,
    // LIB-6 qa-report): it cannot be told from a program that waits with
    // INKEY$, which must count as idle.
    // While the CE-1600F's motor runs (INIT"X:", SAVE"X:…" write for seconds
    // and read the keyboard meanwhile) the machine is not idle (M-47: the
    // disk steps of make_diskworks_media).
    if (pc1600())
        r.readyNow = [](const calcu::machine::Machine& cm) {
            auto& m = const_cast<calcu::machine::Machine&>(cm);   // capability() reads only
            if (!m.isKeyReady()) return false;
            const sharp::FloppyDriveAccess* drive = sharp::firstOf<sharp::FloppyDriveAccess>(m);
            return !drive || !drive->motorOn();
        };
    r.stretch = calcu::machine::Seconds(20.0 / 60.0);
    const calcu::machine::TickRate rate = sharp::sharpCatalogue().model(calcu::machine::ModelId{m_model})->tickRate;
    r.sample = rate / 60;
    return r;
}

calcu::sessions::Task<ItemResult> SharpModelRules::afterBuild(calcu::presets::RunContext& run) {
    // AC-46: a PC-1500 or PC-1500A whose ROM stops at `NEW0? :CHECK` (a
    // memory change at power-on) gets CL, as a user would press it, at most
    // five times; the PC-1600 presses nothing.
    if (pc1600()) co_return ItemResult{};
    calcu::typing::TypingEngine* engine = run.typing();
    if (!engine) co_return ItemResult{};
    calcu::presets::IdleWatch watch(idle());
    for (int attempt = 0; attempt < 5; ++attempt) {
        calcu::machine::Machine* m = run.machine();
        if (!m) break;
        bool asks = false;
        for (const std::string& row : displayLines(*m, m->readDisplay()).rows)
            asks = asks || row.find("NEW0?") != std::string::npos;
        if (!asks) break;
        // Named locals, not temporaries in the co_await expressions: GCC 13
        // ends in an internal compiler error on those (CI on 0f1d319).
        std::vector<calcu::typing::Press> presses;
        presses.push_back(calcu::typing::Press{calcu::machine::KeyId{"cl"}});
        const calcu::typing::TypeResult t = co_await calcu::typing::typeAndWait(run.op(), *engine, presses);
        if (!t.done) co_return ItemResult{false, t.reason, {}};
        std::optional<calcu::machine::Ticks> limit;
        if (calcu::machine::Machine* now = run.machine()) limit = now->descriptor().tickRate * 12;
        co_await watch.wait(run.op(), limit);
    }
    co_return ItemResult{};
}

std::optional<std::vector<std::uint8_t>> SharpModelRules::screenshot(calcu::machine::Machine&,
                                                                     const calcu::machine::DisplayOutput& o) {
    return screenPng(o, pc1600());
}

calcu::presets::ExpectReading SharpModelRules::compare(calcu::machine::Machine& m, const calcu::machine::DisplayOutput& o,
                                                       const calcu::presets::ExpectText& e) {
    calcu::presets::ExpectReading r;
    if (!o.on) {
        r.displayOff = true;
        r.shows = "nothing (the display is off)";
        return r;
    }
    const calcu::harness::DisplayText t = displayLines(m, o);
    std::string all;
    for (const std::string& row : t.rows) {
        all += (all.empty() ? "" : "\n") + row;
        r.shows += (r.shows.empty() ? "" : " | ") + row;
    }
    r.found = calcu::presets::foundInLines(all, e);
    return r;
}

Applies SharpModelRules::applies(std::string_view k) const {
    // preset.md §7, the three Sharp columns, for the keywords Calc-U-1600's
    // vocabulary has (the reader refuses the TI's as unknown before).
    if (k == "pause" || k == "pause-on-start" || k == "step" || k == "resume") return Applies::Undefined;
    if (k == "debug-panel") return Applies::NotForThisModel;
    if (!pc1600() && (k == "slot-2" || k == "host-drive" || k == "floppy" || k == "slot-2-rom"))
        return Applies::NotForThisModel;
    return Applies::Yes;
}

void SharpModelRules::checkHeader(const calcu::presets::LoadPlan& plan, calcu::presets::ReadContext& ctx) const {
    // AC-47, row 1600-22: the CE-1600P and the CE-158 not together, at the
    // line of the later key.
    const calcu::presets::PlaceRequest *printer = nullptr, *interface = nullptr;
    for (const auto& r : plan.places) {
        if (r.request.peripheral.value == sharp::kCE1600P) printer = &r;
        if (r.request.peripheral.value == sharp::kCE158) interface = &r;
    }
    if (printer && interface)
        ctx.error(std::max(printer->line, interface->line), "The CE-1600P and the CE-158 cannot be used together.");
    checkSharpHeader(plan, ctx);
}

calcu::presets::ModelRules& SharpPresetRules::forModel(const calcu::machine::ModelId& m) {
    if (m.value == sharp::kPC1500) return m_pc1500;
    if (m.value == sharp::kPC1600) return m_pc1600;
    return m_pc1500a;
}

} // namespace shared
