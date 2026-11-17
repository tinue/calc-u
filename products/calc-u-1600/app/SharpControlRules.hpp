#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "control/Compute.hpp"
#include "control/ControlRules.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"

// Calc-U-1600's side of the control state (LIB-4 Part D, D.4; spec AC-7,
// AC-17, AC-18): the one rule of its own and its product entries. The rest
// of today's ControlState is the library's generic entries, from the
// descriptors (one pin-out per 60-pin place, D-11).

namespace shared {

// The product entries (AC-17), each a value payload of the same kind.
inline constexpr const char* kDiskControlsEntry = "c1600.disk-controls";              // {shown, available, reason}
inline constexpr const char* kCe158PrinterPanelEntry = "c1600.ce158-printer-panel";   // {shown}
inline constexpr const char* kHostDriveFolderEntry = "c1600.host-drive-folder";       // {folder ("" none), available, reason}: Unmount
inline constexpr const char* kMountDirectoryEntry = "c1600.mount-directory";          // {available, reason}: Mount Directory…
inline constexpr const char* kCe1600pNextVariantEntry = "c1600.ce1600p-next-variant"; // {variant}, while unplugged

inline constexpr const char* kNoHostFolderHere = "A host folder cannot be mounted here.";
inline constexpr const char* kDiskInCe1600p = "The disk drive is in the CE-1600P.";
inline constexpr const char* kHostDriveIsPc1600s = "The host drive is the PC-1600's.";
inline constexpr const char* kNoFolderMounted = "No folder is mounted.";

class SharpControlRules final : public calcu::control::ControlRules {
public:
    struct Inputs {
        bool canMountHostFolder = true;                   // AC-18 (D-12): Qt6 and the harness yes, the Web no
        std::function<std::string()> ce1600pNextVariant;  // "New" or "Old" (LIB-18 AC-31)
    };
    explicit SharpControlRules(Inputs in) : m_in(std::move(in)) {}

    std::optional<std::string> unavailable(const calcu::control::EntryKey&, const calcu::machine::Machine&) const override;
    void productEntries(const calcu::machine::Machine&, std::vector<calcu::control::ProductEntry>&) const override;

private:
    Inputs m_in;
};

// The product's rules of the library's ControlPart for Calc-U-1600 (D-13);
// its catalogue and printers are LibraryParts' (the papers' printers).
std::vector<const calcu::control::ControlRules*> sharpControlSetup(const SharpControlRules&);

} // namespace shared
