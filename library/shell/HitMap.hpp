#pragma once
#include "machine/Ids.hpp"
#include "sessions/Messages.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The hit rule and the pointers on a faceplate. The hit map is the
// product's: an area per key in its own units and one distance in logical
// pixels of the shown faceplate. Mirrored in shellModel.js
// (calcuShellKeyAt, calcuShellPointer); both read tests/hit_cases.txt.

namespace calcu::shell {

struct Rect {
    double x = 0, y = 0, w = 0, h = 0;
};

struct HitMap {
    std::vector<std::pair<machine::KeyId, Rect>> keys;   // in the product's order
    std::optional<Rect> display;
    double distance = 0;   // logical pixels
};

// `scale`: logical pixels per hit-map unit as shown now; (px, py) in logical
// pixels. Inside an area, edges included, the first; else the nearest
// within `distance`, the first of equals; else none.
std::optional<machine::KeyId> keyAt(const HitMap&, double scale, double px, double py);
// Whether the point is on the display area (holding it runs at full speed).
bool onDisplay(const HitMap&, double scale, double px, double py);

// Per faceplate: what each pointer holds, and the commands that follow.
class PointerTracker {
public:
    struct Out {
        std::vector<sessions::Command> commands;   // screen-key-down/up {key, pointer}, speed {full}
    };
    explicit PointerTracker(HitMap map = {}, std::function<double()> scale = {});
    const HitMap& map() const { return map_; }

    Out down(std::int64_t pointer, double px, double py);
    Out move(std::int64_t pointer, double px, double py);
    Out up(std::int64_t pointer);   // also cancel
    void releaseAll(Out&);          // a model switch, the window losing focus
    Out releaseAll() {
        Out o;
        releaseAll(o);
        return o;
    }

private:
    struct Held {
        std::optional<machine::KeyId> key;   // fixed at down; a drag does not move it to another key
        bool display = false;
    };
    double scale() const { return scale_ ? scale_() : 1.0; }
    int onDisplayCount() const;
    void endDisplay(std::int64_t pointer, Held&, Out&);

    HitMap map_;
    std::function<double()> scale_;
    std::map<std::int64_t, Held> held_;
};

} // namespace calcu::shell
