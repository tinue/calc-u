#include "qt6/FaceplateInput.hpp"
#include "qt6/SessionHost.hpp"
#include <QEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWidget>

namespace calcu::qt6 {

FaceplateInput::FaceplateInput(QWidget& faceplate, SessionHost& host, shell::HitMap map, std::function<double()> scale)
    : QObject(&faceplate), faceplate_(faceplate), host_(host), tracker_(std::move(map), std::move(scale)) {
    faceplate_.setAttribute(Qt::WA_AcceptTouchEvents);
    faceplate_.installEventFilter(this);
}

void FaceplateInput::send(const shell::PointerTracker::Out& out) {
    for (const auto& c : out.commands) host_.send(c.name, c.args);
}

void FaceplateInput::releaseAll() { send(tracker_.releaseAll()); }

bool FaceplateInput::eventFilter(QObject* o, QEvent* e) {
    if (o != &faceplate_) return false;
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick: {
        auto* m = static_cast<QMouseEvent*>(e);
        if (m->button() != Qt::LeftButton) return false;
        const QPointF p = m->position();
        send(tracker_.down(kMousePointer, p.x(), p.y()));
        return true;
    }
    case QEvent::MouseMove: {
        auto* m = static_cast<QMouseEvent*>(e);
        if (!(m->buttons() & Qt::LeftButton)) return false;
        const QPointF p = m->position();
        send(tracker_.move(kMousePointer, p.x(), p.y()));
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto* m = static_cast<QMouseEvent*>(e);
        if (m->button() != Qt::LeftButton) return false;
        send(tracker_.up(kMousePointer));
        return true;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        auto* t = static_cast<QTouchEvent*>(e);
        for (const auto& point : t->points()) {
            const std::int64_t id = static_cast<std::int64_t>(point.id()) + 1;
            const QPointF p = point.position();
            if (e->type() == QEvent::TouchCancel) {
                send(tracker_.up(id));
                continue;
            }
            switch (point.state()) {
            case QEventPoint::State::Pressed: send(tracker_.down(id, p.x(), p.y())); break;
            case QEventPoint::State::Updated: send(tracker_.move(id, p.x(), p.y())); break;
            case QEventPoint::State::Released: send(tracker_.up(id)); break;
            default: break;
            }
        }
        e->accept();
        return true;
    }
    case QEvent::Hide:
    case QEvent::WindowDeactivate:
        releaseAll();
        return false;
    default: return false;
    }
}

} // namespace calcu::qt6
