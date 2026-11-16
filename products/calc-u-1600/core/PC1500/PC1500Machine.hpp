#pragma once
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Connector/BusRomCard.hpp"
#include "../CPU/DebugStop.hpp"
#include "../CPU/LH5801/LH5801.hpp"
#include "../Connector/Ce150Card.hpp"
#include "../Connector/Ce158Port.hpp"
#include "../Connector/ExpansionConnector.hpp"
#include "../Connector/SystemBus.hpp"
#include "../Tape/TapeDeck.hpp"
#include "PC1500Clocks.hpp"
#include "PC1500Display.hpp"
#include "PC1500Memory.hpp"
#include "PC1500TraceFile.hpp"

// ── PC-1500A machine facade ──────────────────────────────────────────────
//
// Standalone PC-1500/1500A core -- not a subset of the PC-1600 dual-CPU
// core. Owns a value-type CPU + memory pair: step()/reset()/pressKey()/
// display() plus the trace/breakpoint passthrough the debugger builds on.
// The ON key isn't in the named-key vocabulary: it is wired to the CPU's
// BFI power-on latch, not the matrix (setOnKeyPressed()).
//
// Threading (ADR 0009): single-threaded, no lock. Everything above the
// core reaches it through PC1500InterfaceMachine (calcu::machine::Machine,
// C1600-1); this class stays the core's own, for the core and its tests.
class PC1500Machine {
public:
    /// LH5801 cycles per second -- the unit runCycles() counts (PC1500Clocks.hpp).
    static constexpr uint32_t kCpuHz = kPC1500CpuHz;

    explicit PC1500Machine(PC1500Variant variant = PC1500Variant::PC1500A);

    PC1500Variant variant() const { return m_memory.variant(); }

    bool loadROM(const uint8_t* data, std::size_t size);
    bool loadROMFile(const std::string& path);
    /// The reset button: CPU and chips reset, RAM kept.
    void reset();
    /// Reset with all RAM cleared to 0x00 first (Machine > Reset All). The
    /// PC-1500 has no separate ALL RESET line; this models pulling the
    /// batteries, like PC1600Machine::allReset().
    void allReset();

    /// Seed the uPD1990AC real-time clock from a host date/time so BASIC's
    /// TIME reads back something sensible instead of 00000. `month` is
    /// 1-12, the rest plain decimals; day-of-week is derived here. A real
    /// PC-1500's RTC is battery-backed and the boot ROM never re-inits it,
    /// so this one seed rides through boot -- but it is deliberately kept
    /// out of reset() so headless tests / the CLI stay deterministic; the
    /// app calls this after reset() with the host time.
    void seedClock(int year, int month, int day, int hour, int minute, int second, int millisecond = 0);

    /// Execute one instruction. Returns the cycle count consumed (0 if
    /// halted with no pending interrupt, or a breakpoint was just hit).
    int step();

    /// Run until either `maxCycles` total cycles have been consumed or a
    /// breakpoint stops execution first. Returns the number of cycles
    /// actually consumed.
    uint64_t runCycles(uint64_t maxCycles);

    /// Optional host callback invoked from inside runCycles() roughly every
    /// `intervalCycles` of emulated time (counted across calls, so many
    /// short runCycles() calls still add up). Lets a caller that drives a
    /// long synchronous preset/program load keep its event loop alive
    /// (repaint, show a progress popup) without the loaders knowing about
    /// it. The hook may read the display; it must not drive the machine. An
    /// empty function (the default) removes it.
    void setYieldHook(std::function<void()> hook, uint64_t intervalCycles);

    // Named-key vocabulary used by preset scripts and the BASIC typer. Unknown
    // names (including "on" -- see class doc comment) are silently ignored,
    // matching a real keyboard's behavior when a matrix position isn't wired
    // to anything.
    void pressKey(const std::string& name);
    void releaseKey(const std::string& name);
    void setOnKeyPressed(bool pressed);


    /// A snapshot of the display, built from the display RAM.
    PC1500Display display() const;

