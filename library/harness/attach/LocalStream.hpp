#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// A one-client byte stream on 127.0.0.1 (C1600-3 design §3.5; C59-5
// design §3.2): harness attach's listener, for bytes instead of lines.
// AttachEndpoint keeps its lines on it; Calc-U-1600's `calcu serve --dap`
// carries the DAP on it. A host adapter at the ADR 0009 boundary: it has an
// I/O thread of its own and its own queue of outgoing bytes, and never
// touches a session. A second client gets `busy` and is closed; the first
// stays. Compiled only with CALCU_ATTACH (POSIX).

namespace calcu::harness {

struct StreamEvent {
    enum class Kind { Connected, Bytes, Disconnected } kind = Kind::Bytes;
    std::string bytes;           // Bytes: as they came
    std::uint64_t client = 0;    // which connection (a new one for each)
};

class LocalStream {
public:
    struct Opened {
        bool ok = false;
        std::string problem;   // "Address already in use", …
    };
    // `sink` is called on the I/O thread, in order; it never waits for a host call.
    LocalStream(std::function<void(StreamEvent)> sink, std::uint16_t port, std::string busy);
    ~LocalStream();   // closes the client and the socket, joins
    LocalStream(const LocalStream&) = delete;
    LocalStream& operator=(const LocalStream&) = delete;

    Opened open();   // bind 127.0.0.1:port and listen; fails without throwing
    std::uint16_t port() const { return port_; }   // the bound port once open
    // Any thread: bytes for that client, in order; dropped when it is gone.
    void write(std::uint64_t client, std::string bytes);
    // Any thread: the client is closed after what was written before
    // (a Disconnected event follows).
    void close(std::uint64_t client);

private:
    void run();
    void wake();
    void dropClient();

    std::function<void(StreamEvent)> sink_;
    std::uint16_t port_;
    std::string busy_;
    int listen_ = -1, client_ = -1, wake_[2] = {-1, -1};
    std::uint64_t clientId_ = 0;
    std::mutex mutex_;
    std::deque<std::pair<std::uint64_t, std::string>> out_;
    std::vector<std::uint64_t> closing_;
    bool quit_ = false;
    std::thread thread_;
};

} // namespace calcu::harness
