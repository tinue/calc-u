#pragma once
#include <optional>
#include <string>
#include <string_view>

// The DAP's base protocol (C1600-3 design §3.1): each message is a
// `Content-Length: n` header, an empty line, and n bytes of JSON. A reader
// takes the bytes as they arrive, in any pieces, and gives whole message
// bodies; a header without a length is dropped, as today
// (frontends/qt6/app/debug/DapServer.cpp of 0.8.1).

namespace cu1600::dap {

class DapReader {
public:
    void feed(std::string_view bytes) { buffer_.append(bytes); }
    // The next whole body, or std::nullopt until more bytes have come.
    std::optional<std::string> next();
    void clear() { buffer_.clear(); }

private:
    std::string buffer_;
};

// One body with its header, ready to send.
std::string frame(std::string_view body);

} // namespace cu1600::dap
