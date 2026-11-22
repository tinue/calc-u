#include "FaceplateWidget.hpp"
#include "app/SharpChrome.hpp"
#include "qt6/ChromeStyle.hpp"
#include "LcdWidget.hpp"

#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <algorithm>

namespace {
constexpr int kFlashMs = 150;   // long enough to see, short as a tap
QRect toQRect(const KeyLayout::PaneRect& r) { return QRect(r.x, r.y, r.w, r.h); }
} // namespace

FaceplateWidget::FaceplateWidget(QWidget* parent) : QWidget(parent) {
    m_lcd = new LcdWidget(this);
    setModel(calcu::machine::ModelId{"PC-1500A"});
}

void FaceplateWidget::setModel(const calcu::machine::ModelId& model) {
    const KeyLayout::Layout* layout = &KeyLayout::layoutFor(model);
    const QString art = model.value == "PC-1600"    ? QStringLiteral(":/faceplates/pc1600.png")
                        : model.value == "PC-1500" ? QStringLiteral(":/faceplates/pc1500.png")
                                                   : QStringLiteral(":/faceplates/pc1500A.png");
    if (property("art").toString() == art) return;
    setProperty("art", art);
    m_pixmap.load(art);
    m_layout = layout;
    m_lcd->setModel(model);
    m_held.clear();
    m_flashed.clear();
    m_pressX = m_pressY = -1;
    updateGeometry();
    relayout();
    update();
}

void FaceplateWidget::setHeldKeys(std::vector<std::string> keys) {
    if (keys == m_held) return;
    m_held = std::move(keys);
    update();
}

void FaceplateWidget::flashKey(const std::string& name) {
    m_flashed.push_back(name);
    update();
    QTimer::singleShot(kFlashMs, this, [this, name] {
        const auto it = std::find(m_flashed.begin(), m_flashed.end(), name);
        if (it != m_flashed.end()) m_flashed.erase(it);
        update();
    });
}

std::vector<std::string> FaceplateWidget::shownKeys() const {
    std::vector<std::string> shown = m_held;
    for (const std::string& k : m_flashed)
        if (std::find(shown.begin(), shown.end(), k) == shown.end()) shown.push_back(k);
    return shown;
}

int FaceplateWidget::heightForWidth(int w) const {
    return m_layout ? KeyLayout::heightForWidth(*m_layout, w) : w;
}

QSize FaceplateWidget::sizeHint() const {
    const int w = 800;
    return QSize(w, heightForWidth(w));
}

double FaceplateWidget::artScale() const {
    if (!m_layout || !m_fitted.art.valid() || m_layout->imageWidth <= 0) return 1.0;
    return m_fitted.art.w / m_layout->imageWidth;
}

QPointF FaceplateWidget::artOrigin() const {
    return QPointF(m_fitted.art.x, m_fitted.art.y);
}

QPoint FaceplateWidget::keyCentre(const QString& name) const {
    if (!m_layout) return QPoint(-1, -1);
    const int i = KeyLayout::keyIndex(*m_layout, name.toUtf8().constData());
    if (i < 0 || i >= static_cast<int>(m_fitted.keys.size())) return QPoint(-1, -1);
    return toQRect(m_fitted.keys[static_cast<std::size_t>(i)]).center();
}

bool FaceplateWidget::notePress(double artX, double artY) {
    m_pressX = artX;
    m_pressY = artY;
    return false;
}

void FaceplateWidget::relayout() {
    m_fitted = m_layout ? KeyLayout::fitLayout(*m_layout, width(), height()) : KeyLayout::FittedLayout{};
    if (m_fitted.art.valid()) m_lcd->setGeometry(toQRect(m_fitted.lcd));
}

void FaceplateWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (!m_pixmap.isNull() && m_fitted.art.valid()) {
        // Without this, scaling the 4000px source art down to the widget's
        // size uses a fast/non-smooth transform -- most visible on
        // non-integer display scale factors (e.g. Windows' common 125%/
        // 150%), where it reads noticeably softer/rougher than a smooth
        // scale would.
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap(toQRect(m_fitted.art), m_pixmap);
    }
    // The keys live input holds: the chrome's overlay on each one's own area.
    if (!m_layout) return;
    const QColor held = calcu::qt6::chrome::qcolour(cu1600::sharpChrome().light.named(cu1600::chrome::kKeyPressed));
    for (const std::string& k : shownKeys()) {
        const int i = KeyLayout::heldKeyIndex(*m_layout, k.c_str(), m_pressX, m_pressY);
        if (i >= 0 && i < static_cast<int>(m_fitted.keys.size()))
            painter.fillRect(toQRect(m_fitted.keys[static_cast<std::size_t>(i)]), held);
    }
}

void FaceplateWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}
