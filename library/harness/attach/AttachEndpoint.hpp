#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Harness attach, the transport (C59-5 AC-10 to AC-13, EC-1, EC-2, EC-4;
// design §3.2): a TCP port bound to 127.0.0.1 only, one client at a time,
// text lines. A host adapter at the ADR 0009 boundary (LIB-1 design §2a):
// it has an I/O thread of its own and its own queue of outgoing lines, and
// never touches the session. What arrives goes to an AttachInbox, which the
// host of the session implements (cu59::SessionThread; a loop in the tests).
// Compiled only with CALCU_ATTACH (POSIX; never in a release or device
// build, C59-5 AC-13).

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

    Opened open();   // bind 127.0.0.1:port and listen; fails without throwing (EC-1)
    std::uint16_t port() const { return port_; }   // the bound port once open
    // Any thread: lines for that client, in order; dropped when it is gone.
    void write(std::uint64_t client, std::vector<std::string> lines);
    // The session's side ends an attach (EC-2 tests): the client is closed.
    void close(std::uint64_t client);

private:
    void run();
    void wake();
    void dropClient(bool tell);

    AttachInbox& inbox_;
    std::uint16_t port_;
    int listen_ = -1, client_ = -1, wake_[2] = {-1, -1};
    std::uint64_t clientId_ = 0;
    std::string in_;
    std::mutex mutex_;
    std::deque<std::pair<std::uint64_t, std::string>> out_;
    std::vector<std::uint64_t> closing_;
    bool quit_ = false;
    std::thread thread_;
};

} // namespace calcu::harness
