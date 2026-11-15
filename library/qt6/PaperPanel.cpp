#include "qt6/PaperPanel.hpp"
#include "qt6/PaperPainter.hpp"
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QScroller>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace calcu::qt6 {

using namespace calcu::paper;
using sessions::CommandName;
using sessions::Value;

namespace {
const CommandName kSetControl{"set-control"};   // LIB-2's PeripheralsPart (LIB-6 D-16)
const std::string kStreamPrefix = "paper ";
} // namespace

// ── PaperPanel ───────────────────────────────────────────────────────────

PaperPanel::PaperPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("paper-panel"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(2);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(4, 2, 4, 2);
    ownerLabel_ = new QLabel(this);
    ownerLabel_->setObjectName(QStringLiteral("paper-owner"));
    header->addWidget(ownerLabel_);
    viewToggle_ = new QToolButton(this);
    viewToggle_->setObjectName(QStringLiteral("paper-view-toggle"));
    viewToggle_->setCheckable(true);
    viewToggle_->setToolTip(tr("Switches between the marks and the text"));
    header->addWidget(viewToggle_);
    controlsBox_ = new QWidget(this);
    controlsLayout_ = new QHBoxLayout(controlsBox_);
    controlsLayout_->setContentsMargins(0, 0, 0, 0);
    header->addWidget(controlsBox_);
    header->addStretch(1);
    copy_ = new QToolButton(this);
    copy_->setObjectName(QStringLiteral("paper-copy"));
    copy_->setText(tr("Copy"));
    header->addWidget(copy_);
    cut_ = new QToolButton(this);
    cut_->setObjectName(QStringLiteral("paper-cut"));
    cut_->setText(tr("Cut"));
    header->addWidget(cut_);
    outer->addLayout(header);

    stack_ = new QStackedWidget(this);
    view_ = new PaperView(stack_);
    text_ = new QPlainTextEdit(stack_);
    text_->setObjectName(QStringLiteral("paper-text"));
    text_->setReadOnly(true);
    text_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setStyleHint(QFont::TypeWriter);
    fixed.setFixedPitch(true);
    text_->setFont(fixed);
    text_->setLineWrapMode(QPlainTextEdit::NoWrap);
    stack_->addWidget(view_);
    stack_->addWidget(text_);
    outer->addWidget(stack_, 1);

    toast_ = new QLabel(this);
    toast_->setObjectName(QStringLiteral("paper-toast"));
    toast_->setAlignment(Qt::AlignCenter);
    toast_->setAutoFillBackground(true);
    toast_->hide();
    toastTimer_ = new QTimer(this);
    toastTimer_->setObjectName(QStringLiteral("paper-toast-timer"));
    toastTimer_->setSingleShot(true);
    toastTimer_->setInterval(kToastMilliseconds);
    connect(toastTimer_, &QTimer::timeout, toast_, &QWidget::hide);

    connect(viewToggle_, &QToolButton::toggled, this, [this](bool on) { showText(on); });
    connect(copy_, &QToolButton::clicked, this, [this] {
        send(paper::kPaperCopyCommand, Value::object({{"owner", ownerName()}}));
    });
    connect(cut_, &QToolButton::clicked, this, [this] {
        send(paper::kPaperCutCommand, Value::object({{"owner", ownerName()}, {"copy", true}}));
    });
    refresh();
}

PaperPanel::~PaperPanel() = default;

void PaperPanel::send(const CommandName& name, Value args) {
    if (sender_) sender_(name, std::move(args));
}

std::string PaperPanel::ownerName() const {
    if (!owner_) return {};
    for (const auto& e : entries_)
        if (e.printer == *owner_) return e.owner;
    return {};
}

const PaperPanel::Shown* PaperPanel::shown() const {
    const std::string name = ownerName();
    if (name.empty()) return nullptr;
    auto it = papers_.find(name);
    return it == papers_.end() ? nullptr : &it->second;
}

bool PaperPanel::showsText() const { return stack_->currentWidget() == text_; }

void PaperPanel::showText(bool on) {
    const Shown* s = shown();
    const bool text = on && s && s->description.textLayer;
    stack_->setCurrentWidget(text ? static_cast<QWidget*>(text_) : view_);
    // AC-14: the control shows the view that is on.
    viewToggle_->blockSignals(true);
    viewToggle_->setChecked(text);
    viewToggle_->blockSignals(false);
    viewToggle_->setText(text ? tr("Text") : tr("Marks"));
}

void PaperPanel::deliver(const sessions::Message& m) {
    if (const auto* st = std::get_if<sessions::StateMessage>(&m)) {
        if (st->topic != paper::kPapersTopic) return;
        if (const auto* list = sessions::payloadAs<PaperList>(st->state)) {
            entries_ = list->papers;
            for (const auto& e : entries_) papers_[e.owner].description = e.description;
            refresh();
        }
        return;
    }
    const auto* sm = std::get_if<sessions::StreamMessage>(&m);
    if (!sm) return;
    if (sm->topic == paper::kPaperCopiesTopic) {
        for (const auto& item : sm->appended)
            if (const auto* o = sessions::payloadAs<CopyOutcome>(item); o && o->owner == ownerName())
                toast(o->outcome == CopyOutcome::Kind::Failed ? tr("Copy failed") : tr("Copied to clipboard"));
        return;
    }
    const std::string& topic = sm->topic.value;
    if (topic.compare(0, kStreamPrefix.size(), kStreamPrefix) != 0) return;
    const std::string owner = topic.substr(kStreamPrefix.size());
    Shown& p = papers_[owner];
    if (sm->reset) {
        p.marks.clear();
        p.lines.clear();
        p.reach = {};
        if (owner == ownerName()) {
            textLinesShown_ = 0;
            text_->clear();
        }
    }
    for (const auto& item : sm->appended)
        if (const auto* a = sessions::payloadAs<PaperAppended>(item)) apply(owner, *a);
    if (owner == ownerName()) {
        view_->setPaper(&p);
        refreshText();
        const bool enabled = !p.empty();
        copy_->setEnabled(enabled);
        cut_->setEnabled(enabled);
    }
}

void PaperPanel::apply(const std::string& owner, const PaperAppended& a) {
    Shown& p = papers_[owner];
    auto first = a.marks.begin();
    if (a.continuesStroke && first != a.marks.end() && !p.marks.empty()) {
        if (auto* last = std::get_if<Stroke>(&p.marks.back()); last && std::holds_alternative<Stroke>(*first)) {
            const auto& more = std::get<Stroke>(*first).path;
            last->path.insert(last->path.end(), more.begin(), more.end());
            ++first;
        }
    }
    p.marks.insert(p.marks.end(), first, a.marks.end());
    p.lines.insert(p.lines.end(), a.lines.begin(), a.lines.end());
    p.reach = a.reach;
}

void PaperPanel::setOwner(std::optional<machine::PeripheralId> owner) {
    if (owner == owner_) return;
    owner_ = std::move(owner);
    textLinesShown_ = 0;
    text_->clear();
    refresh();
}

void PaperPanel::setControls(std::vector<peripherals::ControlInfo> controls) {
    const bool same = controls.size() == controls_.size() &&
                      std::equal(controls.begin(), controls.end(), controls_.begin(),
                                 [](const auto& a, const auto& b) { return a.id == b.id && a.form == b.form; });
    controls_ = std::move(controls);
    if (!same) rebuildControls();
    // The state of each control: a key down while held, a switch at its position (AC-15).
    for (const auto& c : controls_) {
        auto* b = controlsBox_->findChild<QToolButton*>(QString::fromStdString("paper-control-" + c.id.value));
        if (!b) continue;
        if (c.form == machine::ControlForm::Switch) {
            b->blockSignals(true);
            b->setChecked(c.on);
            b->blockSignals(false);
        } else {
            b->setDown(c.on);
        }
    }
}

void PaperPanel::rebuildControls() {
    for (QToolButton* b : controlsBox_->findChildren<QToolButton*>()) delete b;
    for (const auto& c : controls_) {
        auto* b = new QToolButton(controlsBox_);
        const std::string id = c.id.value;
        b->setObjectName(QString::fromStdString("paper-control-" + id));
        b->setText(QString::fromStdString(id));
        const auto args = [this, id](bool on) {
            return Value::object({{"peripheral", owner_ ? owner_->value : std::string()}, {"control", id}, {"on", on}});
        };
        if (c.form == machine::ControlForm::Switch) {
            b->setCheckable(true);
            connect(b, &QToolButton::toggled, this, [this, args](bool on) { send(kSetControl, args(on)); });
        } else {
            connect(b, &QToolButton::pressed, this, [this, args] { send(kSetControl, args(true)); });
            connect(b, &QToolButton::released, this, [this, args] { send(kSetControl, args(false)); });
        }
        controlsLayout_->addWidget(b);
    }
}

void PaperPanel::refresh() {
    const Shown* s = shown();
    ownerLabel_->setText(QString::fromStdString(ownerName()));   // AC-12: no name without an owner
    viewToggle_->setVisible(s && s->description.textLayer);
    controlsBox_->setVisible(owner_.has_value());
    const bool enabled = s && !s->empty();   // AC-12, AC-19
    copy_->setEnabled(enabled);
    cut_->setEnabled(enabled);
    view_->setPaper(s);
    showText(showsText());
    refreshText();
}

void PaperPanel::refreshText() {
    const Shown* s = shown();
    if (!s) {
        text_->clear();
        textLinesShown_ = 0;
        return;
    }
    if (textLinesShown_ > s->lines.size()) {
        text_->clear();
        textLinesShown_ = 0;
    }
    for (; textLinesShown_ < s->lines.size(); ++textLinesShown_)
        text_->appendPlainText(QString::fromStdString(s->lines[textLinesShown_].text));
}

void PaperPanel::toast(const QString& sentence) {
    toast_->setText(sentence);
    toast_->adjustSize();
    toast_->resize(std::max(toast_->width() + 16, 160), toast_->height() + 8);
    toast_->move((width() - toast_->width()) / 2, std::max(0, (height() - toast_->height()) / 2));
    toast_->raise();
    toast_->show();
    toastTimer_->start();
}

// ── PaperView ────────────────────────────────────────────────────────────

PaperView::PaperView(QWidget* parent) : QAbstractScrollArea(parent) {
    setObjectName(QStringLiteral("paper-view"));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // AC-13: no scroll bar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
    verticalScrollBar()->setSingleStep(16);
}

double PaperView::pointsPerCoord() const {
    if (!paper_ || paper_->description.width <= 0) return 0;
    return static_cast<double>(viewport()->width()) / static_cast<double>(paper_->description.width);
}

int PaperView::scrollPosition() const { return verticalScrollBar()->value(); }
int PaperView::scrollEnd() const { return verticalScrollBar()->maximum(); }

void PaperView::setPaper(const PaperPanel::Shown* p) {
    paper_ = p;
    updateRange(true);
    viewport()->update();
}

void PaperView::resizeEvent(QResizeEvent* e) {
    QAbstractScrollArea::resizeEvent(e);
    updateRange(true);
}

// AC-13: the view follows the newest marks while it is at the end or no more
// than 64 points from it; scrolled further back, it stays.
void PaperView::updateRange(bool follow) {
    QScrollBar* bar = verticalScrollBar();
    const bool atEnd = bar->maximum() - bar->value() <= PaperPanel::kFollowPoints;
    double content = 0;
    if (paper_) {
        const PaperExtent e = paperExtent(paper_->description, paper_->marks, paper_->reach);
        top_ = e.top;
        bottom_ = e.bottom;
        content = static_cast<double>(bottom_ - top_) * pointsPerCoord();
    }
    const int total = static_cast<int>(std::ceil(content)) + 2 * PaperPanel::kBlankPoints;
    bar->setRange(0, std::max(0, total - viewport()->height()));
    bar->setPageStep(viewport()->height());
    if (follow && atEnd) bar->setValue(bar->maximum());
}

void PaperView::paintEvent(QPaintEvent*) {
    QPainter p(viewport());
    p.fillRect(viewport()->rect(), Qt::white);
    if (!paper_ || paper_->description.width <= 0) return;
    const double k = pointsPerCoord();
    if (k <= 0) return;
    PaperScale s;
    s.pointsPerCoord = k;
    s.top = top_;
    s.originY = PaperPanel::kBlankPoints - verticalScrollBar()->value();
    const auto toCoord = [&](double y) { return top_ + static_cast<Coord>((y - s.originY) / k); };
    paintMarks(p, paper_->description, paper_->marks, s, toCoord(0) - 1, toCoord(viewport()->height()) + 1);
}

} // namespace calcu::qt6
