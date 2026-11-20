#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../Debug/Inspect/Inspector.hpp"
#include "../Tape/TapeDeck.hpp"
#include "machine/Capability.hpp"

// Calc-U-1600's capability kinds (capabilities.md, product kinds). The app layer reaches what only a Sharp machine offers
// through these, never through a machine class.
//
// Product kinds, which stay:
//   c1600.loader-memory        memory for the BASIC and machine-code loaders (owner: the main CPU)
//   c1600.inspector            the INSPECTOR views (owner: the main CPU)
//   c1600.ce158-printer-port   bytes sent to the CE-158's printer port (owner: the CE-158)
//   c1600.host-drive-folder    the host drive's folder (owner: the host drive)
//   c1600.trace-records        the records of a CPU trace (owner: the main CPU)
//   c1600.routine-call         a routine called from the current state (owner: the main CPU)
//   c1600.tape-deck            the cassette's counter and motor (owner: the CE-150 on a PC-1500/1500A, or the CE-1600P)
//   c1600.floppy-drive         the CE-1600F's lamp (owner: the CE-1600P)
// The media themselves are the library's media kinds (SharpMediaPlaces.hpp).
//
// The owner "the main CPU" (LH5801, SC7852) stands for the machine itself.

namespace sharp {

// The loaders' memory access: everything the
// BASIC and machine-code loaders read and write, and nothing more. The
// loaders (core/Loaders.hpp) name no machine class; they reach memory only
// through this kind. The implementations over the core are
// core/Machine/SharpLoaderMemory.hpp.
class LoaderMemory : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.loader-memory"};
    calcu::machine::KindId kind() const override { return kKind; }

    enum class Processor { Main, Lh5803 };   // Main: the LH5801 (PC-1500) or the Z-80 (PC-1600)
    enum class Mode { None, Mode0, Mode1 };  // None on the PC-1500 and PC-1500A
    // All or nothing: `stored` false means nothing was written, and
    // `firstMiss` is the first address of the range that is not RAM.
    struct Write {
        bool stored = false;
        std::uint32_t firstMiss = 0;
    };
    struct Module {
        bool present = false;
        std::uint32_t size = 0;   // the module's whole image, every bank
        int banks = 0;            // its banks; 0: unbanked
    };

    // Reads as the processor sees memory now (the banks selected now); no
    // effect on the machine. On the PC-1500 both processors read ME0.
    virtual std::uint8_t peek(std::uint16_t address, Processor = Processor::Main) const = 0;
    virtual Mode mode() const = 0;
    // The selected program area: PC-1600 0 S0, 1 S1, 2 S2 (TITLE); PC-1500 0.
    virtual int programArea() const = 0;
    // The built-in RAM: on the PC-1500 its ME0 (the RAM of a module there
    // too); on the PC-1600 the internal RAM at &C000-&FFFF.
    virtual Write writeBuiltIn(std::uint16_t address, const std::uint8_t* data, std::size_t size) = 0;
    // PC-1600: a module's RAM in global bank 0-3 behind &8000-&BFFF,
    // whichever bank is switched in. Not stored on the PC-1500.
    virtual Write writeSlot(int bank, std::uint16_t address, const std::uint8_t* data, std::size_t size) = 0;
    // Whether writeSlot would store [address, address + size).
    virtual bool slotRam(int bank, std::uint16_t address, std::size_t size) const = 0;
    // PC-1600: what a module answers at &8000-&BFFF in global bank 0-3, as
    // writeSlot reaches it, without side effects; 0xFF where nothing answers.
    // 0xFF on the PC-1500.
    virtual std::uint8_t peekSlot(int bank, std::uint16_t address) const = 0;
    virtual Module module(int slot) const = 0;   // slot 1 or 2
    // The PC-1600's fixed 16 KB internal RAM; empty on the PC-1500.
    virtual std::vector<std::uint8_t> internalRam() const = 0;

    bool pc1600() const { return mode() != Mode::None; }
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

// The records of a CPU trace in Calc-U-1600's trace format, version 3: the header and SESSION_START at begin(), the
// instruction frames of every CPU and the gap records as the core drains
// its rings, SESSION_END at endRecord(). The library's trace session writes
// them into the file, at most a budget at a time.
class TraceRecords : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.trace-records"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual bool begin() = 0;   // false: a trace runs already
    // Whole records waiting, at most `budget` bytes, appended to `out`;
    // false: more were waiting than the budget took.
    virtual bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) = 0;
    virtual void endRecord() = 0;   // drains what is left, SESSION_END
    virtual void end() = 0;         // recording off
};

// A machine-code routine called from the current state: the processor of a debug target makes the call as its own
// call instruction does (the LH5801's and LH5803's SJP, the Z-80's CALL):
// the return address pushed, the program counter at `address`. The harness
// then runs the debugger's step-out until the routine has returned. Owner: the
// main CPU, for every processor of the model.
class RoutineCall : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.routine-call"};
    calcu::machine::KindId kind() const override { return kKind; }

    // False: the model has no processor of that debug target name.
    virtual bool call(const std::string& target, std::uint16_t address) = 0;
};

// The tape and the disk themselves are the media
// places `cassette` and `floppy` (core/Machine/SharpMediaPlaces.hpp); these
// two product kinds keep what the control bar shows: the tape counter, its
// motor and whether it plays or records, and the drive's lamp.
class TapeDeckAccess : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.tape-deck"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual TapeDeck::Status status() const = 0;
};

class FloppyDriveAccess : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c1600.floppy-drive"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual bool motorOn() const = 0;
};

} // namespace sharp
