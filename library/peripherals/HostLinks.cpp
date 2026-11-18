#include "peripherals/HostLinks.hpp"
#include "peripherals/SerialPort.hpp"
#include <algorithm>

namespace calcu::peripherals {

HostLinks::HostLinks(HostLinkRules rules, std::optional<std::string> folder, HostLinkFactory factory)
    : rules_(std::move(rules)), folder_(std::move(folder)), factory_(std::move(factory)) {
    if (!factory_) factory_ = [](const std::optional<std::string>& f, const std::string& n) { return openHostLink(f, n); };
}

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
                links_.push_back(Link{rule->fileName, factory_(folder_, rule->fileName), std::nullopt, {}});
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
    }
}

void HostLinks::moveTo(std::optional<std::string> folder) {
    folder_ = std::move(folder);
    for (auto& l : links_)
        if (l.link) l.link->moveTo(folder_);
}

std::vector<HostLinkStatus> HostLinks::status() const {
    std::vector<HostLinkStatus> out;
    for (const auto& l : links_)
        out.push_back(HostLinkStatus{l.fileName, l.link ? l.link->path() : std::string(), l.link != nullptr,
                                     l.bound.has_value()});
    return out;
}

} // namespace calcu::peripherals
