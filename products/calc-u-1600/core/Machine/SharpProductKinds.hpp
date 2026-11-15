#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../Connector/AlpsPlotterMechanism.hpp"
#include "../Debug/Inspect/Inspector.hpp"
#include "../Tape/TapeDeck.hpp"
#include "machine/Capability.hpp"

// Calc-U-1600's capability kinds (C1600-1 design §3.6, §5; capabilities.md,
// product kinds). The app layer reaches what only a Sharp machine offers
// through these, never through a machine class (C1600-1 AC-19, AC-20).
//
// Product kinds, which stay:
//   c1600.loader-memory        memory for the BASIC and machine-code loaders (owner: the main CPU)
//   c1600.inspector            the INSPECTOR views (owner: the main CPU)
//   c1600.ce158-printer-port   bytes sent to the CE-158's printer port (owner: the CE-158)
//   c1600.host-drive-folder    the host drive's folder (owner: the host drive)
// Adapter kinds, each removed by the step named (design §5):
//   c1600.key-queue      the PC-1500's paced keystroke queue (M-36, LIB-5)
//   c1600.plotter-paper  CE-150 / CE-1600P paper (M-45, LIB-8)
//   c1600.trace          the CPU trace file (M-45, LIB-10)
//   c1600.tape-deck      the cassette recorder (M-47, LIB-7)
//   c1600.floppy-drive   the CE-1600F (M-47, LIB-7)
//
// The owner "the main CPU" (LH5801, SC7852) stands for the machine itself
// (design D-1).

namespace sharp {

class LoaderMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.loader-memory"};
    calcu::machine::KindId kind() const override { return kKind; }

    // Through the banks selected now (PC-1500 ME0; PC-1600 the Z-80 view).
    virtual std::uint8_t peek(std::uint16_t address) const = 0;
    // All or nothing; false if a byte would not land in RAM.
    virtual bool poke(std::uint16_t address, const std::uint8_t* data, std::size_t size) = 0;
    // The PC-1600's fixed 16 KB internal RAM; empty on the PC-1500.
    virtual std::vector<std::uint8_t> internalRam() const = 0;
};

class InspectorViews : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.inspector"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::vector<std::string> view(inspect::View) const = 0;
    virtual std::vector<inspect::MenuEntry> menu(bool dumps) const = 0;
    // The module in `slot` (from 1) as one image, and its bank count (-1:
    // none or unbanked).
    virtual std::vector<std::uint8_t> slotImage(int slot, int* bankCount) const = 0;
};

class Ce158PrinterPort : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.ce158-printer-port"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::vector<std::uint8_t> drain() = 0;   // the bytes since the last call
};

class HostDriveFolder : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.host-drive-folder"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::string folder() const = 0;            // UTF-8
    virtual void setFolder(const std::string& folder) = 0;   // a live media swap
};

// ---- Adapter kinds ----

class KeyQueue : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.key-queue"};
    calcu::machine::KindId kind() const override { return kKind; }

    // A tap at the ROM's own key-scan cadence (PC1500Machine::enqueueKey);
    // false for an ID that is not a key of the matrix.
    virtual bool enqueue(const std::string& keyId) = 0;
};

class PlotterPaper : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.plotter-paper"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::vector<AlpsPlotterMechanism::FlatPoint> points() const = 0;
    virtual std::uint64_t revision() const = 0;
    virtual std::vector<std::string> drainEvents() = 0;
    virtual void clear() = 0;
};

class CpuTrace : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.trace"};
    calcu::machine::KindId kind() const override { return kKind; }

    // Takes the file on success (see PC1500Machine::beginCpuTrace).
    virtual bool begin(std::FILE* handle, std::uint32_t flags) = 0;
    virtual void end() = 0;
    virtual bool active() const = 0;
    virtual std::uint64_t bytes() const = 0;
};

class TapeDeckAccess : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.tape-deck"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual bool play(const std::string& path, std::string& error) = 0;
    virtual void record(const std::string& path) = 0;
    virtual bool eject(std::string* error) = 0;
    virtual TapeDeck::Status status() const = 0;
    virtual std::string takeLastError() = 0;
};

class FloppyDriveAccess : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.floppy-drive"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual std::vector<std::uint8_t> diskImage() const = 0;
    virtual std::uint64_t revision() const = 0;
    virtual void eject() = 0;
    virtual bool hasDisk() const = 0;
    virtual bool loadImage(const std::uint8_t* data, std::size_t size) = 0;
    virtual int side() const = 0;
    virtual void setSide(int side) = 0;
    virtual bool motorOn() const = 0;
};

// The first capability of kind K the machine `m` has now, or nullptr.
template <class K, class M> K* firstOf(M& m) {
    for (const auto& e : m.capabilities(K::kKind))
        if (K* k = calcu::machine::capabilityAs<K>(m, e.id)) return k;
    return nullptr;
}

} // namespace sharp
