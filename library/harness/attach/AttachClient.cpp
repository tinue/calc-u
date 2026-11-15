#include "harness/attach/AttachClient.hpp"
#include "harness/attach/AttachPort.hpp"
#include <cstdlib>
#include <cstring>
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define CALCU_ATTACH_CLIENT 1
#endif

namespace calcu::harness {

namespace {

const char* const kNotHere[] = {"--product", "--settings", "--save-folder", "--output-folder", "--rom-folder", "--roms",
                                "--clock"};

#ifdef CALCU_ATTACH_CLIENT
struct Line {
    int fd;
    std::string buf;
    std::optional<std::string> next() {
        for (;;) {
            const auto nl = buf.find('\n');
            if (nl != std::string::npos) {
                std::string l = buf.substr(0, nl);
                buf.erase(0, nl + 1);
                return l;
            }
            char c[4096];
            const ssize_t n = ::recv(fd, c, sizeof c, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return std::nullopt;
            buf.append(c, static_cast<std::size_t>(n));
        }
    }
};

bool sendLine(int fd, const std::string& l) {
    const std::string d = l + "\n";
    std::size_t off = 0;
    while (off < d.size()) {
        const ssize_t n = ::send(fd, d.data() + off, d.size() - off, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<std::size_t>(n);
    }
    return true;
}

// One reply: plain `done`, `done <n>` and n lines, `refused: …`; a JSON
// object. Message lines before it are printed only with --messages.
bool relayReply(Line& in, bool json, bool messages, const std::function<void(const std::string&)>& out) {
    for (;;) {
        auto l = in.next();
        if (!l) return false;
        const bool message = l->rfind("message ", 0) == 0 || l->rfind("{\"message\":", 0) == 0;
        if (message) {
            if (messages) out(*l + "\n");
            continue;
        }
        out(*l + "\n");
        if (!json && l->rfind("done ", 0) == 0) {
            const int n = std::atoi(l->c_str() + 5);
            for (int i = 0; i < n; ++i) {
                auto x = in.next();
                if (!x) return false;
                out(*x + "\n");
            }
        }
        return true;
    }
}
#endif

} // namespace

int runAttach(const std::vector<std::string>& args, const std::function<std::optional<std::string>()>& readLine,
              const std::function<void(const std::string&)>& out, const std::function<void(const std::string&)>& err) {
    std::vector<std::string> portArgs;
    bool json = false, messages = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        for (const char* o : kNotHere)
            if (a == o || a.rfind(std::string(o) + "=", 0) == 0) {   // AC-11, D-4
                err(a + " does not apply to an attached harness: it works with the app's own product, settings and folders.\n");
                return 2;
            }
        if (a == "--json") json = true;
        else if (a == "--messages") messages = true;
        else if (a == "--port" && i + 1 < args.size()) {
            portArgs = {"--attach-port", args[++i]};
        } else {
            err("usage: calcu attach [--port N] [--json] [--messages]\n");
            return 2;
        }
    }
    const std::uint16_t port = attachPort(portArgs);
#ifndef CALCU_ATTACH_CLIENT
    (void)readLine, (void)out, (void)json, (void)messages, (void)port;
    err("calcu attach is not available on this platform.\n");
    return 2;
#else
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (fd < 0 || ::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
        err("Cannot attach to 127.0.0.1:" + std::to_string(port) + ": " + std::strerror(errno) + "\n");
        if (fd >= 0) ::close(fd);
        return 2;
    }
    Line in{fd, {}};
    if (json) {   // D-2: the app formats JSON from now on
        sendLine(fd, "json on");
        relayReply(in, true, false, [](const std::string&) {});
    }
    while (auto l = readLine()) {
        std::string line = *l;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") == std::string::npos) continue;
        if (!sendLine(fd, line)) break;
        if (line == "quit") break;
        if (!relayReply(in, json, messages, out)) break;
    }
    sendLine(fd, "quit");
    ::close(fd);
    return 0;
#endif
}

} // namespace calcu::harness