    /// Whether the LCD panel is actually lit -- for the app to blank the
    /// dot-matrix and indicator bar rather than freeze-frame the last
    /// image. True DISP (LH5801::displayOn(), the RDP/SDP flip-flop) isn't
    /// by itself enough: a real multiplexed LCD needs the CPU continuously
    /// re-driving its backplate signals to stay visible at all, and a
    /// genuine power-down (poweredOff(), see LH5801's own doc comment)
    /// stops that regardless of DISP's last logical value -- confirmed by
    /// tracing the real ROM's OFF-key handler, which reaches OFF (0xFD
    /// 0x4C) without ever executing RDP first. So this is off whenever
    /// either signal says the backplate isn't being driven.
    bool isDisplayOn() const {
        return m_cpu.displayOn() && !m_cpu.poweredOff();
    }

    /// Buzzer audio (mono int16 PCM at audioSampleRate()), produced from
    /// the PC6 drive line as emulated time advances -- see PiezoSampler.
    /// drainAudio() moves up to `max` of the oldest samples into `out`.
    size_t drainAudio(int16_t* out, size_t max) {
        return m_memory.piezo().drain(out, max);
    }
    void discardAudio() {
        m_memory.piezo().discard();
    }
    int audioSampleRate() const { return PiezoSampler::kDefaultSampleRate; }

    /// LH5801 cycles consumed since the machine was built (step() and
    /// runCycles() alike, halted and powered-off time included); no reset
    /// clears it. The tick count of the machine interface (C1600-1 AC-1).
    uint64_t elapsedCycles() const { return m_elapsedCycles; }

    /// Key readiness (C1600-1 AC-21; first step 15): the CPU is in the
    /// ROM's key wait WAIT_4_KB (E243-E33D) with bit 0 of CURS_CTRL (7B0E)
    /// clear, and not powered off.
    bool keyReady() const {
        const uint16_t pc = m_cpu.pc();
        return !m_cpu.poweredOff() && pc >= 0xE243 && pc <= 0xE33D && (m_memory.peek(0x7B0E) & 0x01) == 0;
    }
    /// Buzzer line level changes so far (PiezoSampler::edgeCount()).
    uint64_t buzzerEdgeCount() {
        return m_memory.piezo().edgeCount();
    }

    // Debug reads, so a caller can log core-internal state (PC, and RAM bytes like the cursor-key dispatch
    // gate at 0x7B0EH, or the BREAK_LINE pointer at 0x78AEH/0x78AFH) right
    // alongside its own key-event logs, for direct correlation.
    uint8_t debugPeek(uint16_t addr) const {
        return m_memory.peek(addr);
    }
    uint16_t debugPC() const {
        return m_cpu.pc();
    }

    LH5801&       cpu() { return m_cpu; }
    const LH5801& cpu() const { return m_cpu; }
    PC1500Memory&       memory() { return m_memory; }
    const PC1500Memory& memory() const { return m_memory; }

    /// Runs `view(*this)` and returns its result: one consistent snapshot
    /// for the inspector (Core/Debug/Inspect), which reads through the const
    /// accessors only.
    template <class F>
    auto debugInspect(F&& view) const {
        return view(*this);
    }

    // The 40-pin (single-slot) and 60-pin (daisy-chain) connectors, owned
    // by m_memory. No card is attached by default -- tests and the app
    // layer attach directly via these accessors.
    ExpansionConnector& expansionConnector() { return m_memory.expansionConnector(); }
    SystemBus&          systemBus() { return m_memory.systemBus(); }

    /// Takes ownership of `card` and attaches it to the 40-pin
    /// ExpansionConnector -- for callers (the preset loader, the CLI) with
    /// no longer-lived object of their own to hold the card. Replaces any
    /// previously-attached owned card (matches the 40-pin connector's own
    /// single-slot semantics -- see ExpansionConnector::attach()).
    void attachExpansionCard(std::unique_ptr<ExpansionCard> card) {
        m_memory.expansionConnector().attach(card.get());
        m_attachedExpansionCard = std::move(card); // old card freed only after it's unplugged
    }

