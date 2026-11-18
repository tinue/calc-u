#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// Bytes of a serial or printer port as text: a
// line end (LF) and printable ASCII stand as they are, a carriage return is
// left out, any other byte is `<XX>` (hex). One rule for the GUI's printer
// pane of the CE-158 and the harness's `c1600 serial <port> report`.
inline std::string printableBytes(const std::uint8_t* bytes, std::size_t size) {
    static const char kHex[] = "0123456789ABCDEF";
    std::string text;
    for (std::size_t i = 0; i < size; ++i) {
        const std::uint8_t b = bytes[i];
        if (b == '\r') continue;
        if (b == '\n' || (b >= 0x20 && b < 0x7F)) {
            text += static_cast<char>(b);
        } else {
            text += '<';
            text += kHex[b >> 4];
            text += kHex[b & 0x0F];
            text += '>';
        }
    }
    return text;
}
