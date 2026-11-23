#pragma once
#include "machine/Ids.hpp"
#include "paper/PaperTopics.hpp"
#include "peripherals/Controls.hpp"
#include "sessions/Messages.hpp"
#include "shell/Chrome.hpp"
#include <QAbstractScrollArea>
#include <QColor>
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
// scroll bar, or the text in a fixed-width font; below the paper one row
// that never wraps: the owner's controls left (a switch with the dot of its
// position), labelled as the control state labels them (the short labels
// where shell::paperRowCompact says so for the panel's width), and Copy
// and Cut right as icons (tooltips and accessible names "Copy", "Cut").
// The product's chrome (setChrome) gives the header and the button bar
// their heights and look.
// A Copy or Cut is answered by a toast inside the panel for 1.8 s.
//
// Object names: paper-panel, paper-header, paper-footer, paper-owner,
// paper-view-toggle, paper-copy, paper-cut, paper-control-<id>, paper-view,
// paper-text, paper-toast.

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
    bool compact() const { return compact_; }   // the row's short labels and gap are on
    // The product's chrome (shell/Chrome): the header and the button bar
    // line up with the panels beside it; the paper's colours.
    void setChrome(const shell::ChromeColours&, const shell::ChromeSizes&);
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

protected:
    void resizeEvent(QResizeEvent*) override;

private:
    void applyCompact();
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

    QWidget* header_;
    QWidget* footer_;
    QColor plugged_, unplugged_;   // the chrome's (setChrome)
    shell::ChromeSizes sizes_;     // the chrome's (setChrome); the library's until then
    bool compact_ = false;
    QHBoxLayout* footerLayout_;
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
    QColor paperColour_;   // the chrome's paper; until it came, the palette's base
};

} // namespace calcu::qt6
