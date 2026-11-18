#pragma once
#include <cstdint>
#include <optional>
#include <string>

// What a host gives the DAP server: a
// socket on this computer, and the way back onto the session's thread. The
// server calls these on the session's thread; the host hands what arrives
// to DapPart::take(), on the session's thread, in the order it came. The
// Qt6 frontend's is frontends/qt6/app/debug/DapSocket (QTcpServer), the
// harness's app/HarnessProduct.cpp's on library/harness/attach/LocalStream.

namespace cu1600::dap {

class DapTransport {
public:
    virtual ~DapTransport() = default;
    // Listens on 127.0.0.1:port; the reason it cannot (std::nullopt: it
    // listens). A second client gets `busy` and is closed by the host.
    virtual std::optional<std::string> listen(std::uint16_t port, std::string busy) = 0;
    virtual void close() = 0;                       // stops listening, drops a client
    virtual void send(std::string bytes) = 0;       // to the client (nothing without one)
    virtual void dropClient() = 0;                  // after what was sent
    virtual void quitProgram() = 0;                 // c1600/quit
};

struct DapInput {
    enum class Kind { Connected, Bytes, Disconnected };
    Kind kind = Kind::Bytes;
    std::string bytes;
};

} // namespace cu1600::dap