    // ── CE-150 plotter / printer (60-pin system bus) ─────────────────────
    //
    // The CE-150 plugs into the 60-pin connector (SystemBus): its ROM
    // window (guest 0xA000-0xBFFF, ME0) and its LH5810 register block
    // (guest 0xB008-0xB00F, ME1) are both served by the card off that bus.
    // `attachCE150` builds a card, loads its 8 KB ROM, resets it, and
    // attaches it to the memory's `systemBus()`. A chip/machine reset does not clear the
    // attachment (it does re-anchor the card -- see reset()).
    bool attachCE150(const uint8_t* rom, size_t romSize);
    void detachCE150();
    bool ce150Attached() const { return m_ce150Card != nullptr; }
    /// Direct access, like cpu()/memory(); above the core the plotter's
    /// paper is the card's paper-output capability (LIB-8).
    Ce150Card* ce150Card() { return m_ce150Card.get(); }
    /// The CE-150's paper output, nullptr while it is not plugged in.
    sharp::PlotterPaperOutput* ce150PaperOutput();

    /// Empty when no plotter is attached.
    std::vector<std::string> drainCE150Events();

    // ── Cassette recorder (CE-152 on the CE-150's jacks) ────────────────
    //
    // The recorder outlives attach/detach of the interface: arm it with a
    // WAV to play (CLOAD) or a file to record into (CSAVE); it moves only
    // while a CE-150 REMOTE relay runs it, in emulated time. A recording is
    // saved each time the motor stops.
    bool tapePlay(const std::string& path, std::string& error);
    void tapeRecord(const std::string& path);
    bool tapeEject(std::string* error = nullptr);
    TapeDeck::Status tapeStatus() const;
    /// Why the last automatic save (at a motor stop) failed, once; empty if
    /// it didn't (TapeDeck::takeLastError()).
    std::string tapeTakeLastError();
    /// Direct access, for the core and its tests.
    TapeDeck& tapeDeck() { return m_tapeDeck; }

    // ── CE-158 RS-232C / Centronics interface (60-pin system bus) ────────
    //
    // Attached to the same SystemBus chain as the CE-150, alone or together
    // with it (on hardware it plugs into the PC-1500 directly or into the
    // CE-150's rear connector). `attachCE158` builds a card, loads its
    // 16 KB ROM, resets it and attaches it; the serial link set with
    // setCE158SerialLink() is kept across detach/attach, so the host PTY
    // stays put while the user toggles the interface.
    bool attachCE158(const uint8_t* rom, size_t romSize);
    void detachCE158();

    // ── Bus ROMs (preset `bus-rom:`, Connector/BusRomCard.hpp) ────────────
    //
    // A plain ROM on the same SystemBus chain, in front of every other card
    // there, so a rebuilt ROM shadows a bundled one at the same place. Kept
    // for the machine's lifetime; a preset builds a fresh machine.
    void attachBusRom(std::unique_ptr<BusRomCard> card);
    void detachBusRoms();
    bool ce158Attached() const { return m_ce158.attached(); }
    /// Direct access (see ce150Card()).
    Ce158Card* ce158Card() { return m_ce158.card(); }
    /// Non-owning; the caller keeps `link` alive until it sets another one
    /// (or nullptr) or destroys the machine.
    void setCE158SerialLink(SerialLink* link);
    /// The bytes printed on the Centronics port since the last
    /// call. Empty when no CE-158 is attached.
    std::vector<uint8_t> drainCE158ParallelOutput();

    // Trace/breakpoint passthrough (LH5801 keeps its own trace ring and
    // breakpoint state, see LH5801.hpp).
    void     setTraceFlags(uint32_t flags) { m_cpu.setTraceFlags(flags); }
    uint32_t traceFlags() const { return m_cpu.traceFlags(); }
    uint32_t drainTraceEvents(CpuFrame* out, uint32_t max, uint32_t* outLost) { return m_cpu.drainTraceEvents(out, max, outLost); }

    // ── Headless CPU-trace file (the preset loader's `trace:` step) ──────
    //
    // Unlike the setTraceFlags()/drainTraceEvents() pair above -- which
    // leave draining the ring to the caller -- this captures a full
    // instruction trace to a file entirely inside the Core: step()/
    // runCycles() drain the ring into the file themselves. Used by the
    // GUI's TRACE button and the preset `trace:` step. Don't mix the two
    // APIs on one machine.

    /// Begin capturing to `handle` (open for binary writing; this machine
    /// takes ownership and endCpuTrace() closes it). Sets `flags` as the
    /// active trace level. Returns false and does nothing if `handle` is
    /// null or a capture is already active.
    bool beginCpuTrace(std::FILE* handle, uint32_t flags);

