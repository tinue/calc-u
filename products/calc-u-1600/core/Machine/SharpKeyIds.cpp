#include "Machine/SharpKeyIds.hpp"

namespace sharp {

namespace {

std::vector<std::string> commonKeys() {
    std::vector<std::string> keys;
    for (char c = '0'; c <= '9'; ++c) keys.emplace_back(1, c);
    for (char c = 'a'; c <= 'z'; ++c) keys.emplace_back(1, c);
    for (const char* k : {"+", "-", "=", "*", "/", "(", ")", ".", "cl", "enter", "mode", "def", "sml", "rcl", "shift",
                          "off", "up", "down", "left", "right", "rsv", "space", "f1", "f2", "f3", "f4", "f5", "f6", "on"})
        keys.emplace_back(k);
    return keys;
}

} // namespace

const std::vector<std::string>& pc1500KeyIds() {
    static const std::vector<std::string> keys = commonKeys();
    return keys;
}

const std::vector<std::string>& pc1600KeyIds() {
    static const std::vector<std::string> keys = [] {
        std::vector<std::string> k = commonKeys();
        for (const char* extra : {"ctrl", "kbii", "bs"}) k.emplace_back(extra);
        return k;
    }();
    return keys;
}

} // namespace sharp
