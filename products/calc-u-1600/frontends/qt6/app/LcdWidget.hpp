#pragma once
#include <QColor>
#include <QWidget>

#include "Display/DisplayOutputText.hpp"
#include "machine/Display.hpp"
#include "machine/Ids.hpp"

// Dot-matrix LCD + status-indicator renderer of the `display` topic's output
// (no frame type of its own). PC-1500/1500A: an
// indicator bar above the dot matrix, both sized off the panel's own height.
// PC-1600: a fixed-legend status strip above a 156x32 graphics area, both
// positioned as fractions of the panel's own width/height (app/LcdLayout).
// Blanks entirely while the output is off rather than freeze-framing the
// last image.
//
// It takes no mouse input: the pointer on the display (full speed while
// held) is the shell's FaceplateInput on the faceplate.
class LcdWidget : public QWidget {
    Q_OBJECT
public:
    explicit LcdWidget(QWidget* parent = nullptr);

    void setModel(const calcu::machine::ModelId& model);
    void setOutput(const calcu::machine::DisplayOutput& output);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    calcu::machine::ModelId m_model{"PC-1500A"};
    calcu::machine::DisplayOutput m_output;
    LcdBitmap m_bitmap;

    void paintPC1500(class QPainter& painter);
    void paintPC1600(class QPainter& painter);
    void paintDotMatrix(class QPainter& painter, const QRectF& area, const QColor& ink);
};
