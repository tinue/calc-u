#include "KeyboardWidget.hpp"
#include "CueCardRenderer.hpp"
#include "EmulatorController.hpp"
#include "LedRenderer.hpp"
#include "TIShellProduct.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace {
QRectF norm(const QRectF& canvas, const cu59::NRect& r) {
    return QRectF(canvas.left() + r.x * canvas.width(), canvas.top() + r.y * canvas.height(), r.w * canvas.width(),
                  r.h * canvas.height());
}
} // namespace

KeyboardWidget::KeyboardWidget(EmulatorController& controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setObjectName(QStringLiteral("faceplate"));
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto reload = [this] {
        // The TI-59's art with the card slot; the TI-58 and TI-58C share the other.
        const bool ti59 = m_controller.modelId().empty() || m_controller.isTI59();
        m_base = QPixmap(ti59 ? QStringLiteral(":/img/ti59_base.png") : QStringLiteral(":/img/ti58_base.png"));
        m_scaled = QPixmap();
        update();
    };
    reload();
    connect(&m_controller, &EmulatorController::modelChanged, this, reload);
    m_ledStyle = m_controller.ledStyle();
    // The LED style is the `settings` topic's ([display] led-style).
    connect(&m_controller, &EmulatorController::settingsChanged, this, [this] {
        m_ledStyle = m_controller.ledStyle();
        update();
    });
    // The display changes up to 60 times a second: repaint just the display.
    connect(&m_controller, &EmulatorController::displayChanged, this,
            [this] { update(norm(canvasRect(), cu59::kDisplayRect).toAlignedRect()); });
    for (auto sig : {&EmulatorController::cueCardChanged, &EmulatorController::runStateChanged, &EmulatorController::cardStateChanged})
        connect(&m_controller, sig, this, qOverload<>(&QWidget::update));
}

int KeyboardWidget::heightForWidth(int w) const { return static_cast<int>(w / cu59::kCanvasAspect); }

QSize KeyboardWidget::sizeHint() const { return QSize(420, heightForWidth(420)); }

QRectF KeyboardWidget::canvasRect() const {
    // Keep the artwork's aspect, centred.
    double w = width(), h = height();
    if (h <= 0 || w <= 0) return QRectF();
    if (w / h > cu59::kCanvasAspect) w = h * cu59::kCanvasAspect;
    else h = w / cu59::kCanvasAspect;
    return QRectF((width() - w) / 2, (height() - h) / 2, w, h);
}

double KeyboardWidget::scale() const { return canvasRect().width() / cu59::kCanvasWidth; }

bool KeyboardWidget::cardTakes(double x, double y) const {
    if (!m_controller.cardOnShow()) return false;
    return cu59::kCardRect.contains(x / cu59::kCanvasWidth, y / cu59::kCanvasHeight);
}

void KeyboardWidget::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    QRectF c = canvasRect();
    // The artwork scaled once per size (smooth scaling the 2064×4075 image is costly).
    const qreal dpr = devicePixelRatioF();
    const QSize pixels = (c.size() * dpr).toSize();
    if (m_scaled.size() != pixels) {
        m_scaled = m_base.scaled(pixels, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_scaled.setDevicePixelRatio(dpr);
    }
    p.drawPixmap(c, m_scaled, QRectF());

    QRectF display = norm(c, cu59::kDisplayRect);
    if (e->rect().intersects(display.toAlignedRect()))
        LedRenderer::paint(p, display, m_controller.display(), m_controller.calcIndicatorOpacity(), m_ledStyle);
    QRectF cardRect = norm(c, cu59::kCardRect);
    if (const auto& card = m_controller.cueCard(); card && e->rect().intersects(cardRect.toAlignedRect()))
        CueCardRenderer::paint(p, cardRect, *card, m_controller.litCardCorner());

    // The pressed key (live input's: the pointer's or the
    // physical keyboard's): black out the key and draw its artwork shifted
    // 3 px down-right.
    const int pk = m_controller.keyboardHeldKey();
    if (pk >= 0) {
        const cu59::NRect& r = cu59::kKeyRects[pk / 5][pk % 5];
        QRectF key(c.left() + r.x * c.width(), c.top() + (cu59::kKeyboardYStart + r.y * cu59::kKeyboardYScale) * c.height(),
                   r.w * c.width(), r.h * cu59::kKeyboardYScale * c.height());
        p.fillRect(key, Qt::black);
        QRectF src((key.left() - c.left()) / c.width() * m_base.width(), (key.top() - c.top()) / c.height() * m_base.height(),
                   key.width() / c.width() * m_base.width(), key.height() / c.height() * m_base.height());
        p.drawPixmap(key.translated(3, 3), m_base, src);
    }
}

// The shell lets through only a press cardTakes() said is the product's: a
// tap on the fitted card, whose top corner feeds that side.
void KeyboardWidget::mousePressEvent(QMouseEvent* e) {
    setFocus();   // a click on the calculator claims the physical keyboard
    if (e->button() != Qt::LeftButton) return;
    const QRectF c = canvasRect();
    if (c.isEmpty()) return;
    m_controller.tapCard((e->position().x() - c.left()) / c.width(), (e->position().y() - c.top()) / c.height());
}

void KeyboardWidget::keyPressEvent(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;   // the host's repeat: ignored
    const auto k = m_hostKeys.press(*e);
    if (!k) {
        QWidget::keyPressEvent(e);
        return;
    }
    m_controller.hostKeyDown(*k);   // the TI keyboard map refuses a command chord
}

void KeyboardWidget::keyReleaseEvent(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;
    const auto physical = m_hostKeys.release(*e);
    if (!physical) {
        QWidget::keyReleaseEvent(e);
        return;
    }
    m_controller.hostKeyUp(*physical);
}

void KeyboardWidget::focusOutEvent(QFocusEvent* e) {
    m_hostKeys.clear();
    m_controller.focusLost();   // every key held comes up, after its minimum
    QWidget::focusOutEvent(e);
}

void KeyboardWidget::hideEvent(QHideEvent* e) {
    m_hostKeys.clear();
    m_controller.focusLost();
    QWidget::hideEvent(e);
}
