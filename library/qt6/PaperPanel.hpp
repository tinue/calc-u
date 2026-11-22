#pragma once
#include "machine/Ids.hpp"
#include "paper/PaperTopics.hpp"
#include "peripherals/Controls.hpp"
#include "sessions/Messages.hpp"
#include <QAbstractScrollArea>
#include <QColor>
#include <QIcon>
#include <QWidget>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class QLabel;
class QPlainTextEdit;
class QStackedWidget;
class QTimer;
class QToolButton;
class QHBoxLayout;

// The paper panel of the Qt6 component set. Built from the session's topics
// only; it names no printer:
// - deliver(): the `papers` state, the `paper <owner>` streams (every paper
//   is kept here; the one shown follows the owner) and `paper-copies`;
// - setOwner(): the printer plugged in, from the control state, handed
//   in by the shell, or none; setControls(): its controls with their state;
// - setSender(): where its commands go: `paper.copy {owner}`, `paper.cut
//   {owner, copy: true}`, `set-control {peripheral, control, on}`.
// The header shows the shell's place controls on the paper (setPlaces)
// left, the owner's name centred and the view control (with a text layer)
// right; below it the paper, scrolled with the wheel or by touch without a
// scroll bar, or the text in a fixed-width font; below the paper the
// owner's controls left, Copy and Cut right.
// A Copy or Cut is answered by a toast inside the panel for 1.8 s.
//
// Object names: paper-panel, paper-owner, paper-view-toggle, paper-copy,
// paper-cut, paper-control-<id>, paper-view, paper-text, paper-toast.

namespace calcu::qt6 {

class PaperView;

class PaperPanel : public QWidget {
    Q_OBJECT
public:
    using Sender = std::function<void(const sessions::CommandName&, sessions::Value)>;
    static constexpr int kToastMilliseconds = 1800;
    static constexpr int kFollowPoints = 64;
    static constexpr int kBlankPoints = 8;            // above and below the paper

    explicit PaperPanel(QWidget* parent = nullptr);
    ~PaperPanel() override;

    void deliver(const sessions::Message&);
    void setOwner(std::optional<machine::PeripheralId>);
    void setControls(std::vector<peripherals::ControlInfo>);
    // The shell's place controls on the paper, first in the header (the
    // panel area makes them); a button shows the printer icon where the
    // platform has one, its name as the tooltip.
    void setPlaces(std::vector<QWidget*>);
    static QIcon placeIcon(const QIcon& printer, bool plugged);
    void setSender(Sender s) { sender_ = std::move(s); }

    // What the panel shows now (the tests, the shell).
    std::string ownerName() const;               // "" without an owner
    bool showsText() const;                      // the text view is on
    void showText(bool on);                      // as the view control does
    bool paperEmpty() const;                     // the paper shown has nothing on it (or there is none)
    PaperView* paperView() const { return view_; }
    // The paper's colour in both views (white by default); the text view
    // prints in near-black on it, whatever the window's palette.
    void setPaperColour(QColor);

    struct Shown {
        paper::PaperDescription description;
        std::vector<paper::Mark> marks;
        std::vector<paper::TextLine> lines;
        paper::Reach reach;
        bool empty() const { return marks.empty() && lines.empty() && reach == paper::Reach{}; }
    };

private:
    void apply(const std::string& owner, const paper::PaperAppended&);
    void refresh();
    void refreshText();
    void rebuildControls();
    void send(const sessions::CommandName&, sessions::Value);
    void toast(const QString&);
    const Shown* shown() const;

    Sender sender_;
    std::vector<paper::PaperEntry> entries_;
    std::map<std::string, Shown> papers_;   // by owner's name
    std::optional<machine::PeripheralId> owner_;
    std::vector<peripherals::ControlInfo> controls_;

    QHBoxLayout* placesLayout_;
    QLabel* ownerLabel_;
    QToolButton* viewToggle_;
    QWidget* controlsBox_;
    QHBoxLayout* controlsLayout_;
    QToolButton* copy_;
    QToolButton* cut_;
    QStackedWidget* stack_;
    PaperView* view_;
    QPlainTextEdit* text_;
    QLabel* toast_;
    QTimer* toastTimer_;
    std::size_t textLinesShown_ = 0;
};

// The scrolling paper: the whole width scaled to the viewport's width; only
// the marks in view are drawn.
class PaperView : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit PaperView(QWidget* parent = nullptr);
    void setPaper(const PaperPanel::Shown*);   // nullptr: an empty paper without a description
    void setPaperColour(QColor c) { paperColour_ = c; viewport()->update(); }
    int scrollPosition() const;
    int scrollEnd() const;
    double pointsPerCoord() const;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void updateRange(bool follow);
    const PaperPanel::Shown* paper_ = nullptr;
    paper::Coord top_ = 0, bottom_ = 0;
    QColor paperColour_ = Qt::white;
};

} // namespace calcu::qt6
