#include "Machine/SharpCatalogue.hpp"

#include <algorithm>

#include "../Connector/MemoryCardCatalog.hpp"
#include "../Display/StatusLine.hpp"
#include "../FileIO.hpp"
#include "../PC1500/PC1500Clocks.hpp"
#include "../PC1600/PC1600Clocks.hpp"
#include "Machine/SharpKeyIds.hpp"

using namespace calcu::machine;

namespace sharp {

namespace {

// The place bus-rom takes any number of bus ROMs.
constexpr int kBusRomsAtOnce = 1000;

RomEntry rom(const char* file, std::uint64_t bytes, const char* md5) {
    return RomEntry{file, RomEncoding::Binary, bytes, ContentUnit::Bytes, md5};
}

// The fetch list of roms/fetch_roms.sh.
std::vector<RomEntry> pc1600Roms(bool old) {
    if (old)
        return {rom("PC1600-P0-B0-old.bin", 16384, "5afcc22134e106bfd63b899febe9df7c"),
                rom("PC1600-P1-B0-old.bin", 16384, "3bcb6b178f5967c7c8e32afe560a3e75"),
                rom("PC1600-P1-B3-old.bin", 16384, "ded92d8280f8f83ce3498fb9cbfb9b3d"),
                rom("PC1600-P1-B3B-old.bin", 16384, "2e8e075cac8f9696c5e833ceef130a4f"),
                rom("PC1600-P2-B6-old.bin", 16384, "2c977fdd8c924c1492a2c23f67a20f23"),
                rom("PC1600-LH5803-C000-FFFF-old.bin", 16384, "6005b6420bd5e191e81a1562f3242ec9")};
    return {rom("PC1600-P0-B0-new.bin", 16384, "404bf6f2df489e09649167078acd9a25"),
            rom("PC1600-P1-B0-new.bin", 16384, "bddbb8bbf0b2bd2d95038f67b8d002ac"),
            rom("PC1600-P1-B3-new.bin", 16384, "6f6e1a9d46db7dc91d4322c93583ac81"),
            rom("PC1600-P1-B3B-new.bin", 16384, "2483319acf35da4e848e59ab954abf46"),
            rom("PC1600-P2-B6-new.bin", 16384, "86cb9036da284de2b04c7946d140a9fd"),
            rom("PC1600-LH5803-C000-FFFF-new.bin", 16384, "56168830b46d637b08529a74609bee3f")};
}

VariantDescriptor pc1500Variant(const char* id, const char* md5) {
    return {VariantId{id}, {rom(("PC-1500_" + std::string(id) + ".ROM").c_str(), 16384, md5)}};
}

DisplayDescriptor display(bool pc1600) {
    DisplayDescriptor d;
    d.geometry = DotMatrixGeometry{156, pc1600 ? 32 : 7, std::nullopt};
    for (std::size_t i = 0; i < StatusLine::kCount; ++i) {
        const std::string name = kStatusSymbolNames[i];
        // S, ROMAJI, CTRL and BATT are the PC-1600's only.
        if (!pc1600 && (name == "S" || name == "ROMAJI" || name == "CTRL" || name == "BATT")) continue;
        d.indicators.push_back({name, false});
    }
    return d;
}

PinOutDescriptor contactLevel(const char* id, Takes takes) {
    // Every Sharp pin-out is contact level and a power cycle follows a
    // change on it.
    return {PinOutId{id}, PinOutLevel::ContactLevel, takes, AfterChange::PowerCycle};
}

MachineDescriptor pc1500Descriptor(bool pc1500a) {
    MachineDescriptor d;
    d.model = ModelId{pc1500a ? kPC1500A : kPC1500};
    d.family = ModelFamilyId{kSharpFamily};
    if (pc1500a) {
        d.variants = {pc1500Variant("A04", "8ebec8b0ef358645df14807c31df7d06")};
    } else {
        d.variants = {pc1500Variant("A01", "fbc55a9a8743e619b7709721ff5bcbff"),
                      pc1500Variant("A03", "4bcf78a6d3d32e2a0349eb2d28987b8d"),
                      pc1500Variant("A04", "8ebec8b0ef358645df14807c31df7d06")};
    }
    d.defaultVariant = VariantId{"A04"};
    d.tickRate = kPC1500CpuHz;   // one LH5801 cycle
    d.display = display(false);
    d.hasSound = true;
    for (const auto& k : pc1500KeyIds()) d.keys.push_back({KeyId{k}, {}});
    d.connectors = {{ConnectorId{"40-pin"}, "40-pin memory slot", {contactLevel(kPC1500SlotPinOut, Takes::One)}},
                    {ConnectorId{"60-pin"}, "60-pin system bus", {contactLevel(kPC1500BusPinOut, Takes::Chain), contactLevel(kPC1500PrinterPinOut, Takes::Chain), contactLevel(kPC1500InterfacePinOut, Takes::Chain)}}};
    d.places = {{PlaceName{kSlot1}, ConnectorId{"40-pin"}, PinOutId{kPC1500SlotPinOut}, 1},
                {PlaceName{kPrinter}, ConnectorId{"60-pin"}, PinOutId{kPC1500PrinterPinOut}, 1},
                {PlaceName{kInterface}, ConnectorId{"60-pin"}, PinOutId{kPC1500InterfacePinOut}, 1},
                {PlaceName{kBusRomPlace}, ConnectorId{"60-pin"}, PinOutId{kPC1500BusPinOut}, kBusRomsAtOnce}};
    // Depth as the machine is built.
    const ComponentId memory{"memory"};
    d.components = {{ComponentId{"LH5801"}, "LH5801 CPU", Depth::Separate, std::nullopt},
                    {ComponentId{kRtcComponent}, "uPD1990AC real-time clock", Depth::Separate, std::nullopt},
                    {ComponentId{"keyboard"}, "key matrix", Depth::Separate, std::nullopt},
                    {memory, "address decoding and RAM", Depth::Separate, std::nullopt},
                    {ComponentId{"LH5811"}, "LH5811 I/O port", Depth::Folded, memory},
                    {ComponentId{"lcd"}, "LCD and its RAM", Depth::Functional, std::nullopt},
                    {ComponentId{"buzzer"}, "piezo buzzer", Depth::Functional, std::nullopt}};
    // A battery-backed module is a medium in its slot, but the slot is no media
    // place of the descriptor: the module is a peripheral there.
    d.debugTargets = {DebugTargetId{"LH5801"}};
    return d;
}

MachineDescriptor pc1600Descriptor() {
    MachineDescriptor d;
    d.model = ModelId{kPC1600};
    d.family = ModelFamilyId{kSharpFamily};
    d.variants = {{VariantId{"New"}, pc1600Roms(false)}, {VariantId{"Old"}, pc1600Roms(true)}};
    d.defaultVariant = VariantId{"New"};
    d.tickRate = kPC1600TStateHz;   // one SC7852 T-state; the LH5803 counts in them too
    d.display = display(true);
    d.hasSound = true;
    for (const auto& k : pc1600KeyIds()) d.keys.push_back({KeyId{k}, {}});
    d.connectors = {{ConnectorId{"40-pin-1"}, "40-pin memory slot", {contactLevel(kPC1600Slot1PinOut, Takes::One)}},
                    {ConnectorId{"40-pin-2"}, "40-pin memory slot", {contactLevel(kPC1600Slot2PinOut, Takes::One)}},
                    {ConnectorId{"60-pin"}, "60-pin system bus", {contactLevel(kPC1600BusPinOut, Takes::Chain), contactLevel(kPC1600PrinterPinOut, Takes::Chain), contactLevel(kPC1600InterfacePinOut, Takes::Chain), contactLevel(kPC1600HostDrivePinOut, Takes::Chain)}},
                    {ConnectorId{"rs232c"}, "RS-232C", {}}};
    d.places = {{PlaceName{kSlot1}, ConnectorId{"40-pin-1"}, PinOutId{kPC1600Slot1PinOut}, 1},
                {PlaceName{kSlot2}, ConnectorId{"40-pin-2"}, PinOutId{kPC1600Slot2PinOut}, 1},
                {PlaceName{kPrinter}, ConnectorId{"60-pin"}, PinOutId{kPC1600PrinterPinOut}, 1},
                {PlaceName{kInterface}, ConnectorId{"60-pin"}, PinOutId{kPC1600InterfacePinOut}, 1},
                {PlaceName{kHostDrivePlace}, ConnectorId{"60-pin"}, PinOutId{kPC1600HostDrivePinOut}, 1},
                {PlaceName{kBusRomPlace}, ConnectorId{"60-pin"}, PinOutId{kPC1600BusPinOut}, kBusRomsAtOnce}};
    const ComponentId memory{"memory"};
    const ComponentId subCpu{"sub-cpu"};
    d.components = {{ComponentId{"SC7852"}, "SC7852 (Z-80) CPU", Depth::Separate, std::nullopt},
                    {ComponentId{"LH5803"}, "LH5803 CPU", Depth::Separate, std::nullopt},
                    {ComponentId{"bus-arbiter"}, "bus arbiter", Depth::Separate, std::nullopt},
                    {memory, "memory and bank decoding", Depth::Separate, std::nullopt},
                    {ComponentId{"SC7852-blocks"}, "SC7852 on-chip blocks", Depth::Folded, memory},
                    {ComponentId{"LR38041"}, "LR38041 gate array", Depth::Folded, memory},
                    {subCpu, "sub-CPU", Depth::Functional, std::nullopt},
                    {ComponentId{kRtcComponent}, "LU-57813P real-time clock", Depth::Folded, subCpu},
                    {ComponentId{kRs232cComponent}, "TC8576F UART", Depth::Separate, std::nullopt},
                    {ComponentId{"keyboard"}, "key matrix", Depth::Separate, std::nullopt},
                    {ComponentId{"lcd"}, "HD61102 LCD controllers", Depth::Separate, std::nullopt},
                    {ComponentId{"buzzer"}, "piezo buzzer", Depth::Functional, std::nullopt}};
    d.debugTargets = {DebugTargetId{"SC7852"}, DebugTargetId{"LH5803"}};
    return d;
}

std::vector<PeripheralDescriptor> peripherals() {
    PeripheralDescriptor ce150;
    ce150.id = PeripheralId{kCE150};
    ce150.name = "CE-150";
    ce150.roms = {rom("CE-150.ROM", 8192, "eb9aa5156c6849890b137799efc50a4b")};
    ce150.fits = {PinOutId{kPC1500PrinterPinOut}, PinOutId{kPC1600PrinterPinOut}};
    ce150.mediaPlaces = {{MediaPlaceId{"cassette-recorder"}, "cassette", MediaPassage::Holding, {}}};
    ce150.components = {{ComponentId{"plotter"}, "ALPS plotter mechanism and its paper", Depth::Functional, std::nullopt}};

    PeripheralDescriptor ce158;
    ce158.id = PeripheralId{kCE158};
    ce158.name = "CE-158";
    ce158.roms = {rom("CE-158.ROM", 16384, "aa952878fb29da4844791d95185649ca")};
    ce158.fits = {PinOutId{kPC1500InterfacePinOut}, PinOutId{kPC1600InterfacePinOut}};
    ce158.components = {{ComponentId{"uart"}, "CE-158 UART", Depth::Separate, std::nullopt},
                        {ComponentId{"printer-port"}, "Centronics printer port", Depth::Functional, std::nullopt}};

    PeripheralDescriptor ce1600p;
    ce1600p.id = PeripheralId{kCE1600P};
    ce1600p.name = "CE-1600P";
    ce1600p.variants = {{VariantId{"New"},
                         {rom("PC1600-P1-B4-CE1600P-new.bin", 16384, "05548a8dda3e572d50d4bd281a650ea8"),
                          rom("PC1600-P1-B5-CE1600P-OR-F-new.bin", 16384, "a675c6dbdf7dc4c10e8d96891e196f8f")}},
                        {VariantId{"Old"},
                         {rom("PC1600-P1-B4-CE1600P-old.bin", 16384, "df41b050acbc29c83214cbaaf29bee91"),
                          rom("PC1600-P1-B5-CE1600P-OR-F-old.bin", 16384, "33f3ef7207eac06587cc4c6d70c6cbd0")}}};
    ce1600p.defaultVariant = VariantId{"New"};
    ce1600p.fits = {PinOutId{kPC1600PrinterPinOut}};
    // The CE-1600F is described with it.
    ce1600p.mediaPlaces = {{MediaPlaceId{"cassette-recorder"}, "cassette", MediaPassage::Holding, {}},
                           {MediaPlaceId{"floppy"}, "floppy-disk", MediaPassage::Holding, {0, 1}}};
    const ComponentId plotter{"plotter"};
    ce1600p.components = {{plotter, "ALPS plotter mechanism and its paper", Depth::Functional, std::nullopt},
                          {ComponentId{"CE-1600F"}, "CE-1600F floppy drive", Depth::Functional, std::nullopt}};

    PeripheralDescriptor hostDrive;
    hostDrive.id = PeripheralId{kHostDrive};
    hostDrive.name = "host drive";
    // Calc-U-1600's own ROM (firmware/pc1600-hostdrive/), always bundled.
    hostDrive.roms = {rom("PC1600-P1-B7-HOSTDRIVE.bin", 16384, "2aa388667706bf9bf5a707dc7b36416d")};
    hostDrive.fits = {PinOutId{kPC1600HostDrivePinOut}};

    // No ROM entry of its own; its bytes come with the
    // request (BusRomRequest), so only a preset can plug one in.
    PeripheralDescriptor busRom;
    busRom.id = PeripheralId{kBusRom};
    busRom.name = "bus ROM";
    busRom.fits = {PinOutId{kPC1500BusPinOut}, PinOutId{kPC1600BusPinOut}};

    return {ce150, ce158, ce1600p, hostDrive, busRom};
}

} // namespace

PeripheralDescriptor moduleDescriptor(const MemoryCardDefinition& def) {
    PeripheralDescriptor d;
    d.id = PeripheralId{def.moduleName};
    d.name = def.moduleName;
    auto fit = [&d](const char* pinOut) {
        if (std::none_of(d.fits.begin(), d.fits.end(), [&](const PinOutId& p) { return p.value == pinOut; }))
            d.fits.push_back(PinOutId{pinOut});
    };
    for (CardHost h : def.compatibleHosts) {
        switch (h) {
            case CardHost::PC1500:
            case CardHost::PC1500A: fit(kPC1500SlotPinOut); break;
            case CardHost::PC1600Slot1: fit(kPC1600Slot1PinOut); break;
            case CardHost::PC1600Slot2: fit(kPC1600Slot2PinOut); break;
        }
    }
    bool writeProtect = false;
    for (const Region& r : def.regions)
        for (std::uint32_t b = 0; b < r.banking.bankCount; ++b) writeProtect = writeProtect || r.contentForBank(b).hasWriteProtect;
    if (writeProtect) d.controls = {{ControlId{"write-protect"}, ControlForm::Switch}};
    d.productData = std::make_shared<ModuleDefinitionData>(def);
    return d;
}

int addModuleDefinitions(Catalogue& catalogue, const std::string& dir) {
    int added = 0;
    std::string error;
    for (const MemoryCardCatalogEntry& e : scanMemoryCardDirectory(dir, &error)) {
        if (catalogue.peripheral(PeripheralId{e.moduleName})) continue;
        std::vector<std::uint8_t> bytes;
        MemoryCardDefinition def;
        std::string parseError;
        if (!readWholeFile(e.filePath, &bytes) ||
            !parseMemoryCardDefinition(std::string(bytes.begin(), bytes.end()), &def, &parseError))
            continue;
        catalogue.addPeripheral(moduleDescriptor(def));
        ++added;
    }
    return added;
}

const Catalogue& sharpCatalogue() {
    static const Catalogue c({pc1500Descriptor(false), pc1500Descriptor(true), pc1600Descriptor()}, peripherals());
    return c;
}

} // namespace sharp
