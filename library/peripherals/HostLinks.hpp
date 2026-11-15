#pragma once
#include "machine/Capability.hpp"
#include "machine/Machine.hpp"
#include "peripherals/HostLink.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The host links an app session holds (LIB-2 design §3.5; spec AC-18 to
// AC-20, EC-5, EC-7). Used on the session's thread only, between two
// advances. A product's rules say which serial port gets which serial port
// file and when.
//
// - A link is made by rule: at once when the port first exists
//   (openAtOnce), or else when the port is first used (D-5). Once made, it
//   lives as long as the HostLinks, so its file keeps its name for as long
//   as the product runs (AC-19).
// - When its port goes away (a model switch, a detach), the link stays,
//   unbound, so a program that has the file open is not cut off; it is bound
//   again to the next port the rules give the same file name (AC-20).
// - pump() moves every byte the machine sent to the link, and what the link
//   received into the machine as far as the port takes it; the line
//   settings are never read (AC-18; spec OQ-2: as today).
// - Where there are no pseudo-terminals the factory gives no link: pump()
//   then drains what the machine sends and drops it, and the machine runs
//   (EC-5).

namespace calcu::peripherals {

struct HostLinkRule {
    std::string fileName;   // "calcu1600-rs232c.serial"
    bool openAtOnce = false;
};
using HostLinkRules = std::function<std::optional<HostLinkRule>(const machine::CapabilityEntry&)>;
using HostLinkFactory =
    std::function<std::unique_ptr<HostLink>(const std::optional<std::string>& folder, const std::string& fileName)>;

struct HostLinkStatus {
    std::string fileName;
    std::string path;        // the serial port file; empty without a link
    bool open = false;       // a pseudo-terminal is behind it
    bool connected = false;  // bound to a serial port of the machine now
};

class HostLinks {
public:
    // folder: LIB-9's Paths::serialLinkFolder(). The factory defaults to
    // openHostLink (tests inject their own).
    HostLinks(HostLinkRules, std::optional<std::string> folder, HostLinkFactory = {});

    void machineChanged(machine::Machine&);   // after a build, attach, detach, replace: rebind by rule
    void machineGone();                       // no machine: every link unbound
    void pump(machine::Machine&);             // between two advances; rebinds first
    void moveTo(std::optional<std::string> folder);
    std::vector<HostLinkStatus> status() const;

private:
    struct Link {
        std::string fileName;
        std::unique_ptr<HostLink> link;   // null: none on this platform (EC-5)
        std::optional<machine::CapabilityId> bound;
        std::vector<std::uint8_t> pending;   // received, not yet taken by the port
    };
    Link* find(const std::string& fileName);

    HostLinkRules rules_;
    std::optional<std::string> folder_;
    HostLinkFactory factory_;
    std::vector<Link> links_;
};

} // namespace calcu::peripherals
