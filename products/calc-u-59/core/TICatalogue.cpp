#include "TICatalogue.hpp"
#include "TIKeys.hpp"
#include "TIMachine.hpp"
#include "TIRomText.hpp"
#include <string>

using namespace calcu::machine;

namespace {

// Tick rates (machine.md M-1, fit table): the resonator period. TI-59 455 kHz,
// TI-58 384 kHz, ±1% ceramic resonators (TP0240 clock chip,
// http://www.datamath.org/Chips/TP0240.htm; the service manual names 455 kHz
// and allows the TI-58 a 384 kHz part, "the preferred part"). The TI-58C has
// no clock chip and no resonator: its clock comes from the oscillator built
// into a TMC0580 DSCOM (CD2400 or CD2401), set by external R/C parts
// (http://www.datamath.org/Chips/TMC0580.htm; reference/CoreArchitecture.md,
// "TI-58C clock"). Its rate varies by unit; 394 kHz here is one real TI-58C's
// clock, from its error blinks; the emulator matches that unit's instruction
// counts exactly, but its rate wanders ~0.8% between sessions and other units
// run at PHI 194.7-200 kHz. One instruction is 32 ticks (÷ 2 two-phase ÷ 16
// digit-serial), 128 in the ROM's IDLE state on the TI-59 and TI-58 (C59-1 AC-1).
constexpr TickRate kTI59Rate = 455'000, kTI58Rate = 384'000, kTI58CRate = 394'000;

// The ROM files of roms/fetch_roms.sh (LIB-12 AC-9): the three instruction
// chips in the order they are concatenated, then the two constant files.
std::vector<RomEntry> romEntries(bool ti58c) {
    auto text = [](const char* name, std::uint64_t amount, ContentUnit unit, const char* md5) {
        return RomEntry{name, RomEncoding::Text, amount, unit, md5};
    };
    if (ti58c)
        return {text("CD2400.txt", 2560, ContentUnit::Words, "638c4d0c8359e00cf1f600884f43b789"),
                text("CD2401.txt", 2560, ContentUnit::Words, "3b0ec122cd34b214477780fd405ffdd8"),
                text("TMC0573.txt", 1024, ContentUnit::Words, "6c0cb7db542f3b74a28613ff4ed14bce"),
                text("CD2400-CONST-K.txt", 32, ContentUnit::Rows, "dbaee54fe0f478c7659d17bff4b790d4"),
                text("CD2401-CONST-K.txt", 32, ContentUnit::Rows, "ac81d5b45c736a0f45c3835f0d977b87")};
    return {text("TMC0582.txt", 2560, ContentUnit::Words, "c1cb6d40e063aae1ba7b851b11b1c18a"),
            text("TMC0583.txt", 2560, ContentUnit::Words, "2341a8e484c31ff24a16e70c14c1b3a3"),
            text("TMC0571B.txt", 1024, ContentUnit::Words, "599050d2b1c1900e64b56c089338d32e"),
            text("TMC0582-CONST-K.txt", 32, ContentUnit::Rows, "bf4fb6005177ffdec8f8ceee7e7e16d6"),
            text("TMC0583-CONST-K.txt", 32, ContentUnit::Rows, "81724f14b2827410650f032ef352b1aa")};
}

MachineDescriptor descriptor(const char* model, TickRate rate, bool ti59, bool ti58c) {
    MachineDescriptor d;
    d.model = ModelId{model};
    d.family = kTIFamily;
    d.roms = romEntries(ti58c);
    d.tickRate = rate;
    SegmentLayout layout;
    for (const char* s : {"A", "B", "C", "D", "E", "F", "G"}) layout.segments.push_back({s, false});
    layout.segments.push_back({"DP", true});
    d.display.geometry = SegmentGeometry{12, layout, true};
    d.display.indicators = {{"C", true}};
    d.hasSound = false;
    for (const auto& k : tiKeys()) {
        KeyDescriptor kd{KeyId{std::string(k.id)}, {}};
        for (auto s : k.otherSpellings) kd.otherSpellings.emplace_back(s);
        d.keys.push_back(std::move(kd));
    }
    // Functional pin-outs, one peripheral each; a change is followed by a
    // power cycle (LIB-2 AC-24; ledger row 59-11; C59-6 owns the peripherals).
    d.connectors = {
        {ConnectorId{"module-slot"}, "8 contacts",
         {{PinOutId{"module-slot"}, PinOutLevel::Functional, Takes::One, AfterChange::PowerCycle}}},
        {ConnectorId{"printer-connector"}, "11 pads",
         {{PinOutId{"printer-connector"}, PinOutLevel::Functional, Takes::One, AfterChange::PowerCycle}}},
    };
    d.places = {{PlaceName{"slot-1"}, ConnectorId{"module-slot"}, PinOutId{"module-slot"}, 1},
                {PlaceName{"printer"}, ConnectorId{"printer-connector"}, PinOutId{"printer-connector"}, 1}};
    // Depth as the machine is built (machine.md fit table, "Depth today";
    // ADR 0011, 0012; reference/CoreArchitecture.md § Component Overview). From
    // C59-6 (M-25) the module and the printer interface no longer fold into
    // TMC0501.
    const ComponentId cpu{"TMC0501"};
    d.components = {
        {cpu, "TMC0501 processor", Depth::Separate, std::nullopt},
        {ComponentId{"SCOM"}, ti58c ? "CD2400/CD2401 SCOM registers" : "TMC0582/TMC0583 SCOM registers",
         Depth::Folded, cpu},
        {ComponentId{"ROM"}, "instruction ROM", Depth::Separate, std::nullopt},
        {ComponentId{"RAM"}, "data RAM", Depth::Separate, std::nullopt},
        {ComponentId{"CROM"}, "Solid State ROM module interface", Depth::Folded, cpu},
        {ComponentId{"printer-interface"}, "PC-100C interface", Depth::Folded, cpu},
        {ComponentId{"module-slot"}, "module slot", Depth::Functional, std::nullopt},
        {ComponentId{"printer-connector"}, "printer connector", Depth::Functional, std::nullopt},
    };
    if (ti59) {
        d.components.push_back({ComponentId{"card-reader"}, "magnetic card reader", Depth::Folded, cpu});
        d.mediaPlaces = {{MediaPlaceId{"card-reader"}, "magnetic-card", MediaPassage::PassThrough, {0, 1}}};
    }
    d.debugTargets = {DebugTargetId{"TMC0501"}};
    return d;
}

} // namespace

