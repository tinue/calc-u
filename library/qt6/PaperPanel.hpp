#pragma once
#include "machine/Ids.hpp"
#include "paper/PaperTopics.hpp"
#include "peripherals/Controls.hpp"
#include "sessions/Messages.hpp"
#include <QAbstractScrollArea>
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

// The paper panel of the Qt6 component set (LIB-8 design §3.7; spec AC-12 to
// AC-15, AC-19, AC-20). Built from the session's topics only; it names no
// printer (capabilities.md I-1):
// - deliver(): the `papers` state, the `paper <owner>` streams (every paper
//   is kept here; the one shown follows the owner) and `paper-copies`;
// - setOwner(): the printer plugged in, from LIB-4's control state (A-4),
//   handed in by the shell, or none; setControls(): its controls (LIB-2
//   AC-14) with their state;
// - setSender(): where its commands go: `paper.copy {owner}`, `paper.cut
//   {owner, copy: true}`, `set-control {peripheral, control, on}` (A-7).
// The header shows the owner's name, the view control (with a text layer),
// the owner's controls, Copy and Cut; below it the paper, scrolled with the
// wheel or by touch without a scroll bar, or the text in a fixed-width font.
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
    static constexpr int kToastMilliseconds = 1800;   // AC-20
    static constexpr int kFollowPoints = 64;          // AC-13
    static constexpr int kBlankPoints = 8;            // above and below the paper

    explicit PaperPanel(QWidget* parent = nullptr);
    ~PaperPanel() override;

    void deliver(const sessions::Message&);
    void setOwner(std::optional<machine::PeripheralId>);
    void setControls(std::vector<peripherals::ControlInfo>);
    void setSender(Sender s) { sender_ = std::move(s); }

    // What the panel shows now (the tests, the shell).
    std::string ownerName() const;               // "" without an owner
    bool showsText() const;                      // the text view is on
    void showText(bool on);                      // as the view control does
    bool paperEmpty() const;                     // the paper shown has nothing on it (or there is none)
    PaperView* paperView() const { return view_; }

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
// the marks in view are drawn (AC-12, AC-13).
class PaperView : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit PaperView(QWidget* parent = nullptr);
    void setPaper(const PaperPanel::Shown*);   // nullptr: an empty paper without a description
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
};

} // namespace calcu::qt6
