#pragma once
#include "shell/HitMap.hpp"
#include <QObject>
#include <functional>

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
    void releaseAll();   // a model switch, the window losing focus

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    void send(const shell::PointerTracker::Out&);
    QWidget& faceplate_;
    SessionHost& host_;
    shell::PointerTracker tracker_;
};

} // namespace calcu::qt6
