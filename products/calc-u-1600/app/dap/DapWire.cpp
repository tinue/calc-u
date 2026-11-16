#include "app/dap/DapWire.hpp"

#include <cctype>
#include <cstdlib>

namespace shared::dap {

std::optional<std::string> DapReader::next() {
    for (;;) {
        const std::size_t headerEnd = buffer_.find("\r\n\r\n");
        if (headerEnd == std::string::npos) return std::nullopt;
        long length = -1;
        std::size_t lineStart = 0;
        while (lineStart < headerEnd) {
            std::size_t lineEnd = buffer_.find("\r\n", lineStart);
            if (lineEnd == std::string::npos || lineEnd > headerEnd) lineEnd = headerEnd;
            std::string line = buffer_.substr(lineStart, lineEnd - lineStart);
            for (char& c : line) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (line.rfind("content-length:", 0) == 0) length = std::strtol(line.c_str() + 15, nullptr, 10);
            lineStart = lineEnd + 2;
        }
        if (length < 0) {   // not DAP: the header is dropped
            buffer_.erase(0, headerEnd + 4);
            continue;
        }
        const std::size_t total = headerEnd + 4 + static_cast<std::size_t>(length);
        if (buffer_.size() < total) return std::nullopt;
        std::string body = buffer_.substr(headerEnd + 4, static_cast<std::size_t>(length));
        buffer_.erase(0, total);
        return body;
    }
}

std::string frame(std::string_view body) {
    std::string out = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    out.append(body);
    return out;
}

} // namespace shared::dap
