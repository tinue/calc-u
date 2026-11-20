#pragma once
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../Connector/BusRomCard.hpp"
#include "../CPU/DebugStop.hpp"
#include "../CPU/LH5801/LH5801.hpp"
#include "../Machine/SharpMachine.hpp"
#include "../Machine/StateRecords.hpp"
#include "../Serial/SerialPortCapability.hpp"
#include "../Connector/Ce150Card.hpp"
#include "../Connector/Ce158Port.hpp"
#include "../Connector/ExpansionConnector.hpp"
#include "../Connector/SystemBus.hpp"
#include "../Tape/TapeDeck.hpp"
#include "PC1500Clocks.hpp"
#include "PC1500Display.hpp"
#include "PC1500Memory.hpp"
#include "PC1500TraceFile.hpp"

// A PC-1500 or PC-1500A: the core and the calcu::machine::Machine in one
// class. Ticks are LH5801 cycles at 1,300,000 per second. It owns a
// value-type CPU + memory pair: step()/pressKey()/display() plus the
// trace/breakpoint passthrough the debugger builds on. The ON key isn't in
// the named-key vocabulary: it is wired to the CPU's BFI power-on latch, not
// the matrix (setOnKeyPressed()).
//
// Threading: single-threaded, no lock. Above the core it is reached only as
// a calcu::machine::Machine (built by SharpMachineBuilder; the CTest
// CalcU1600MachineIncludeCheck keeps its name out of app/ and the frontends).
class PC1500Machine final : public sharp::SharpMachine {
public:
    /// LH5801 cycles per second -- the unit runCycles() counts (PC1500Clocks.hpp).
    static constexpr uint32_t kCpuHz = kPC1500CpuHz;

    PC1500Machine(const calcu::machine::MachineDescriptor&, const calcu::machine::VariantId&);
    // The model's descriptor of sharpCatalogue(), its default ROM variant.
    explicit PC1500Machine(PC1500Variant model = PC1500Variant::PC1500A);
    ~PC1500Machine() override;

    PC1500Variant modelVariant() const { return m_memory.variant(); }

    calcu::machine::Ticks tickCount() const override { return m_elapsedCycles; }
    calcu::machine::DisplayOutput readDisplay() override;
    std::size_t takeSound(std::span<std::int16_t> out) override { return drainAudio(out.data(), out.size()); }

    bool loadROM(const uint8_t* data, std::size_t size);
    bool loadROMFile(const std::string& path);

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
    /// clears it. The tick count of the machine interface.
    uint64_t elapsedCycles() const { return m_elapsedCycles; }

