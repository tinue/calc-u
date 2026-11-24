#include "peripherals/HostLinks.hpp"
#include "peripherals/SerialPort.hpp"
#include <algorithm>
#include <utility>

namespace calcu::peripherals {

HostLinks::HostLinks(HostLinkRules rules, std::optional<std::string> folder, HostLinkFactory factory)
    : rules_(std::move(rules)), folder_(std::move(folder)), factory_(std::move(factory)) {}

HostLinks::Link* HostLinks::find(const std::string& fileName) {
    for (auto& l : links_)
        if (l.fileName == fileName) return &l;
    return nullptr;
}

void HostLinks::machineGone() {
    for (auto& l : links_) {
        l.bound.reset();
        l.pending.clear();
    }
}

void HostLinks::machineChanged(machine::Machine& m) {
    std::vector<std::string> boundNow;
    if (rules_) {
        for (const auto& e : m.capabilities(SerialPort::kKind)) {
            const auto rule = rules_(e);
            if (!rule) continue;
            if (std::find(boundNow.begin(), boundNow.end(), rule->fileName) != boundNow.end()) continue;   // first wins
            Link* l = find(rule->fileName);
            if (!l) {
                auto* port = machine::capabilityAs<SerialPort>(m, e.id);
                if (!port || !(rule->openAtOnce || port->used())) continue;
                std::string why;
                std::unique_ptr<HostLink> made = factory_(folder_, rule->fileName, &why);
                if (!made && !why.empty())
                    problems_.push_back("The serial port file " + rule->fileName + " has no host link: " + why + ".");
                links_.push_back(Link{rule->fileName, std::move(made), std::nullopt, {}, {}});
                l = &links_.back();
            }
            if (l->bound != e.id) l->pending.clear();
            l->bound = e.id;
            boundNow.push_back(rule->fileName);
        }
    }
    for (auto& l : links_)
        if (std::find(boundNow.begin(), boundNow.end(), l.fileName) == boundNow.end()) {
            l.bound.reset();   // the port went away; the file stays
            l.pending.clear();
        }
}

void HostLinks::pump(machine::Machine& m) {
    machineChanged(m);
    std::uint8_t buf[1024];
    for (auto& l : links_) {
        if (!l.bound) continue;
        auto* port = machine::capabilityAs<SerialPort>(m, *l.bound);
        if (!port) continue;
        // Machine → link: everything sent; dropped without a link.
        for (;;) {
            const std::size_t n = port->takeSent(buf);
            if (n == 0) break;
            if (l.link) l.link->write(std::span<const std::uint8_t>(buf, n));
        }
        // Link → machine: up to the port's limit, the rest waits here.
        if (l.link) {
            while (l.pending.size() < SerialPortQueues::kReceiveLimit) {
                const std::size_t want = std::min(sizeof buf, SerialPortQueues::kReceiveLimit - l.pending.size());
                const std::size_t n = l.link->read(std::span<std::uint8_t>(buf, want));
                if (n == 0) break;
                l.pending.insert(l.pending.end(), buf, buf + n);
            }
            const std::size_t taken = port->putReceived(l.pending);
            l.pending.erase(l.pending.begin(), l.pending.begin() + static_cast<std::ptrdiff_t>(taken));
        }
        port->setPeerLines(PeerLines{true, true, l.link && l.link->peerOpen(), false});
        port->setPeerSending(!l.pending.empty());
    }
    noteProblems();
}

void HostLinks::noteProblems() {
    for (auto& l : links_) {
        if (!l.link) continue;
        std::string now = l.link->problem();
        if (now == l.reported) continue;
        l.reported = std::move(now);
        if (!l.reported.empty()) problems_.push_back("The serial port file " + l.fileName + ": " + l.reported + ".");
    }
}

std::vector<std::string> HostLinks::takeProblems() {
    noteProblems();
    return std::exchange(problems_, {});
}

void HostLinks::moveTo(std::optional<std::string> folder) {
    folder_ = std::move(folder);
    for (auto& l : links_)
        if (l.link) l.link->moveTo(folder_);
    noteProblems();
}

std::vector<HostLinkStatus> HostLinks::status() const {
    std::vector<HostLinkStatus> out;
    for (const auto& l : links_)
        out.push_back(HostLinkStatus{l.fileName, l.link ? l.link->path() : std::string(), l.link != nullptr,
                                     l.bound.has_value()});
    return out;
}

} // namespace calcu::peripherals
