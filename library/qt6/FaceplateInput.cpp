#include "qt6/FaceplateInput.hpp"
#include "qt6/SessionHost.hpp"
#include <QEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWidget>

namespace calcu::qt6 {

FaceplateInput::FaceplateInput(QWidget& faceplate, SessionHost& host, shell::HitMap map, std::function<double()> scale)
    : FaceplateInput(faceplate, host, std::move(map), std::move(scale), {}, {}) {}

FaceplateInput::FaceplateInput(QWidget& faceplate, SessionHost& host, shell::HitMap map, std::function<double()> scale,
                               std::function<QPointF()> origin, std::function<bool(double, double)> productFirst)
    : QObject(&faceplate), faceplate_(faceplate), host_(host), tracker_(std::move(map), scale), scale_(std::move(scale)),
      origin_(std::move(origin)), productFirst_(std::move(productFirst)) {
    faceplate_.setAttribute(Qt::WA_AcceptTouchEvents);
    faceplate_.installEventFilter(this);
}

QPointF FaceplateInput::local(QPointF p) const { return origin_ ? p - origin_() : p; }

bool FaceplateInput::productTakes(QPointF p) const {
    if (!productFirst_) return false;
    const QPointF l = local(p);
    const double s = scale_ ? scale_() : 1.0;
    return s > 0 && productFirst_(l.x() / s, l.y() / s);
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
        if (productTakes(m->position())) {   // the product's widget gets it
            mousePassing_ = true;
            return false;
        }
        const QPointF p = local(m->position());
        send(tracker_.down(kMousePointer, p.x(), p.y()));
        return true;
    }
    case QEvent::MouseMove: {
        auto* m = static_cast<QMouseEvent*>(e);
        if (!(m->buttons() & Qt::LeftButton)) return false;
        if (mousePassing_) return false;
        const QPointF p = local(m->position());
        send(tracker_.move(kMousePointer, p.x(), p.y()));
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto* m = static_cast<QMouseEvent*>(e);
        if (m->button() != Qt::LeftButton) return false;
        if (mousePassing_) {
            mousePassing_ = false;
            return false;
        }
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
            const QPointF p = local(point.position());
            if (point.state() == QEventPoint::State::Pressed && productTakes(point.position())) touchPassing_.insert(id);
            if (touchPassing_.count(id)) {   // the product's: Qt synthesises its mouse events
                if (point.state() == QEventPoint::State::Released || e->type() == QEvent::TouchCancel) touchPassing_.erase(id);
                continue;
            }
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
