#pragma once
#include "shell/HitMap.hpp"
#include <QObject>
#include <QPointF>
#include <cstdint>
#include <functional>
#include <set>

class QWidget;

// The pointer on a product's faceplate (LIB-14 design §3.7, §3.8; spec AC-24,
// AC-43, EC-8): an event filter on the faceplate widget that hands mouse and
// touch to PointerTracker and sends what follows (screen-key-down/up,
// speed). Key events are not touched, so the window's shortcuts reach their
// actions first (AC-4). The product draws pressed keys from live input
// (H6).

namespace calcu::qt6 {

class SessionHost;

class FaceplateInput : public QObject {
    Q_OBJECT
public:
    static constexpr std::int64_t kMousePointer = 0;   // touch points: their ID + 1

    FaceplateInput(QWidget& faceplate, SessionHost&, shell::HitMap, std::function<double()> scale);
    // LIB-14 Part E (design §10.3.4, D-44): the hit map's (0, 0) in the
    // widget (none: the widget's corner), and a press the product takes
    // first, in hit-map units: that press is not consumed (the filter
    // returns false), and the moves and the release of that pointer pass
    // through to the product's widget as well.
    FaceplateInput(QWidget& faceplate, SessionHost&, shell::HitMap, std::function<double()> scale,
                   std::function<QPointF()> origin, std::function<bool(double x, double y)> productFirst);
    void releaseAll();   // a model switch, the window losing focus

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    void send(const shell::PointerTracker::Out&);
    QPointF local(QPointF widgetPoint) const;      // the origin subtracted
    bool productTakes(QPointF widgetPoint) const;  // productFirst, in hit-map units
    QWidget& faceplate_;
    SessionHost& host_;
    shell::PointerTracker tracker_;
    std::function<double()> scale_;
    std::function<QPointF()> origin_;
    std::function<bool(double, double)> productFirst_;
    bool mousePassing_ = false;                     // the mouse's press went to the product
    std::set<std::int64_t> touchPassing_;           // touch points that went to the product
};

} // namespace calcu::qt6
