#include "PC100C.hpp"
#include <algorithm>
#include <cstring>

using calcu::machine::ControlDescriptor;
using calcu::machine::ControlForm;
using calcu::machine::ControlId;

namespace {

// ── Printer function-code table ───────────────────────────────────────────────
//
// The characters of the 6-bit codes are pc100cpaper::kCharacters (PC100CPaper.hpp).
// PRN_STR[]:    maps a 7-bit function code to a 3-character mnemonic string.
//               The strings are stored right-to-left in the buffer (hardware
//               writes them reversed so that the print-buffer-reversal on output
//               restores the correct reading order).

const char* const* const PRN_CODE = pc100cpaper::kCharacters;

// Reverse-map a single ASCII character to its PRN_CODE index (0 = space/fallback).
// Used to recover raw codes when function-mnemonic characters are stored in the buffer.
uint8_t prnCharToCode(char c) {
    for (int i = 0; i < 64; i++) {
        if (PRN_CODE[i][0] == c && PRN_CODE[i][1] == '\0')
            return static_cast<uint8_t>(i);
    }
    return 0;  // space
}

const struct { uint8_t code; char str[4]; } PRN_STR[] = {
    {0x00, "   "},
    {0x11, " = "},
    {0x12, " - "},
    {0x13, " + "},
    {0x16, " / "},
    {0x17, " x "},
    {0x1A, "xsY"},
    {0x1B, "Y^x"},
    {0x21, "CLR"},
    {0x22, "INV"},
    {0x23, "DPT"},
    {0x26, "CE "},
    {0x27, "+/-"},
    {0x2D, "EE "},
    {0x31, "e^x"},
    {0x33, "x^2"},
    {0x36, "1/x"},
    {0x3C, "sX "},
    {0x3D, "X_Y"},
    {0x51, "LNX"},
    {0x53, "PRM"},
    {0x54, " % "},
    {0x56, "COS"},
    {0x57, "SIN"},
    {0x5D, "TAN"},
    {0x61, "SUM"},
    {0x66, "STO"},
    {0x67, "pi "},
    {0x68, "RCL"},
    {0x69, "S+ "},
    {0x70, "ERR"},
    {0x71, " { "},
    {0x72, " ) "},
    {0x73, "LRN"},
    {0x74, "RUN"},
    {0x76, "HLT"},
    {0x78, "STP"},
    {0x7A, "GTO"},
    {0x7C, "IF "},
    {0x00, ""}   // sentinel
};

} // namespace

std::vector<ControlDescriptor> PC100C::controlDescriptors() {
    // In the order of the printer's row: PRINT, the TRACE switch, ADV.
    return {{kPrint, ControlForm::Key, "PRINT", "PRN"}, {kTrace, ControlForm::Switch, "T"}, {kAdv, ControlForm::Key}};
}

PC100C::PC100C(const std::vector<calcu::machine::SwitchPosition>& switches)
    : m_controls(controlDescriptors(), [this](const ControlId& id, bool on) { onControl(id, on); }) {
    for (auto& s : m_buf) s = " ";
    m_controls.restore(switches);   // no reaction: TRACE starts where it was
    m_trace = m_controls.on(kTrace);
}

void PC100C::onControl(const ControlId& id, bool on) {
    // The keys and the switch close KP/KN at their strobe.
    auto line = [&](unsigned bit) { m_d12 = static_cast<std::uint8_t>(on ? (m_d12 | bit) : (m_d12 & ~bit)); };
    if (id == kPrint) line(1u << 2);
    else if (id == kAdv) line(1u << 0);
    else if (id == kTrace) m_trace = on;
}

std::uint8_t PC100C::keyLines(PrinterPad pad) const {
    switch (pad) {
    case PrinterPad::D0: return 1u << 2;   // KP at D0: a printer is present
    case PrinterPad::D12: return m_d12;
    case PrinterPad::D15: return m_trace ? (1u << 2) : 0;
    }
    return 0;
}

void PC100C::instructionCycle() {
    if (m_busy > 0 && --m_busy == 0) flushPendingCodeLine(kLineInstructions);   // complete: all 7 dot-rows printed
}

