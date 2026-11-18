#pragma once
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QWidget>
#include <string>
#include <vector>

#include "app/KeyLayout/FaceplateLayout.hpp"
#include "machine/Ids.hpp"

class LcdWidget;

// Faceplate art + embedded LCD: a resizable widget that keeps the body
// art's aspect ratio (heightForWidth) and lets its child LcdWidget track
// every resize. The pointer on it is the shell's (Shell::setFaceplate with the hit map of app/SharpShellProduct and
// artScale()); it draws the keys live input holds pressed (the
// `held-keys` topic).
class FaceplateWidget : public QWidget {
    Q_OBJECT
public:
    explicit FaceplateWidget(QWidget* parent = nullptr);

    void setModel(const calcu::machine::ModelId& model);
    void setHeldKeys(std::vector<std::string> keys);
    // Draws `name` pressed for a moment: a press too short to show in the
    // held keys (the host's SHIFT tap is down and up at once).
    void flashKey(const std::string& name);
    // The keys drawn pressed now: the held ones and the flashed ones.
    std::vector<std::string> shownKeys() const;

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override;
    QSize sizeHint() const override;

    LcdWidget* lcdWidget() const { return m_lcd; }
    // Logical pixels per pixel of the art as shown now (the hit map's scale).
    double artScale() const;
    // Where the art's (0, 0) lies in the widget: the art is letterboxed
    // (centred, aspect kept), so the hit map starts there, not at the corner.
    QPointF artOrigin() const;
    // The centre of the key `name` in widget pixels; (-1, -1) if the model
    // has no such key (the scenarios' `hold-key:`).
    QPoint keyCentre(const QString& name) const;
    // The shell's productFirst hook: notes where the pointer
    // went down, in art pixels, so a held key that has several rectangles
    // (the PC-1600's two SHIFT keys) is drawn on the one pressed. Never
    // takes the press.
    bool notePress(double artX, double artY);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    const KeyLayout::Layout* m_layout = nullptr;
    QPixmap m_pixmap;
    KeyLayout::FittedLayout m_fitted;   // art, key and LCD rects for the current size, widget-local
    LcdWidget* m_lcd = nullptr;
    std::vector<std::string> m_held;
    std::vector<std::string> m_flashed;   // flashKey's, each for kFlashMs
    double m_pressX = -1, m_pressY = -1;   // the last pointer press, art pixels

    void relayout();
};
