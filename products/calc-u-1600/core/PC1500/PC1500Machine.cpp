#include "PC1500Machine.hpp"

PC1500Machine::PC1500Machine(PC1500Variant variant)
    : m_memory(variant), m_cpu(m_memory) {}

bool PC1500Machine::loadROM(const uint8_t* data, std::size_t size) {
    return m_memory.loadROM(data, size);
}

bool PC1500Machine::loadROMFile(const std::string& path) {
    return m_memory.loadROMFile(path);
}

void PC1500Machine::reset() {
    m_memory.reset();
    m_memory.keyboard().releaseAll();
    m_cpu.reset();
    m_debugStop.clear();
    // A chip reset re-anchors an attached CE-150 (LH5810 latches cleared,
    // steppers/pen re-homed) but does NOT unplug it or wipe its paper --
    // real ink stays on real paper. Mirrors the CE-1600P, whose card is
    // likewise left attached across PC1600Machine::reset().
    if (m_ce150Card) m_ce150Card->reset();
    m_ce158.reset();
}

void PC1500Machine::allReset() {
    {
        m_memory.clearRam();
    }
    reset();
}

bool PC1500Machine::attachCE150(const uint8_t* rom, size_t romSize) {
    if (romSize != Ce150Card::kRomSize) return false;
    auto card = std::make_unique<Ce150Card>();
    if (!card->loadRom(rom, romSize)) return false;
    detachCE150Locked();
    card->reset();
    card->connectRecorder(&m_tapeDeck);
    m_memory.systemBus().attach(card.get());
    m_ce150Card = std::move(card);
    return true;
}

bool PC1500Machine::tapePlay(const std::string& path, std::string& error) {
    return m_tapeDeck.loadForPlay(path, error);
}

void PC1500Machine::tapeRecord(const std::string& path) {
    m_tapeDeck.armRecord(path);
}

bool PC1500Machine::tapeEject(std::string* error) {
    return m_tapeDeck.eject(error);
}

TapeDeck::Status PC1500Machine::tapeStatus() const {
    return m_tapeDeck.status();
}

std::string PC1500Machine::tapeTakeLastError() {
    return m_tapeDeck.takeLastError();
}

void PC1500Machine::detachCE150() {
    detachCE150Locked();
}

void PC1500Machine::detachCE150Locked() {
    if (!m_ce150Card) return;
    m_memory.systemBus().detach(m_ce150Card.get());
    m_ce150Card.reset();
}

bool PC1500Machine::attachCE158(const uint8_t* rom, size_t romSize) {
    auto card = m_ce158.build(rom, romSize, Ce158Card::kCpuHz);
    if (!card) return false;
    detachCE158Locked();
    m_memory.systemBus().attach(card.get());
    m_ce158.install(std::move(card));
    return true;
}

void PC1500Machine::attachBusRom(std::unique_ptr<BusRomCard> card) {
    m_memory.systemBus().attachFirst(card.get());
    m_busRoms.push_back(std::move(card));
}

void PC1500Machine::detachBusRoms() {
    for (auto& c : m_busRoms) m_memory.systemBus().detach(c.get());
    m_busRoms.clear();
}

void PC1500Machine::detachCE158() {
    detachCE158Locked();
}

void PC1500Machine::detachCE158Locked() {
    if (!m_ce158.attached()) return;
    m_memory.systemBus().detach(m_ce158.card());
    m_ce158.remove();
}

void PC1500Machine::setCE158SerialLink(SerialLink* link) {
    m_ce158.setSerialLink(link);
}

std::vector<uint8_t> PC1500Machine::drainCE158ParallelOutput() {
    return m_ce158.drainParallelOutput();
}

sharp::PlotterPaperOutput* PC1500Machine::ce150PaperOutput() {
    return m_ce150Card ? &m_ce150Card->paperOutput() : nullptr;
}

// Reads and clears the plotter mechanism's event log, which step() and
// runCycles() fill on every pen change.
std::vector<std::string> PC1500Machine::drainCE150Events() {
    if (!m_ce150Card) return {};
    return m_ce150Card->mechanism().drainEvents();
}