void PC100C::word(PrinterWord w, std::uint16_t kr) {
    switch (w) {
    case PrinterWord::Out:   // load one character into the print buffer
        if (m_ready) {
            uint8_t code = static_cast<uint8_t>((kr >> 4) & 0x3Fu);
            uint8_t slot = m_ptr % 20;
            m_buf[slot] = PRN_CODE[code];
            m_codeBuf[slot] = code;
            m_ptr = slot + 1;
        }
        break;
    case PrinterWord::Func:   // load a 3-char function mnemonic (stored reversed)
        if (m_ready) {
            uint8_t code = static_cast<uint8_t>((kr >> 4) & 0x7Fu);
            for (int i = 0; PRN_STR[i].str[0]; i++) {
                if (code == PRN_STR[i].code) {
                    // Store chars reversed so print-reversal restores correct order
                    for (int k = 3; k > 0;) {
                        --k;
                        uint8_t slot = m_ptr % 20;
                        m_buf[slot] = PRN_STR[i].str[k];
                        m_codeBuf[slot] = prnCharToCode(PRN_STR[i].str[k]);
                        m_ptr = slot + 1;
                    }
                    break;
                }
            }
        }
        break;
    case PrinterWord::Clear:   // reset the print buffer; arms the ready gate
        for (auto& s : m_buf) s = " ";
        std::memset(m_codeBuf, 0, sizeof(m_codeBuf));
        m_ptr = 0;
        m_ready = true;
        break;
    case PrinterWord::Step:   // advance one position (blank)
        if (m_ready) {
            uint8_t slot = m_ptr % 20;
            m_buf[slot] = " ";
            m_codeBuf[slot] = 0;
            m_ptr = slot + 1;
        }
        break;
    case PrinterWord::Go:   // print the buffer as a line
        if (m_ready) {
            // PRT.GO while BUSY: the printer aborts the in-progress print and ignores the
            // new one. This is an unhandled hardware edge case (e.g. hacked "1F" keycode)
            // not foreseen by the ROM; there is no retry or recovery — the printer just stops.
            if (m_busy > 0 && m_hasPending) {
                flushPendingIfActive();
                break;   // the new print is silently ignored
            }
            // Buffer is right-to-left: read from position 19 down to 0.
            // Text is committed immediately (always shows full content, even if aborted).
            std::string line;
            for (int i = 19; i >= 0; --i) {
                line += m_buf[i];
                m_pending.codes[static_cast<std::size_t>(19 - i)] = m_codeBuf[i];
            }
            m_hasPending = true;
            m_pendingTop = m_standing;
            emitTextLine(std::move(line), calcu::paper::Position{0, m_standing});
            m_busy = kLineInstructions;
        }
        break;
    case PrinterWord::Feed:
        // Half a line of paper; Adv (key, button or program step) issues two.
        // Busy gates the rate. A line in progress (its code line pending) is
        // ended now: its dots and its paper come before the feed's.
        flushPendingIfActive();
        emitTextLine(std::string(), calcu::paper::Position{0, m_standing});
        feedTo(m_standing + pc100cpaper::kFeed);
        if (m_recordCodes) m_codeLines.emplace_back();   // zero codes, rowCount 7
        m_busy = kFeedInstructions;
        break;
    }
}

void PC100C::flushPendingCodeLine(std::uint32_t elapsed) {
    if (!m_hasPending) return;
    // Rows before the one in progress are on paper; of the row in progress,
    // the columns that have started (right to left), and how long the last
    // of them was heated.
    uint32_t rows = elapsed / kRowInstructions, cols = 0, heat = 0;
    uint32_t into = elapsed % kRowInstructions;
    if (rows < 7 && into >= kRowStart) {
        uint32_t fired = into - kRowStart;
        cols = std::min<uint32_t>(5, fired / kColumnInstructions + 1);
        heat = fired - (cols - 1) * kColumnInstructions;
    }
    m_pending.rowCount = static_cast<uint8_t>(std::min<uint32_t>(7, rows));
    m_pending.partialColumns = static_cast<uint8_t>(cols);
    m_pending.partialHeat = static_cast<uint8_t>(std::min<uint32_t>(255, heat));
    m_hasPending = false;
    // The dots of the rows printed, then the paper they take.
    for (const auto& d : pc100cpaper::lineDots(m_pending.codes, m_pending.rowCount, m_pending.partialColumns,
                                          m_pending.partialHeat, m_pendingTop))
        emitDot(d.at, d.darkness);
    feedTo(m_pendingTop + pc100cpaper::lineHeight(m_pending.rowCount, m_pending.partialColumns));
    if (m_recordCodes) m_codeLines.push_back(m_pending);
}

void PC100C::feedTo(calcu::paper::Coord y) {
    emitFedTo(y);   // before standing() moves (PaperOutputBase::keep)
    m_standing = y;
}

void PC100C::flushPendingIfActive() {
    if (!m_hasPending) return;
    flushPendingCodeLine(kLineInstructions - m_busy);
}

void PC100C::operation() {
    for (auto& s : m_buf) s = " ";
    m_ptr = 0;
    m_ready = false;
    flushPendingIfActive();   // nothing on paper is lost
    m_busy = 0;
    m_controls.release(kPrint);
    m_controls.release(kAdv);
}

std::string PC100C::bufferContent() const {
    // Same read order as the print routine: position 19 (leftmost) → 0 (rightmost).
    std::string result;
    for (int i = 19; i >= 0; --i) result += m_buf[i];
    return result;
}
