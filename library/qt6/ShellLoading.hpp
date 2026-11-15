#pragma once
#include <QWidget>
#include <optional>

class QLabel;
class QProgressBar;

// "Loading…" of the Qt6 shell (LIB-14 design §3.6 L2, D-6; spec AC-15,
// AC-16): an overlay on the faceplate below its display, "Loading…" with a
// busy sign, no button, no title, transparent to input, so that the menus,
// the bar and the keys stay usable. Object name `loading`.

namespace calcu::qt6 {

class ShellLoading : public QWidget {
    Q_OBJECT
public:
    explicit ShellLoading(QWidget* over);
    // Where it stands: below `display` (in the parent's pixels), else centred.
    void place(std::optional<QRect> display);

protected:
    bool eventFilter(QObject*, QEvent*) override;   // follows the parent's size

private:
    std::optional<QRect> display_;
    QLabel* text_;
    QProgressBar* busy_;
};

} // namespace calcu::qt6
