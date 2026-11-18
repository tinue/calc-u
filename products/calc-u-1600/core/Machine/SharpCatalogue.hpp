#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../Connector/MemoryCardDefinition.hpp"
#include "machine/Catalogue.hpp"
#include "machine/ProductData.hpp"

// Calc-U-1600's catalogue: the PC-1500, PC-1500A and PC-1600, and the CE-150, CE-158,
// CE-1600P (with the CE-1600F) and the host drive. The memory modules are
// module definitions, read from files: whoever holds the folder adds them
// to a copy with addModuleDefinitions().

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
// A ROM on the 60-pin connector that only a preset plugs in: its bytes
// come with the request (BusRomRequest); it has no ROM entry of its own.
inline constexpr const char* kBusRom = "bus-rom";

// Places.
inline constexpr const char* kSlot1 = "slot-1";
inline constexpr const char* kSlot2 = "slot-2";
inline constexpr const char* kPrinter = "printer";
inline constexpr const char* kInterface = "interface";
inline constexpr const char* kHostDrivePlace = "host-drive";
inline constexpr const char* kBusRomPlace = "bus-rom";   // any number, in plug order

// Pin-outs. Both PC-1500 models share one memory slot pin-out; the module definition's own host list is checked at the attach
// as well (SharpPlaces).
inline constexpr const char* kPC1500SlotPinOut = "PC-1500 memory slot";
inline constexpr const char* kPC1600Slot1PinOut = "PC-1600 Slot 1";
inline constexpr const char* kPC1600Slot2PinOut = "PC-1600 Slot 2";
inline constexpr const char* kPC1500BusPinOut = "PC-1500 60-pin";
inline constexpr const char* kPC1600BusPinOut = "PC-1600 60-pin";
// One pin-out per 60-pin place: names of places on
// the one connector, so that each place lists exactly what fits it; each a
// chain (Takes::One would keep the other places of the connector empty),
// one peripheral per place by takesAtOnce. The bus
// pin-outs above stay for the place bus-rom (a chain).
inline constexpr const char* kPC1500PrinterPinOut = "pc1500-60-pin-printer";
inline constexpr const char* kPC1500InterfacePinOut = "pc1500-60-pin-interface";
inline constexpr const char* kPC1600PrinterPinOut = "pc1600-60-pin-printer";
inline constexpr const char* kPC1600InterfacePinOut = "pc1600-60-pin-interface";
inline constexpr const char* kPC1600HostDrivePinOut = "pc1600-60-pin-host-drive";

// Component IDs.
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
// `compatible-hosts` give, the write-protect switch as a control, the
// definition as ProductData.
calcu::machine::PeripheralDescriptor moduleDescriptor(const MemoryCardDefinition&);

// Adds every module definition (`*.module.yaml`) of `dir` that is not in the
// catalogue yet; returns how many it added. A file that does not parse is
// skipped (MemoryCardCatalog's rule).
int addModuleDefinitions(calcu::machine::Catalogue&, const std::string& dir);

} // namespace sharp
