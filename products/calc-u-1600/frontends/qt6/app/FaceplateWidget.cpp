#include "FaceplateWidget.hpp"
#include "LcdWidget.hpp"

#include <QPainter>
#include <QResizeEvent>
#include <algorithm>

namespace {
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
    updateGeometry();
    relayout();
    update();
}

void FaceplateWidget::setHeldKeys(std::vector<std::string> keys) {
    if (keys == m_held) return;
    m_held = std::move(keys);
    update();
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

QPoint FaceplateWidget::keyCentre(const QString& name) const {
    if (!m_layout) return QPoint(-1, -1);
    const int i = KeyLayout::keyIndex(*m_layout, name.toUtf8().constData());
    if (i < 0 || i >= static_cast<int>(m_fitted.keys.size())) return QPoint(-1, -1);
    return toQRect(m_fitted.keys[static_cast<std::size_t>(i)]).center();
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
    // The keys live input holds (LIB-5 AC-19): a translucent white overlay
    // on each one's own area.
    if (!m_layout) return;
    for (const std::string& k : m_held) {
        const int i = KeyLayout::keyIndex(*m_layout, k.c_str());
        if (i >= 0 && i < static_cast<int>(m_fitted.keys.size()))
            painter.fillRect(toQRect(m_fitted.keys[static_cast<std::size_t>(i)]), QColor(255, 255, 255, 64));
    }
}

void FaceplateWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}
