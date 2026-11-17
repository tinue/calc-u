#pragma once
#include "KeyboardLayout.hpp"
#include "LedGeometry.hpp"
#include "qt6/HostKeys.hpp"
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QWidget>

class EmulatorController;

// The calculator canvas, the faceplate: base artwork (2064×4075) with the LED
// display and cue card painted over it, the pressed-key effect drawn from
// live input (`held-keys`, LIB-5 AC-19), and the physical keyboard as the
// library's host keys (calcu::qt6::HostKeys; the product's keyboard map,
// app/TITyping, decides; LIB-5 AC-7, D-40). The pointer on it is the shell's
// (LIB-14 AC-24, AC-43; Shell::setFaceplate with the hit map of
// app/TIShellProduct, scale() and origin() below); a press on a fitted card
// on show is the product's (cardTakes(), C59-7's card tap; D-44). The
// counterpart of App/Views/KeyboardView.swift.
class KeyboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit KeyboardWidget(EmulatorController& controller, QWidget* parent = nullptr);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override;
    QSize sizeHint() const override;

    /// The canvas drawn in the widget (the artwork's aspect, centred).
    QRectF canvasRect() const;
    /// Logical pixels per base-image pixel, and the canvas's corner: the hit map's geometry.
    double scale() const;
    QPointF origin() const { return canvasRect().topLeft(); }
    /// A press at (x, y) in base-image pixels that the product takes first: on
    /// the card window while a fitted card is on show (C59-7 AC-12, D-12).
    bool cardTakes(double x, double y) const;

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;   // only the presses cardTakes() let through
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private:
    EmulatorController& m_controller;
    QPixmap m_base;
    QPixmap m_scaled;        ///< m_base at the canvas's device size
    cu59::LedStyle m_ledStyle = cu59::LedStyle::Modernized;
    calcu::qt6::HostKeys m_hostKeys;
};
