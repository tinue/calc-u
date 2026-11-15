// The pseudo-terminal behind a serial port file (LIB-2 design §3.5): today's
// Calc-U-1600 PtySerialLink (products/calc-u-1600/core/Serial/) moved into
// the library. Compiled only where CALCU_HAS_PTY is set (macOS, Linux);
// NoHostLink.cpp elsewhere.
//
// Two I/O threads own the master side: a reader draining it into an RX ring
// (with a high-water mark that lets the terminal's buffer fill and hold back
// a well-behaved sender, the stand-in for a real RTS drop), and a writer
// flushing a TX ring. read() and write() are the session thread's
// lock-guarded view of those rings. A raw pseudo-terminal carries no modem
// lines; peerOpen() approximates "a program holds the file open".
#include "peripherals/HostLink.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <mutex>
#include <poll.h>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace calcu::peripherals {

namespace {

// How long the reader waits before polling again when poll() would return at
// once without anything to do: RX ring full, or no program on the file.
constexpr std::chrono::milliseconds kIdleBackoff{10};
constexpr std::size_t kRxHighWater = 8192;

// <folder>/<name> -> target; the link path, or "" if it could not be made.
std::string makeLink(const std::string& target, std::string folder, const std::string& name) {
    ::mkdir(folder.c_str(), 0755);   // EEXIST ignored: a folder the user granted exists
    // A folder given as "/private/tmp/" must not give "/private/tmp//...".
    while (folder.size() > 1 && folder.back() == '/') folder.pop_back();
    if (folder == "/") folder.clear();
    const std::string link = folder + "/" + name;
    ::unlink(link.c_str());
    if (::symlink(target.c_str(), link.c_str()) != 0) return "";
    return link;
}

// Removes `link` only while it still points at `device`, so that a move or
// the end never deletes a file someone else put there.
void removeLinkIfOurs(const std::string& link, const std::string& device) {
    if (link.empty()) return;
    char buf[1024];
    const ssize_t n = ::readlink(link.c_str(), buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = '\0';
        if (device == buf) ::unlink(link.c_str());
    }
}

class PtyHostLink final : public HostLink {
public:
    PtyHostLink(int master, std::string device, const std::optional<std::string>& folder, std::string name)
        : master_(master), device_(std::move(device)), name_(std::move(name)) {
        if (folder) {
            folder_ = *folder;
            link_ = makeLink(device_, folder_, name_);
        }
        reader_ = std::thread(&PtyHostLink::readerLoop, this);
        writer_ = std::thread(&PtyHostLink::writerLoop, this);
    }
    ~PtyHostLink() override {
        stop_.store(true, std::memory_order_relaxed);
        txCv_.notify_all();
        if (reader_.joinable()) reader_.join();
        if (writer_.joinable()) writer_.join();
        removeLinkIfOurs(link_, device_);
        ::close(master_);
    }
    PtyHostLink(const PtyHostLink&) = delete;
    PtyHostLink& operator=(const PtyHostLink&) = delete;

    std::size_t read(std::span<std::uint8_t> out) override {
        std::lock_guard<std::mutex> lk(rxMx_);
        const std::size_t n = std::min(out.size(), rx_.size());
        std::copy_n(rx_.begin(), n, out.begin());
        rx_.erase(rx_.begin(), rx_.begin() + static_cast<std::ptrdiff_t>(n));
        return n;
    }
    void write(std::span<const std::uint8_t> in) override {
        if (in.empty()) return;
        {
            std::lock_guard<std::mutex> lk(txMx_);
            tx_.insert(tx_.end(), in.begin(), in.end());
        }
        txCv_.notify_one();
    }
    bool peerOpen() const override { return peerOpen_.load(std::memory_order_relaxed); }
    std::string path() const override { return link_.empty() ? device_ : link_; }
    bool moveTo(const std::optional<std::string>& folder) override {
        if (folder && *folder == folder_ && !link_.empty()) return true;   // unchanged
        removeLinkIfOurs(link_, device_);
        link_.clear();
        folder_ = folder.value_or(std::string());
        if (folder) link_ = makeLink(device_, folder_, name_);
        return !link_.empty();
    }

private:
    void readerLoop();
    void writerLoop();

