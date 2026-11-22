#pragma once
#include "DebugSnapshots.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Text and colours of the two debug panels (CALCULATOR and CPU) that both
// apps show identically. Colours are packed 0xRRGGBB, as in RomHeatmap.

namespace cu59 {

/// Base font size of both panels by their width: compact / normal / large.
int debugPanelFontSize(double width);

/// The header of a debug tab (CALCULATOR, CPU), the same in every GUI: its
/// title and its run-control buttons left to right, each with its label in
/// a wide and a narrow header and its colour. RESUME sits far from STEP,
/// against a tap meant for the other while stepping. The buttons are drawn
/// at the panel's base font size. Button IDs: resume, freeze,
/// freeze-on-start, armed, step.
struct DebugHeaderButton {
    std::string id, label, narrowLabel;
    std::uint32_t colour;
};
struct DebugHeader {
    std::string title, narrowTitle;
    std::vector<DebugHeaderButton> buttons;
};
DebugHeader debugHeader(bool cpu);
/// A header goes narrow (short title and labels) below this base font size.
inline constexpr double kDebugHeaderNarrowBelow = 13;
inline bool debugHeaderNarrow(double baseFontSize) { return baseFontSize < kDebugHeaderNarrowBelow; }

/// The printer's button bar goes compact (short labels, tighter buttons) on a
/// narrow tape (iOS).
bool compactPrinterBar(double printerWidth);

/// Space (points) above a side-by-side layout. A window with no top inset
/// (iPhone Duo open, iOS 27.1 simulator) doesn't deliver touches in its top
/// ~30 pt — measured: controls centred at y ≤ 29 dead, ≥ 35 working — so the
/// tabs and printer header keep clear of it.
double mobileTopMargin(double topInset);

/// "Steps: 480, Registers: 60 (479.59)" — "(479.)" when no data registers.
std::string partitionLine(int programRegCount, int dataRegCount);

/// "DEG" / "RAD" / "GRAD", "?" when SCOM doesn't say.
std::string angleModeLabel(std::optional<AngleMode> mode);

/// "R05 = 2.5" (H for the TI-58C's extra registers), the full 13 digits:
/// rounding to the display's 10 can hide stored-value bugs.
std::string registerLine(const LiveDebugSnapshot::RegEntry& r);

/// "1: 3.1416" — HIR n (1–8).
std::string hirLine(int n, double value);

/// "T: 2.71828".
std::string tRegisterLine(double value);

/// "X: 1.414213562373" (all 13 digits), or "X: C=…" when X's register holds no number.
std::string xRegisterLine(double value, const std::string& raw);

/// X's text colour: grey as T (dimmer at 0), amber when shown raw.
uint32_t xRegisterColor(double value, const std::string& raw);

/// Return-stack level colour by its source flag: green user RAM, purple
/// module (2 = saved CROM return address), yellow ROM, grey otherwise;
/// dimmer when the level's address is 0.
uint32_t returnLevelColor(uint8_t sourceFlag, bool zeroAddress);

/// Marker after the current program step: "← next" when frozen (the step
/// about to execute), "← running" while running.
std::string currentStepMarker(bool frozen);

/// A program step: "038  42  STO", with the current-step marker on the current one.
std::string programStepLine(const StepEntry& e, bool frozen);

/// Return-stack level n (1–6): "L1:038".
std::string returnLevelLine(int level, int address);

/// A CPU inspector row: "0837  0A12  " and the disassembly.
std::string inspectorLine(const InspectorEntry& e);

/// Background of the current program step: yellow ROM, blue module, green otherwise.
uint32_t stepHighlightColor(ProgramSource source);

/// CPU inspector: COND and IDLE from the CPU flags word.
inline bool condFlag(uint16_t flags) { return (flags & 0x0800) != 0; }
inline bool idleFlag(uint16_t flags) { return (flags & 0x0001) != 0; }

/// A 16-nibble register as hex, most significant nibble first: "00000000000000A5".
std::string registerNibbles(const uint8_t nibbles[16]);

/// "1F0A [0001 1111 0000 1010]".
std::string hexBin16(uint16_t v);

} // namespace cu59