    /// Drain what's left in the ring, write SESSION_END, close the file,
    /// clear the trace flags. No-op if no capture is active.
    void endCpuTrace();

    // A capture to a file (today's tools); the app's records are not one.
    bool cpuTraceActive() const { return m_traceFile && !m_traceFile->buffered(); }
    /// Size of the active capture so far (0 when none); see
    /// PC1500TraceFile::bytesWritten().
    uint64_t cpuTraceBytes() const { return m_traceFile ? m_traceFile->bytesWritten() : 0; }

    // The app's trace (LIB-10 §10.7, the c1600.trace-records kind): the same
    // records, kept in a buffer until the library's trace session takes
    // them (whole records, at most a budget). endTraceRecord() drains what is
    // left and writes SESSION_END; endTraceRecords() turns recording off.
    bool beginTraceRecords(uint32_t flags);
    bool takeTraceRecords(std::vector<uint8_t>& out, uint64_t budget) {
        return m_traceFile ? m_traceFile->take(out, budget) : true;
    }
    void endTraceRecord();
    void endTraceRecords();
    // ── Debugger stops ────────────────────────────────────────────────────
    /// step() and runCycles() latch a stop when the CPU parks on a PC
    /// breakpoint (nothing executed) or completes an instruction whose data
    /// access hit a watch; runCycles() then returns early. Cleared by a
    /// reset.
    DebugStop consumeDebugStop() { return m_debugStop.consume(); }
    /// Memory watches of CPU 1 (the only one); nullptr turns checking off.
    /// Not owned.
    void setWatches(int cpu, WatchSet* watches) {
        if (cpu != 1) return;
        m_watches = watches;
        m_cpu.setWatches(watches);
    }

private:
    PC1500Memory m_memory;
    LH5801       m_cpu;
    WatchSet*    m_watches{nullptr};
    DebugStopLatch m_debugStop;
    std::unique_ptr<ExpansionCard> m_attachedExpansionCard; // see attachExpansionCard()
    TapeDeck m_tapeDeck{double(kPC1500CpuHz)};               // see tapePlay()
    std::unique_ptr<Ce150Card> m_ce150Card;                 // see attachCE150()
    Ce158Port m_ce158;                                      // see attachCE158()
    std::vector<std::unique_ptr<BusRomCard>> m_busRoms;     // see attachBusRom()
    uint64_t m_elapsedCycles{0};                            // see elapsedCycles()

    // See setYieldHook(). m_yieldCountdown only runs down while a hook is set.
    std::function<void()> m_yieldHook;
    uint64_t m_yieldInterval = 0;
    uint64_t m_yieldCountdown = 0;

    // ── Headless CPU-trace file -- see beginCpuTrace() ───────────────────
    std::unique_ptr<PC1500TraceFile> m_traceFile;
    uint32_t m_traceDrainCounter{0};
    // Scratch buffer for one drainTraceEvents() call -- sized to match
    // LH5801's trace ring so a single drain empties it.
    static constexpr uint32_t kTraceDrainBufFrames = 512;
    // Drain the ring every kTraceDrainInterval instructions -- strictly
    // less than kTraceDrainBufFrames so nothing is lost in the common
    // case (PC1500TraceFile::writeGap() covers the pathological one).
    static constexpr uint32_t kTraceDrainInterval = 256;
    CpuFrame m_traceDrainBuf[kTraceDrainBufFrames];
    /// Bodies of the public detach calls (the name is from when the class
    /// had a lock; it has none since M-26, ADR 0009).
    void detachCE150Locked();
    void detachCE158Locked();

    /// Advances everything outside the CPU that runs on real time (RTC,
    /// buzzer, key queue, CE-150/CE-158) by `cycles`. The one place both
    /// step() and runCycles() feed, so they stay in step.
    void advancePeripherals(uint32_t cycles);

    /// Drain the CPU trace ring into m_traceFile; only while m_traceFile is
    /// set.
    void pumpTraceFile();
    /// step()/runCycles() hook: drain the trace ring into the file every
    /// kTraceDrainInterval instructions while a headless capture is active.
    void maybeDrainTrace() {
        if (m_traceFile && ++m_traceDrainCounter >= kTraceDrainInterval) {
            pumpTraceFile();
            m_traceDrainCounter = 0;
        }
    }
};