    int master_;
    std::string device_, name_;
    std::string folder_, link_;   // the session thread's only
    std::thread reader_, writer_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> peerOpen_{true};
    std::mutex rxMx_;
    std::deque<std::uint8_t> rx_;
    std::mutex txMx_;
    std::condition_variable txCv_;
    std::deque<std::uint8_t> tx_;
};

void PtyHostLink::readerLoop() {
    std::uint8_t buf[512];
    while (!stop_.load(std::memory_order_relaxed)) {
        pollfd pfd{master_, POLLIN, 0};
        const int pr = ::poll(&pfd, 1, 200);
        if (stop_.load(std::memory_order_relaxed)) break;
        if (pr <= 0) continue;   // timeout, EINTR
        // While the RX ring is deep, leave the bytes in the terminal's buffer
        // so that a blocking sender stalls; poll() stays readable meanwhile,
        // so back off instead of spinning.
        bool full;
        {
            std::lock_guard<std::mutex> lk(rxMx_);
            full = rx_.size() >= kRxHighWater;
        }
        if (full) {
            std::this_thread::sleep_for(kIdleBackoff);
            continue;
        }
        const ssize_t n = ::read(master_, buf, sizeof buf);
        if (n > 0) {
            peerOpen_.store(true, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lk(rxMx_);
            rx_.insert(rx_.end(), buf, buf + n);
        } else if (n == 0 || errno == EIO) {
            // No program holds the file open (EIO on macOS and Linux); poll()
            // keeps reporting the hangup at once, so back off.
            peerOpen_.store(false, std::memory_order_relaxed);
            std::this_thread::sleep_for(kIdleBackoff);
        } else if (errno == EAGAIN || errno == EINTR) {
            // spurious wake-up
        } else {
            break;   // unexpected: the end of this link
        }
    }
}

void PtyHostLink::writerLoop() {
    std::vector<std::uint8_t> chunk;
    for (;;) {
        {
            std::unique_lock<std::mutex> lk(txMx_);
            txCv_.wait(lk, [this] { return stop_.load(std::memory_order_relaxed) || !tx_.empty(); });
            if (stop_.load(std::memory_order_relaxed) && tx_.empty()) return;
            chunk.assign(tx_.begin(), tx_.end());
            tx_.clear();
        }
        std::size_t off = 0;
        while (off < chunk.size() && !stop_.load(std::memory_order_relaxed)) {
            const ssize_t n = ::write(master_, chunk.data() + off, chunk.size() - off);
            if (n > 0) {
                off += static_cast<std::size_t>(n);
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
                pollfd pfd{master_, POLLOUT, 0};
                ::poll(&pfd, 1, 200);
                continue;
            }
            break;   // EIO (no reader) or fatal: the rest of the chunk is dropped
        }
    }
}

} // namespace

std::unique_ptr<HostLink> openHostLink(const std::optional<std::string>& folder, const std::string& fileName) {
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) return nullptr;
    if (::grantpt(master) != 0 || ::unlockpt(master) != 0) {
        ::close(master);
        return nullptr;
    }
    const char* sp = ::ptsname(master);   // static storage: copied at once
    if (!sp) {
        ::close(master);
        return nullptr;
    }
    std::string device = sp;
    const int fl = ::fcntl(master, F_GETFL, 0);
    if (fl >= 0) ::fcntl(master, F_SETFL, fl | O_NONBLOCK);
    // A raw line discipline (no echo, no CR/LF or parity changes) as the
    // default; a program that opens the file sets its own settings on top,
    // and the bytes pass as they are (AC-18). Set on the master so that the
    // file is not opened and closed once (which the DCD guess would misread).
    termios t;
    if (::tcgetattr(master, &t) == 0) {
        ::cfmakeraw(&t);
        ::tcsetattr(master, TCSANOW, &t);
    }
    return std::make_unique<PtyHostLink>(master, std::move(device), folder, fileName);
}

} // namespace calcu::peripherals
