#pragma once
#include "PC100CPaper.hpp"
#include "PrinterPinOut.hpp"
#include "machine/Attach.hpp"
#include "machine/Descriptor.hpp"
#include "peripherals/Controls.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// The PC-100C print cradle as a peripheral in the place `printer`. A core
// object: TIMachine's plug() and
// unplug() own it, and it sees only the signals of the printer connector
// (PrinterPinOut): the character tables, the 20-slot buffer, the ready
// gate, the busy count and the pending code line.
//
// Timings: every measurement was made on a PC-100A, not on a PC-100C; no
// difference between the two is expected
// (docs/developer/Core-Architecture.md, "Printer (PC-100C)").
//
// Its paper is the library's paper output kind: PRT.GO
// puts the line's 20 characters on the text layer at once, at the y where
// the paper stands; when the line ends (the busy count runs out, a cut, a
// feed, an operation or a detach) its dots go down and the paper is fed
// (pc100cpaper::lineDots, lineHeight); a feed gives an empty text line and
// 7.09 units of paper. The code lines (the 20 codes and how far the head
// got) are kept only for the golden runs and the headless tool, while
// recordCodeLines is on.

class PC100C final : public PrinterPinOut, public calcu::paper::PaperOutputBase {
public:
    /// A printed line as the head left it. rowCount is 7 for a complete line; 0–6 when the
    /// print was cut short (a PRT.GO while busy, an operation, a detach).
    /// The head fires a row one dot column at a time, right to left, so a cut
    /// can also leave the rightmost partialColumns columns of the next row.
    /// A feed records a line of zero codes (rowCount 7).
    struct CodeLine {
        std::array<std::uint8_t, 20> codes{};
        std::uint8_t rowCount{7};        ///< dot rows started in full (0–7)
        std::uint8_t partialColumns{0};  ///< columns of row rowCount started before the cut (0–5)
        std::uint8_t partialHeat{0};     ///< cycles the leftmost of them was heated (capped at 255)
    };

    /// Busy after PRT.GO (a printed line) and PRT.FEED (half a line; the
    /// ROM's Adv issues two), in instructions of the machine.
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

    /// The controls of the descriptor: PRINT and ADV keys, TRACE switch.
    static std::vector<calcu::machine::ControlDescriptor> controlDescriptors();
    static inline const calcu::machine::ControlId kPrint{"PRINT"}, kAdv{"ADV"}, kTrace{"TRACE"};

    /// `switches`: TRACE's position (the app session keeps it).
    explicit PC100C(const std::vector<calcu::machine::SwitchPosition>& switches);
    PC100C(const PC100C&) = delete;
    PC100C& operator=(const PC100C&) = delete;

    // PrinterPinOut
    void instructionCycle() override;
    void word(PrinterWord, std::uint16_t kr) override;
    bool busy() const override { return m_busy > 0; }
    std::uint8_t keyLines(PrinterPad) const override;

    // PaperOutput
    const calcu::paper::PaperDescription& description() const override { return pc100cpaper::description(); }
    calcu::paper::Coord standing() const override { return m_standing; }

    /// Before a detach: a line being printed ends where the head is,
    /// so its rows go onto the paper before the printer goes.
    void endLineHere() { flushPendingIfActive(); }
    /// Keep the code lines from now on, and take those kept.
    void recordCodeLines(bool on) { m_recordCodes = on; }
    std::vector<CodeLine> takeCodeLines() { return std::move(m_codeLines); }

    /// A reset, power cycle or cold boot of the machine: the
    /// buffer and the ready gate clear, a line in progress ends at its count,
    /// PRINT and ADV are released. TRACE stays.
    void operation();
    /// The content of the character buffer, not yet printed (debug view).
    std::string bufferContent() const;
    calcu::peripherals::ControlTable& controls() { return m_controls; }

    /// The mechanism and its line buffers; the paper itself is
    /// the paper output's, not machine state.
    template <class Io>
    void stateIo(Io& io) {
        io.begin("PRN ");
        io(m_d12)(m_trace)(m_buf)(m_codeBuf)(m_ptr)(m_ready)(m_busy);
        io(m_pending.codes)(m_pending.rowCount)(m_pending.partialColumns)(m_pending.partialHeat);
        io(m_hasPending)(m_standing)(m_pendingTop);
        io.end();
    }

private:
    void onControl(const calcu::machine::ControlId&, bool on);
    void flushPendingCodeLine(std::uint32_t elapsed);   // the pending line cut `elapsed` instructions after PRT.GO
    void flushPendingIfActive();                         // at the current busy count; nothing if no line is pending
    void feedTo(calcu::paper::Coord y);                  // emitFedTo, then the paper stands there

    calcu::peripherals::ControlTable m_controls;
    std::uint8_t m_d12 = 0;      // PRINT (KP) and ADV (KN) at D12
    bool m_trace = false;        // TRACE (KP) at D15

    std::string m_buf[20];       // 20-slot accumulator (right to left on output)
    std::uint8_t m_codeBuf[20]{};
    std::uint8_t m_ptr = 0;
    bool m_ready = false;        // after the first PRT.CLEAR; gates OUT, FUNC, STEP, GO
    std::uint32_t m_busy = 0;    // instructions until Busy drops
    CodeLine m_pending{};
    bool m_hasPending = false;
    calcu::paper::Coord m_standing = 0;   ///< where the paper stands (printer coordinates)
    calcu::paper::Coord m_pendingTop = 0; ///< the top of the pending line
    bool m_recordCodes = false;
    std::vector<CodeLine> m_codeLines;
};