    /// Key readiness (A of docs/developer/Idle.md), not powered off: the
    /// ROM's key wait WAIT_4_KB (E243-E33D) ran in this frame or the last
    /// with bit 0 of CURS_CTRL (7B0E) clear (by frames: the timer interrupt
    /// runs outside the loop), or INKEY$ read a key (BCMD_INKY D9AA).
    bool keyReady() const {
        if (m_cpu.poweredOff()) return false;
        const bool keyWait = m_frame.keyWait || m_lastFrame.keyWait;
        return (keyWait && (m_memory.peek(0x7B0E) & 0x01) == 0) || m_frame.keyRead || m_lastFrame.keyRead;
    }
    /// The buzzer line changed in this frame or the last (a BEEP sounds).
    bool toneSounding() const { return m_frame.tone || m_lastFrame.tone; }
    /// The CE-150's tape loader waited for an edge in this frame or the last,
    /// and the cassette does not play (none in, the motor off, at its end).
    bool tapeAwaited() const {
        if (!ce150Attached() || !(m_frame.tapeWait || m_lastFrame.tapeWait)) return false;
        const TapeDeck::Status st = m_tapeDeck.status();
        return st.mode != TapeDeck::Mode::Play || !st.motor || m_tapeDeck.atEnd();
    }
    /// The CE-158 waited in its receive loop in this frame or the last.
    bool ce158ReceiveWaitSeen() const { return ce158Attached() && (m_frame.portWait || m_lastFrame.portWait); }
    /// The BASIC program has ended: the editor runs (BUSY, 764E bit 0, off)
    /// and does not wait for INPUT or a PRINT stop (BREAKPARAM 788A bits 5, 6).
    bool programEnded() const {
        return (m_memory.peek(0x764E) & 0x01) == 0 && (m_memory.peek(0x788A) & 0x60) == 0;
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
    /// for the inspector (core/Debug/Inspect), which reads through the const
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
    /// paper is the card's paper-output capability.
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
    /// Direct access, like cpu()/memory().
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
    // leave draining the ring to the caller -- the trace records are
    // captured entirely inside the Core: step()/runCycles() drain the ring
    // into them themselves. Don't mix the two APIs on one machine.

    // The app's trace (the c1600.trace-records kind): the same
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
    // ---- The model's part of SharpMachine ----
    calcu::machine::Ticks runCore(calcu::machine::Ticks n) override { return runCycles(n); }
    sharp::SharpDebugging* sharpDebugging() override { return m_debugging.get(); }
    bool coreSwitchedOff() const override { return m_cpu.poweredOff(); }
    bool coreKeyReady() const override { return keyReady(); }
    void coreKey(const std::string& id, bool down) override;
    void coreOnKey(bool down) override { setOnKeyPressed(down); }
    /// The reset button: CPU and chips reset, RAM kept.
    void coreReset() override;
    /// Reset with all RAM cleared to 0x00 first. The PC-1500 has no separate
    /// ALL RESET line; this models pulling the batteries.
    void coreColdBoot() override;
    ExpansionCard* moduleIn(const calcu::machine::PlaceName&) override;
    void corePlug(const calcu::machine::PeripheralRequest&, const calcu::machine::PeripheralDescriptor&) override;
    void coreUnplug(const calcu::machine::PlaceName&, const calcu::machine::PeripheralId&) override;
    void offer(std::vector<Offered>&, const std::vector<calcu::machine::PlugEntry>&) override;
    // The records: the CPU, the memory with its chips, the emulated time,
    // the cassette deck, every card, then the CE-158's line and the cassette
    // place. The debugger's own state is not in it.
    void coreSaveState(sharp::StateWriter&) const override;
    void coreRestoreState(sharp::StateReader&) override;
    template <class Io>
    void stateIo(Io& io);

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
    // keyReady() and toneSounding(), by 60 Hz frames of LH5801 cycles; not in the
    // state record (after a restore they are at most a frame behind).
    struct FrameNotes {
        bool keyWait = false, keyRead = false, tone = false, tapeWait = false, portWait = false;
    };
    FrameNotes m_frame, m_lastFrame;
    uint64_t m_frameCycles = 0, m_frameEdges = 0;
    void noteFetch(uint16_t pc) {
        if (pc >= 0xE243 && pc <= 0xE33D) m_frame.keyWait = true;
        else if (pc == 0xD9AA) m_frame.keyRead = true;
        else if (pc >= 0x8000 && pc < 0xC000) noteMediumWait(pc);
    }
    // docs/developer/Idle.md, "A medium awaited": the CE-150's tape loader in
    // an edge wait of LOAD_NIBBLE (its ROM, PV 0); the CE-158 in its receive
    // loop with RXCOM (its ROM, PV 1, the region both banks share).
    void noteMediumWait(uint16_t pc) {
        if (!m_cpu.pv()) {
            if ((pc >= 0xBE11 && pc <= 0xBE1C) || (pc >= 0xBE21 && pc <= 0xBE2C)) m_frame.tapeWait = true;
        } else if ((pc >= 0x828E && pc <= 0x82A0) || (pc >= 0x81E6 && pc <= 0x822F)) {
            m_frame.portWait = true;
        }
    }

    // ── The trace records -- see beginTraceRecords() ─────────────────────
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
    /// had a lock; it has none).
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

    struct Kinds;   // the capability objects (PC1500Machine.cpp)
    SerialPortCapability m_ce158Port;   // the CE-158's UART peer; kept across detach and attach
    std::unique_ptr<Kinds> m_kinds;
    std::unique_ptr<sharp::SharpDebugging> m_debugging;   // the debugger: after the core, gone before it
};
