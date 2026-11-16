#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../Connector/MemoryCardDefinition.hpp"
#include "machine/Catalogue.hpp"
#include "machine/ProductData.hpp"

// Calc-U-1600's catalogue (LIB-1 AC-34 to AC-39, AC-41; C1600-1 design
// §3.1): the PC-1500, PC-1500A and PC-1600, and the CE-150, CE-158,
// CE-1600P (with the CE-1600F) and the host drive. The memory modules are
// module definitions, read from files: whoever holds the folder adds them
// to a copy with addModuleDefinitions() (LIB-1 AC-3).

namespace sharp {

inline constexpr const char* kSharpFamily = "Sharp PC-1500/1600";

// Model IDs.
inline constexpr const char* kPC1500 = "PC-1500";
inline constexpr const char* kPC1500A = "PC-1500A";
inline constexpr const char* kPC1600 = "PC-1600";

// Peripheral IDs (the modules: their module name, "CE-161", ...).
inline constexpr const char* kCE150 = "CE-150";
inline constexpr const char* kCE158 = "CE-158";
inline constexpr const char* kCE1600P = "CE-1600P";
inline constexpr const char* kHostDrive = "host-drive";
// A ROM on the 60-pin connector that only a preset plugs in (C1600-4 AC-10,
// OQ-2): its bytes come with the request (BusRomRequest), D-5.
inline constexpr const char* kBusRom = "bus-rom";

// Places (C1600-1 §3.4).
inline constexpr const char* kSlot1 = "slot-1";
inline constexpr const char* kSlot2 = "slot-2";
inline constexpr const char* kPrinter = "printer";
inline constexpr const char* kInterface = "interface";
inline constexpr const char* kHostDrivePlace = "host-drive";
inline constexpr const char* kBusRomPlace = "bus-rom";   // any number, in plug order (C1600-4 D-6)

// Pin-outs. Both PC-1500 models share one memory slot pin-out (LIB-1
// AC-36); the module definition's own host list is checked at the attach
// as well (SharpPlaces).
inline constexpr const char* kPC1500SlotPinOut = "PC-1500 memory slot";
inline constexpr const char* kPC1600Slot1PinOut = "PC-1600 Slot 1";
inline constexpr const char* kPC1600Slot2PinOut = "PC-1600 Slot 2";
inline constexpr const char* kPC1500BusPinOut = "PC-1500 60-pin";
inline constexpr const char* kPC1600BusPinOut = "PC-1600 60-pin";

// Component IDs (design D-10).
inline constexpr const char* kRtcComponent = "rtc";
inline constexpr const char* kRs232cComponent = "rs232c";

// A parsed module definition, the ProductData of a module's descriptor.
class ModuleDefinitionData final : public calcu::machine::ProductData {
public:
    explicit ModuleDefinitionData(MemoryCardDefinition d) : definition(std::move(d)) {}
    MemoryCardDefinition definition;
};

const calcu::machine::Catalogue& sharpCatalogue();

// The peripheral descriptor of a module definition: the pin-outs its
// `compatible-hosts` give (AC-41), the write-protect switch as a control
// (AC-39), the definition as ProductData.
calcu::machine::PeripheralDescriptor moduleDescriptor(const MemoryCardDefinition&);

// Adds every module definition (`*.card.yaml`) of `dir` that is not in the
// catalogue yet; returns how many it added. A file that does not parse is
// skipped (MemoryCardCatalog's rule).
int addModuleDefinitions(calcu::machine::Catalogue&, const std::string& dir);

} // namespace sharp
