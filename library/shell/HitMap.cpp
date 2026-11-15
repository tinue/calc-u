#include "shell/HitMap.hpp"
#include "shell/Inputs.hpp"
#include <algorithm>
#include <cmath>

namespace calcu::shell {

namespace {

Rect scaled(const Rect& r, double s) { return Rect{r.x * s, r.y * s, r.w * s, r.h * s}; }

bool inside(const Rect& r, double px, double py) {   // edges included (D-8)
    return px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h;
}

double distanceTo(const Rect& r, double px, double py) {   // Euclidean, 0 inside
    const double dx = std::max({r.x - px, 0.0, px - (r.x + r.w)});
    const double dy = std::max({r.y - py, 0.0, py - (r.y + r.h)});
    return std::sqrt(dx * dx + dy * dy);
}

sessions::Command keyCommand(const sessions::CommandName& n, const machine::KeyId& k, std::int64_t pointer) {
    return sessions::Command{n, sessions::Value::object({{"key", k.value}, {"pointer", pointer}})};
}

sessions::Command speed(bool full) { return sessions::Command{commands::kSpeed, sessions::Value::object({{"full", full}})}; }

} // namespace

std::optional<machine::KeyId> keyAt(const HitMap& map, double scale, double px, double py) {
    for (const auto& [key, r] : map.keys)   // H1
        if (inside(scaled(r, scale), px, py)) return key;
    std::optional<machine::KeyId> best;
    double bestDistance = 0;
    for (const auto& [key, r] : map.keys) {   // H2
        const double d = distanceTo(scaled(r, scale), px, py);
        if (d <= map.distance && (!best || d < bestDistance)) {
            best = key;
            bestDistance = d;
        }
    }
    return best;   // H3
}

bool onDisplay(const HitMap& map, double scale, double px, double py) {
    return map.display && inside(scaled(*map.display, scale), px, py);
}

PointerTracker::PointerTracker(HitMap map, std::function<double()> scale)
    : map_(std::move(map)), scale_(std::move(scale)) {}

int PointerTracker::onDisplayCount() const {
    int n = 0;
    for (const auto& [id, h] : held_) n += h.display ? 1 : 0;
    return n;
}

void PointerTracker::endDisplay(std::int64_t, Held& h, Out& out) {
    if (!h.display) return;
    h.display = false;
    if (onDisplayCount() == 0) out.commands.push_back(speed(false));   // H5
}

PointerTracker::Out PointerTracker::down(std::int64_t pointer, double px, double py) {
    Out out;
    if (held_.count(pointer)) {   // a second down without an up: the first ends
        Out o = up(pointer);
        out.commands = std::move(o.commands);
    }
    Held h;
    const double s = scale();
    if (onDisplay(map_, s, px, py)) {   // H5
        const bool first = onDisplayCount() == 0;
        h.display = true;
        if (first) out.commands.push_back(speed(true));
    } else if (auto k = keyAt(map_, s, px, py)) {   // H4
        h.key = k;
        out.commands.push_back(keyCommand(commands::kScreenKeyDown, *k, pointer));
    }
    held_[pointer] = h;   // EC-8: no key, nothing held, also when it moves onto one
    return out;
}

PointerTracker::Out PointerTracker::move(std::int64_t pointer, double px, double py) {
    Out out;
    auto it = held_.find(pointer);
    if (it == held_.end()) return out;
    // D-9: a move changes no key; H5: leaving the display ends full speed.
    if (it->second.display && !onDisplay(map_, scale(), px, py)) endDisplay(pointer, it->second, out);
    return out;
}

PointerTracker::Out PointerTracker::up(std::int64_t pointer) {
    Out out;
    auto it = held_.find(pointer);
    if (it == held_.end()) return out;
    if (it->second.key) out.commands.push_back(keyCommand(commands::kScreenKeyUp, *it->second.key, pointer));
    endDisplay(pointer, it->second, out);
    held_.erase(it);
    return out;
}

void PointerTracker::releaseAll(Out& out) {
    std::vector<std::int64_t> ids;
    for (const auto& [id, h] : held_) ids.push_back(id);
    for (auto id : ids) {
        Out o = up(id);
        for (auto& c : o.commands) out.commands.push_back(std::move(c));
    }
}

} // namespace calcu::shell
