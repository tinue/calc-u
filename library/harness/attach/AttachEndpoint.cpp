#include "harness/attach/AttachEndpoint.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace calcu::harness {

namespace {

std::string trimmedLine(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
    std::size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t')) ++b;
    return s.substr(b);
}

void sendAll(int fd, const std::string& data) {
    std::size_t off = 0;
    while (off < data.size()) {
#ifdef MSG_NOSIGNAL
        const ssize_t n = ::send(fd, data.data() + off, data.size() - off, MSG_NOSIGNAL);
#else
        const ssize_t n = ::send(fd, data.data() + off, data.size() - off, 0);
#endif
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return;
        off += static_cast<std::size_t>(n);
    }
}

void noSigPipe([[maybe_unused]] int fd) {
#ifdef SO_NOSIGPIPE
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
#endif
}

} // namespace

AttachEndpoint::AttachEndpoint(AttachInbox& inbox, std::uint16_t port) : inbox_(inbox), port_(port) {}

AttachEndpoint::~AttachEndpoint() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        quit_ = true;
    }
    wake();
    if (thread_.joinable()) thread_.join();
    if (client_ >= 0) ::close(client_);
    if (listen_ >= 0) ::close(listen_);
    for (int fd : wake_)
        if (fd >= 0) ::close(fd);
}

AttachEndpoint::Opened AttachEndpoint::open() {
    Opened o;
    listen_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_ < 0) {
        o.problem = std::strerror(errno);
        return o;
    }
    int one = 1;
    ::setsockopt(listen_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);   // not SO_REUSEPORT: a second app must fail (EC-1)
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port_);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // localhost only (AC-10)
    if (::bind(listen_, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0 || ::listen(listen_, 4) != 0) {
        o.problem = std::strerror(errno);
        ::close(listen_);
        listen_ = -1;
        return o;
    }
    socklen_t len = sizeof a;
    if (::getsockname(listen_, reinterpret_cast<sockaddr*>(&a), &len) == 0) port_ = ntohs(a.sin_port);
    if (::pipe(wake_) != 0) {
        o.problem = std::strerror(errno);
        return o;
    }
    ::fcntl(wake_[0], F_SETFL, O_NONBLOCK);
    ::fcntl(wake_[1], F_SETFL, O_NONBLOCK);
    thread_ = std::thread([this] { run(); });
    o.ok = true;
    return o;
}

void AttachEndpoint::wake() {
    if (wake_[1] >= 0) {
        const char c = 1;
        [[maybe_unused]] auto n = ::write(wake_[1], &c, 1);
    }
}

void AttachEndpoint::write(std::uint64_t client, std::vector<std::string> lines) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& l : lines) out_.emplace_back(client, std::move(l));
    }
    wake();
}

void AttachEndpoint::close(std::uint64_t client) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closing_.push_back(client);
    }
    wake();
}

void AttachEndpoint::dropClient(bool tell) {
    if (client_ < 0) return;
    if (tell) inbox_.post(AttachEvent{AttachEvent::Kind::Detached, {}, clientId_});   // before the client sees the close
    ::close(client_);
    client_ = -1;
    in_.clear();
}

void AttachEndpoint::run() {
    char buf[4096];
    for (;;) {
        pollfd fds[3] = {{listen_, POLLIN, 0}, {wake_[0], POLLIN, 0}, {client_, POLLIN, 0}};
        const int n = ::poll(fds, client_ >= 0 ? 3 : 2, -1);
        if (n < 0 && errno != EINTR) return;
        if (fds[1].revents & POLLIN)
            while (::read(wake_[0], buf, sizeof buf) > 0) {
            }
        std::deque<std::pair<std::uint64_t, std::string>> out;
        std::vector<std::uint64_t> closing;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (quit_) return;
            out.swap(out_);
            closing.swap(closing_);
        }
        if (client_ >= 0) {
            std::string data;
            for (auto& [c, line] : out)
                if (c == clientId_) data += line + "\n";
            if (!data.empty()) sendAll(client_, data);
            for (auto c : closing)
                if (c == clientId_) dropClient(true);
        }
        if (fds[0].revents & POLLIN) {
            const int fd = ::accept(listen_, nullptr, nullptr);
            if (fd >= 0) {
                noSigPipe(fd);
                if (client_ >= 0) {   // EC-4: one line, then closed; the first stays
                    sendAll(fd, std::string(kSecondHarness) + "\n");
                    ::close(fd);
                } else {
                    client_ = fd;
                    ++clientId_;
                    inbox_.post(AttachEvent{AttachEvent::Kind::Attached, {}, clientId_});
                }
            }
        }
        if (client_ >= 0 && n > 0 && (fds[2].revents & (POLLIN | POLLHUP | POLLERR))) {
            const ssize_t got = ::read(client_, buf, sizeof buf);
            if (got <= 0) {   // EC-2: gone without quit, as quit
                dropClient(true);
                continue;
            }
            in_.append(buf, static_cast<std::size_t>(got));
            std::size_t nl;
            while (client_ >= 0 && (nl = in_.find('\n')) != std::string::npos) {
                std::string line = trimmedLine(in_.substr(0, nl));
                in_.erase(0, nl + 1);
                if (line == "quit") {   // AC-11: ends the attach, not the app
                    dropClient(true);
                    break;
                }
                inbox_.post(AttachEvent{AttachEvent::Kind::Line, std::move(line), clientId_});
            }
        }
    }
}

} // namespace calcu::harness
