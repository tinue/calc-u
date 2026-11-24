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

// The host links an app session holds. Used on the session's thread only, between two
// advances. A product's rules say which serial port gets which serial port
// file and when.
//
// - A link is made by rule: at once when the port first exists
//   (openAtOnce), or else when the port is first used. Once made, it
//   lives as long as the HostLinks, so its file keeps its name for as long
//   as the product runs.
// - When its port goes away (a model switch, a detach), the link stays,
//   unbound, so a program that has the file open is not cut off; it is bound
//   again to the next port the rules give the same file name.
// - pump() moves every byte the machine sent to the link, and what the link
//   received into the machine as far as the port takes it; the line
//   settings are never read.
// - Where there are no pseudo-terminals the factory gives no link: pump()
//   then drains what the machine sends and drops it, and the machine runs
//   anyway.

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
    // folder: Paths::serialLinkFolder(). The factory defaults to
    // openHostLink (tests inject their own).
    HostLinks(HostLinkRules, std::optional<std::string> folder, HostLinkFactory = {});

    void machineChanged(machine::Machine&);   // after a build, attach, detach, replace: rebind by rule
    void machineGone();                       // no machine: every link unbound
    void pump(machine::Machine&);             // between two advances; rebinds first
    void moveTo(std::optional<std::string> folder);
    std::vector<HostLinkStatus> status() const;
    // The problems found since the last call, one sentence each: a link
    // that could not be opened, a link whose problem() changed. For the
    // owner's app log.
    std::vector<std::string> takeProblems();

private:
    struct Link {
        std::string fileName;
        std::unique_ptr<HostLink> link;   // null: none on this platform
        std::optional<machine::CapabilityId> bound;
        std::vector<std::uint8_t> pending;   // received, not yet taken by the port
        std::string reported;                // the link's problem() last reported
    };
    Link* find(const std::string& fileName);

    HostLinkRules rules_;
    std::optional<std::string> folder_;
    HostLinkFactory factory_;
    std::vector<Link> links_;
    bool ownFactory_ = false;   // openHostLink, which says why it gives no link
    std::vector<std::string> problems_;
    void noteProblems();
};

} // namespace calcu::peripherals
