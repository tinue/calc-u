#include "app/SharpControlRules.hpp"

#include <variant>

#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Value.hpp"

namespace shared {

namespace {

using calcu::machine::Machine;
namespace sessions = calcu::sessions;

std::string pluggedAt(const Machine& m, const char* place) {
    for (const auto& e : m.plugged().byPlace)
        if (e.place.steps.size() == 1 && e.place.steps[0].value == place) return e.peripheral.value;
    return {};
}

std::string hostDriveFolder(const Machine& m) {
    auto* f = calcu::machine::first<sharp::HostDriveFolder>(m);
    return f ? f->folder() : std::string();
}

calcu::control::ProductEntry entry(const char* name, sessions::Value v) {
    return calcu::control::ProductEntry{name, sessions::valuePayload(name, std::move(v))};
}

} // namespace

std::optional<std::string> SharpControlRules::unavailable(const calcu::control::EntryKey& key, const Machine&) const {
    if (const auto* p = std::get_if<calcu::control::PlaceKey>(&key))
        if (!m_in.canMountHostFolder && p->path.steps.size() == 1 && p->path.steps[0].value == sharp::kHostDrivePlace)
            return std::string(kNoHostFolderHere);
    return std::nullopt;
}

void SharpControlRules::productEntries(const Machine& m, std::vector<calcu::control::ProductEntry>& out) const {
    const bool pc1600 = m.model().value == sharp::kPC1600;
    const std::string printer = pluggedAt(m, sharp::kPrinter);
    const bool ce1600p = printer == sharp::kCE1600P;
    // The CE-1600F is in the CE-1600P's box: its controls stay on every
    // PC-1600's bar so the bar does not jump (today's floppyShown/Enabled).
    sessions::Value::Object disk{{"shown", sessions::Value(pc1600)}, {"available", sessions::Value(pc1600 && ce1600p)}};
    if (!(pc1600 && ce1600p)) disk.push_back({"reason", sessions::Value(kDiskInCe1600p)});
    out.push_back(entry(kDiskControlsEntry, sessions::Value(std::move(disk))));
    out.push_back(entry(kCe158PrinterPanelEntry,
                        sessions::Value::object({{"shown", pluggedAt(m, sharp::kInterface) == sharp::kCE158}})));
    // File ▸ Mount Directory… and Unmount (LIB-14 AC-9: the place's owner
    // adds its own items) follow these two.
    sessions::Value::Object mount{{"available", sessions::Value(pc1600 && m_in.canMountHostFolder)}};
    if (!pc1600) mount.push_back({"reason", sessions::Value(kHostDriveIsPc1600s)});
    else if (!m_in.canMountHostFolder) mount.push_back({"reason", sessions::Value(kNoHostFolderHere)});
    out.push_back(entry(kMountDirectoryEntry, sessions::Value(std::move(mount))));
    const std::string folder = hostDriveFolder(m);
    sessions::Value::Object hostDrive{{"folder", sessions::Value(folder)}, {"available", sessions::Value(!folder.empty())}};
    if (folder.empty()) hostDrive.push_back({"reason", sessions::Value(kNoFolderMounted)});
    out.push_back(entry(kHostDriveFolderEntry, sessions::Value(std::move(hostDrive))));
    if (pc1600 && !ce1600p)
        out.push_back(entry(kCe1600pNextVariantEntry,
                            sessions::Value::object({{"variant", m_in.ce1600pNextVariant ? m_in.ce1600pNextVariant()
                                                                                          : std::string("New")}})));
}

std::vector<const calcu::control::ControlRules*> sharpControlSetup(const SharpControlRules& rules) { return {&rules}; }

} // namespace shared
