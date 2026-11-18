#include "harness/attach/AttachEndpoint.hpp"

#include <utility>

namespace calcu::harness {

namespace {

std::string trimmedLine(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
    std::size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t')) ++b;
    return s.substr(b);
}

} // namespace

AttachEndpoint::AttachEndpoint(AttachInbox& inbox, std::uint16_t port)
    : inbox_(inbox),
      stream_([this](StreamEvent e) { onStream(std::move(e)); }, port, std::string(kSecondHarness) + "\n") {}

AttachEndpoint::~AttachEndpoint() = default;

AttachEndpoint::Opened AttachEndpoint::open() {
    const LocalStream::Opened o = stream_.open();
    return Opened{o.ok, o.problem};
}

void AttachEndpoint::write(std::uint64_t client, std::vector<std::string> lines) {
    std::string data;
    for (auto& l : lines) data += l + "\n";
    if (!data.empty()) stream_.write(client, std::move(data));
}

void AttachEndpoint::close(std::uint64_t client) { stream_.close(client); }

void AttachEndpoint::onStream(StreamEvent e) {
    switch (e.kind) {
    case StreamEvent::Kind::Connected:
        in_.clear();
        inbox_.post(AttachEvent{AttachEvent::Kind::Attached, {}, e.client});
        break;
    case StreamEvent::Kind::Disconnected:   // gone without quit: as quit
        in_.clear();
        if (e.client != quitClient_) inbox_.post(AttachEvent{AttachEvent::Kind::Detached, {}, e.client});
        break;
    case StreamEvent::Kind::Bytes: {
        if (e.client == quitClient_) break;
        in_ += e.bytes;
        std::size_t nl;
        while ((nl = in_.find('\n')) != std::string::npos) {
            std::string line = trimmedLine(in_.substr(0, nl));
            in_.erase(0, nl + 1);
            if (line == "quit") {   // ends the attach, not the app
                quitClient_ = e.client;
                in_.clear();
                inbox_.post(AttachEvent{AttachEvent::Kind::Detached, {}, e.client});   // before the client sees the close
                stream_.close(e.client);
                break;
            }
            inbox_.post(AttachEvent{AttachEvent::Kind::Line, std::move(line), e.client});
        }
        break;
    }
    }
}

} // namespace calcu::harness
