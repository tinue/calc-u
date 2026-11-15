#pragma once
#include "PrinterOutput.hpp"
#include "PrinterPinOut.hpp"
#include "TIPrinterTypes.hpp"
#include "machine/Attach.hpp"
#include "machine/Descriptor.hpp"
#include "peripherals/Controls.hpp"
#include <cstdint>
#include <string>
#include <vector>

// The PC-100C print cradle as a peripheral in the place `printer` (C59-6
// design §3.3; AC-1 to AC-6, EC-2). A core object: TIMachine's plug() and
// unplug() own it, and it sees only the signals of the printer connector
// (PrinterPinOut). Moved out of TMC0501 in M-25: the character tables, the
// 20-slot buffer, the ready gate, the busy count and the pending code line.
//
// Timings: every measurement was made on a PC-100A, not on a PC-100C; no
// difference between the two is expected (user, 2026-11-11; spec AC-4;
// reference/CoreArchitecture.md, "Printer (PC-100C)").

class PC100C final : public PrinterPinOut, public PrinterOutput {
public:
    /// Busy after PRT.GO (a printed line) and PRT.FEED (half a line; the
    /// ROM's Adv issues two), in instructions of the machine (AC-4).
    static constexpr std::uint32_t kLineInstructions = 4165;
    static constexpr std::uint32_t kFeedInstructions = 1370;
    /// A printed line is 7 dot-row strokes from PRT.GO on; each fires the 5
    /// dot columns of all 20 characters one column at a time, right to left:
    /// column c (5 = rightmost) of row r starts r·508 + 18 + (5 − c)·64
    /// instructions after PRT.GO (measured on a TI-58C and a TI-59 with one
    /// PC-100A: examples/debug/printer-interrupt-sweep.ti58c, printer-interrupt-bits.ti59).
    static constexpr std::uint32_t kRowInstructions = 508;
    static constexpr std::uint32_t kRowStart = 18;
    static constexpr std::uint32_t kColumnInstructions = 64;

    /// The controls of the descriptor: PRINT and ADV keys, TRACE switch (AC-5).
    static std::vector<calcu::machine::ControlDescriptor> controlDescriptors();
    static inline const calcu::machine::ControlId kPrint{"PRINT"}, kAdv{"ADV"}, kTrace{"TRACE"};

    /// `switches`: TRACE's position (LIB-2 D-3; the app session keeps it, AC-6).
    explicit PC100C(const std::vector<calcu::machine::SwitchPosition>& switches);
    PC100C(const PC100C&) = delete;
    PC100C& operator=(const PC100C&) = delete;

    // PrinterPinOut
    void instructionCycle() override;
    void word(PrinterWord, std::uint16_t kr) override;
    bool busy() const override { return m_busy > 0; }
    std::uint8_t keyLines(PrinterPad) const override;

    // PrinterOutput
    std::vector<std::string> takeLines() override { return std::move(m_lines); }
    std::vector<PrinterCodeLine> takeCodeLines() override { return std::move(m_codeLines); }
    void endLineHere() override { flushPendingIfActive(); }

    /// A reset, power cycle or cold boot of the machine (LIB-2 AC-10): the
    /// buffer and the ready gate clear, a line in progress ends at its count,
    /// PRINT and ADV are released (2.3.0 wiped the key matrix). TRACE stays.
    void operation();
    /// The content of the character buffer, not yet printed (debug view).
    std::string bufferContent() const;
    calcu::peripherals::ControlTable& controls() { return m_controls; }

private:
    void onControl(const calcu::machine::ControlId&, bool on);
    void flushPendingCodeLine(std::uint32_t elapsed);   // the pending line cut `elapsed` instructions after PRT.GO
    void flushPendingIfActive();                         // at the current busy count; nothing if no line is pending

    calcu::peripherals::ControlTable m_controls;
    std::uint8_t m_d12 = 0;      // PRINT (KP) and ADV (KN) at D12
    bool m_trace = false;        // TRACE (KP) at D15

    std::string m_buf[20];       // 20-slot accumulator (right to left on output)
    std::uint8_t m_codeBuf[20]{};
    std::uint8_t m_ptr = 0;
    bool m_ready = false;        // after the first PRT.CLEAR; gates OUT, FUNC, STEP, GO
    std::uint32_t m_busy = 0;    // instructions until Busy drops
    PrinterCodeLine m_pending{};
    bool m_hasPending = false;
    std::vector<std::string> m_lines;
    std::vector<PrinterCodeLine> m_codeLines;
};
