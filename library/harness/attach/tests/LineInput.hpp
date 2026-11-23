#pragma once
// The typed lines of `calcu attach` in a test: each call gives the next
// line, then nothing (the end of the input).

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

inline std::function<std::optional<std::string>()> input(std::vector<std::string> lines) {
    auto q = std::make_shared<std::deque<std::string>>(lines.begin(), lines.end());
    return [q]() -> std::optional<std::string> {
        if (q->empty()) return std::nullopt;
        std::string l = q->front();
        q->pop_front();
        return l;
    };
}