void PC1500Machine::seedClock(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    // Sakamoto's day-of-week: 0 = Sunday. Valid for any Gregorian date;
    // keeps this Core self-contained (no host <ctime> needed) so headless
    // tests can seed a fixed date and check the dow nibble too.
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = (month < 3) ? year - 1 : year;
    int dow = (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;

    m_memory.seedClock(year, month, day, hour, minute, second, dow, millisecond);
}

int PC1500Machine::step() {
    int c = m_cpu.step();
    if (c == 0 && m_cpu.breakpointsEnabled() && m_cpu.consumeBreakpointHit()) m_debugStop.latch(DebugStop::Breakpoint, 1);
    if (m_watches && m_watches->hitPending()) m_debugStop.latch(DebugStop::Watch, 1);
    // PU/PV (SPU/RPU/SPV/RPV) never touch the bus themselves, so pushing
    // their post-instruction state here is sufficient for the next bus
    // access to see it -- see PC1500Memory::updatePUPV()'s own doc comment.
    m_memory.updatePUPV(m_cpu.pu(), m_cpu.pv());
    // Real time keeps passing while halted or powered off, budgeted at
    // kHaltTickCycles like runCycles(). A 0 return otherwise means the CPU
    // is parked on a breakpoint: nothing ran, so no time passes either.
    if (c > 0) advancePeripherals(static_cast<uint32_t>(c));
    else if (m_cpu.halted() || m_cpu.poweredOff()) advancePeripherals(LH5801::kHaltTickCycles);
    maybeDrainTrace();
    return c;
}

void PC1500Machine::advancePeripherals(uint32_t cycles) {
    m_elapsedCycles += cycles;  // every cycle step() and runCycles() consume passes here
    // The uPD1990AC RTC's TP output is defined in real time, not CPU
    // cycles (it has its own independent crystal) -- this Core has no
    // other notion of elapsed time to drive it from, so it advances here,
    // once per instruction, the same way LH5801's own internal timer
    // advances via tickTimer().
    m_memory.advanceRtc(cycles);
    // Per-step hook for an attached CE-150 (no-op today -- the plotter is
    // fully reactive; see Ce150Card::tick()).
    if (m_ce150Card) m_ce150Card->tick(cycles);
    m_memory.advanceSerial(cycles); // SDO -> CMTOUT, the buzzer, and the cassette recorder's time
    m_ce158.tick(cycles); // the UART's own clock keeps running
}

void PC1500Machine::setYieldHook(std::function<void()> hook, uint64_t intervalCycles) {
    m_yieldHook = std::move(hook);
    m_yieldInterval = intervalCycles;
    m_yieldCountdown = intervalCycles;
}

uint64_t PC1500Machine::runCycles(uint64_t maxCycles) {
    uint64_t consumed = 0;
    uint64_t yieldAccounted = 0;
    while (consumed < maxCycles) {
        // See setYieldHook(). Checked at the top of the loop so the halted
        // branch's `continue` below is covered too.
        if (m_yieldHook) {
            const uint64_t ran = consumed - yieldAccounted;
            yieldAccounted = consumed;
            if (ran >= m_yieldCountdown) {
                m_yieldCountdown = m_yieldInterval;
                const std::function<void()> hook = m_yieldHook;
                hook();
            } else {
                m_yieldCountdown -= ran;
            }
        }
        int c = m_cpu.step();
        m_memory.updatePUPV(m_cpu.pu(), m_cpu.pv());
        if (c == 0) {
            if (m_cpu.breakpointsEnabled() && m_cpu.consumeBreakpointHit()) {
                m_debugStop.latch(DebugStop::Breakpoint, 1);
                break;
            }
            if (m_cpu.halted() || m_cpu.poweredOff()) {
                // step() still ticks the timer once per call while halted
                // (see LH5801::step()'s HLT branch) -- keep polling so a
                // loaded timer can eventually wake it, budgeting each poll
                // at kHaltTickCycles the same way step() paces its own
                // internal tick, rather than treating "halted" as "no
                // forward progress possible" (real hardware's HLT-then-
                // timer-interrupt idle loop, confirmed via the boot smoke
                // test, depends on exactly this still advancing). Genuinely
                // powered off (poweredOff()) budgets the same way -- no
                // internal-timer tick happens there (step() returns 0
                // before even reaching it, matching the real cut CPU
                // clock), but the uPD1990AC RTC chip and the host key
                // queue both sit outside the CPU and must keep advancing
                // regardless (design doc: "the RTC keeps advancing while
                // powered off").
                advancePeripherals(LH5801::kHaltTickCycles);
                consumed += static_cast<uint64_t>(LH5801::kHaltTickCycles);
                continue;
            }
        }
        advancePeripherals(static_cast<uint32_t>(c));
        consumed += static_cast<uint64_t>(c);
        maybeDrainTrace();
        if (m_watches && m_watches->hitPending()) { // the watched access's instruction has completed
            m_debugStop.latch(DebugStop::Watch, 1);
            break;
        }
    }
    return consumed;
}

bool PC1500Machine::beginTraceRecords(uint32_t flags) {
    if (m_traceFile) return false;
    m_traceFile = std::make_unique<PC1500TraceFile>();
    m_traceDrainCounter = 0;
    m_cpu.setTraceFlags(flags);
    uint32_t staleLost = 0;
    m_cpu.drainTraceEvents(m_traceDrainBuf, kTraceDrainBufFrames, &staleLost);
    return true;
}

void PC1500Machine::endTraceRecord() {
    if (!m_traceFile) return;
    pumpTraceFile();
    m_traceFile->finish();
    m_cpu.setTraceFlags(TRACE_NONE);
}

void PC1500Machine::endTraceRecords() {
    m_traceFile.reset();
    m_cpu.setTraceFlags(TRACE_NONE);
    m_traceDrainCounter = 0;
}

bool PC1500Machine::beginCpuTrace(std::FILE* handle, uint32_t flags) {
    if (!handle || m_traceFile) return false;
    m_traceFile = std::make_unique<PC1500TraceFile>(handle);
    m_traceDrainCounter = 0;
    m_cpu.setTraceFlags(flags);
    // Discard whatever is already in the ring (and its overflow
    // accounting) so the file starts clean even if tracing was already
    // enabled before this call -- e.g. tools/pc1500_cli.cpp turns the
    // trace flags on for its own ring demo before applying the preset.
    uint32_t staleLost = 0;
    m_cpu.drainTraceEvents(m_traceDrainBuf, kTraceDrainBufFrames, &staleLost);
    return true;
}

void PC1500Machine::endCpuTrace() {
    if (!m_traceFile) return;
    pumpTraceFile();
    m_traceFile->finish();
    m_traceFile.reset();
    m_cpu.setTraceFlags(TRACE_NONE);
    m_traceDrainCounter = 0;
}

void PC1500Machine::pumpTraceFile() {
    // drainTraceEvents() takes LH5801's own ring mutex.
    uint32_t lost = 0;
    uint32_t n = m_cpu.drainTraceEvents(m_traceDrainBuf, kTraceDrainBufFrames, &lost);
    if (lost) m_traceFile->writeGap(lost);
    for (uint32_t i = 0; i < n; i++) m_traceFile->writeFrame(m_traceDrainBuf[i]);
}

void PC1500Machine::pressKey(const std::string& name) {
    m_memory.keyboard().setKeyState(PC1500Keyboard::keyFromName(name), true);
}
void PC1500Machine::releaseKey(const std::string& name) {
    m_memory.keyboard().setKeyState(PC1500Keyboard::keyFromName(name), false);
}
void PC1500Machine::setOnKeyPressed(bool pressed) {
    bool risingEdge = m_memory.setOnKeyPressed(pressed);
    // m_memory.setOnKeyPressed() above only latches the pollable IF bit 1
    // (for BREAK, which needs the CPU already running to poll it) -- a
    // halted or powered-off CPU never polls it, so also raise a real wake
    // here. Two distinct states can leave the CPU unresponsive to
    // anything else, and each needs its own wake shape:
    //
    // - Genuinely powered off (LH5801::poweredOff(), set by the OFF
    //   instruction/0xFD 0x4C -- the ROM's real OFF-key and AUTO POWER OFF
    //   handler both execute it): real hardware has physically cut the
    //   CPU clock, so the ON key's BFI pin restores power with a cold
    //   reset, not a HLT resume -- powerOn() (reset() under the hood).
    // - Merely HLTed with the timer stopped (TM==0, so
    //   LH5801::tickTimer()'s usual wake-on-timer-interrupt can never fire
    //   again): a ROM code path can reach this shape without going through
    //   OFF at all -- wakeFromHalt() is the unconditional (non-IE-gated)
    //   counterpart to requestMaskableInterrupt(), matching the ON key's
    //   real wiring straight to the BFI pin/power latch, not the ordinary
    //   maskable-IRQ line, but (unlike powerOn()) just resumes fetch/
    //   execute right where HLT left off -- no reset.
    //
    // Same shape as PC1600Machine::setOnKeyPressed()'s wakeFromHalt() call
    // for its LH5801-family LH5803 (which has no OFF instruction of its
    // own to distinguish). Rising edge only, matching the latch and a
    // real PB7 edge.
    if (risingEdge) {
        if (m_cpu.poweredOff()) m_cpu.powerOn();
        else m_cpu.wakeFromHalt();
    }
}

PC1500Display PC1500Machine::display() const {
    return PC1500Display(m_memory);
}
