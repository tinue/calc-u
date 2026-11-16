#pragma once
#include <QPixmap>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <string>
#include <vector>

#include "app/KeyLayout/FaceplateLayout.hpp"
#include "machine/Ids.hpp"

class LcdWidget;

// Faceplate art + embedded LCD: a resizable widget that keeps the body
// art's aspect ratio (heightForWidth) and lets its child LcdWidget track
// every resize. The pointer on it is the shell's (LIB-14 AC-24, AC-43:
// Shell::setFaceplate with the hit map of app/SharpShellProduct and
// artScale()); it draws the keys live input holds pressed (LIB-5 AC-19,
// the `held-keys` topic).
class FaceplateWidget : public QWidget {
    Q_OBJECT
public:
    explicit FaceplateWidget(QWidget* parent = nullptr);

    void setModel(const calcu::machine::ModelId& model);
    void setHeldKeys(std::vector<std::string> keys);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override;
    QSize sizeHint() const override;

    LcdWidget* lcdWidget() const { return m_lcd; }
    // Logical pixels per pixel of the art as shown now (the hit map's scale).
    double artScale() const;
    // The centre of the key `name` in widget pixels; (-1, -1) if the model
    // has no such key (the scenarios' `hold-key:`).
    QPoint keyCentre(const QString& name) const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    const KeyLayout::Layout* m_layout = nullptr;
    QPixmap m_pixmap;
    KeyLayout::FittedLayout m_fitted;   // art, key and LCD rects for the current size, widget-local
    LcdWidget* m_lcd = nullptr;
    std::vector<std::string> m_held;

    void relayout();
};
