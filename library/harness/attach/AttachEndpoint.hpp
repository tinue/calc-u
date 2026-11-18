#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "harness/attach/LocalStream.hpp"

// Harness attach, the transport: a TCP port bound to 127.0.0.1 only, one
// client at a time, text lines. A host adapter at the edge of the
// single-threaded session: it keeps its lines on LocalStream (an I/O thread of its own and its own
// queue of outgoing bytes) and never touches the session. What arrives goes to an AttachInbox, which the
// host of the session implements (cu59::SessionThread; a loop in the tests).
// Compiled only with CALCU_ATTACH (POSIX; never in a release or device
// build).

namespace calcu::harness {

struct AttachEvent {
    enum class Kind { Attached, Line, Detached } kind = Kind::Line;
    std::string line;            // Line: one command line, without its end
    std::uint64_t client = 0;    // which connection (a new one for each attach)
};

class AttachInbox {
public:
    virtual void post(AttachEvent) = 0;   // any thread; never waits for a host call
protected:
    ~AttachInbox() = default;
};

class AttachEndpoint {
public:
    struct Opened {
        bool ok = false;
        std::string problem;   // "Address already in use", …
    };
    static constexpr const char* kSecondHarness = "refused: one harness is already attached";

    AttachEndpoint(AttachInbox&, std::uint16_t port);   // port 0: one the system picks (tests)
    ~AttachEndpoint();                                  // closes the client and the socket, joins
    AttachEndpoint(const AttachEndpoint&) = delete;
    AttachEndpoint& operator=(const AttachEndpoint&) = delete;

    Opened open();   // bind 127.0.0.1:port and listen; fails without throwing
    std::uint16_t port() const { return stream_.port(); }   // the bound port once open
    // Any thread: lines for that client, in order; dropped when it is gone.
    void write(std::uint64_t client, std::vector<std::string> lines);
    // The session's side ends an attach (tests): the client is closed.
    void close(std::uint64_t client);

private:
    // On the stream's I/O thread: bytes into lines; `quit` ends the attach.
    void onStream(StreamEvent);

    AttachInbox& inbox_;
    std::string in_;
    std::uint64_t quitClient_ = 0;   // told Detached at its `quit`; its close is not told again
    LocalStream stream_;             // last: its thread stops first
};

} // namespace calcu::harness