const Catalogue& tiCatalogue() {
    static const Catalogue c({descriptor("TI-59", kTI59Rate, true, false), descriptor("TI-58", kTI58Rate, false, false),
                              descriptor("TI-58C", kTI58CRate, false, true)},
                             {});
    return c;
}

BuildResult TIMachineBuilder::build(const BuildRequest& request, const Catalogue& catalogue) {
    const MachineDescriptor* d = catalogue.model(request.model);
    if (!d || d->family != kTIFamily) return {nullptr, {}, "not a TI-58/59 model: \"" + request.model.value + "\""};
    const MachineVariant v = request.model.value == "TI-59"  ? MachineVariant::TI59
                             : request.model.value == "TI-58" ? MachineVariant::TI58
                                                              : MachineVariant::TI58C;
    std::vector<std::string_view> roms, constants;
    for (const auto& e : d->romsFor(std::nullopt))
        for (const auto& image : request.roms)
            if (image.fileName == e.fileName) {
                std::string_view text(reinterpret_cast<const char*>(image.bytes.data()), image.bytes.size());
                (e.unit == ContentUnit::Rows ? constants : roms).push_back(text);
            }
    std::string error;
    TIRoms decoded = decodeRomSet(v, roms, constants, error);
    if (!error.empty()) return {nullptr, {}, error};
    auto content = std::dynamic_pointer_cast<const TIConstantMemory>(request.content);
    return {std::make_unique<TIMachine>(*d, v, decoded, v == MachineVariant::TI58C ? content : nullptr), {}, {}};
}
