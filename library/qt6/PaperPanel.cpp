#include "qt6/PaperPanel.hpp"
#include "qt6/ChromeStyle.hpp"
#include "qt6/PaperPainter.hpp"
#include <QApplication>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
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
const CommandName kSetControl{"set-control"};   // the peripherals' PeripheralsPart
const std::string kStreamPrefix = "paper ";

// The dot of a place button and of a switch: the chrome's plugged colour
// while on, its unplugged one while off.
constexpr qreal kDotPoints = 8;

void drawDot(QPainter& p, QPointF topLeft, const QColor& dot) {
    p.setPen(Qt::NoPen);
    p.setBrush(dot);
    p.drawEllipse(QRectF(topLeft, QSizeF(kDotPoints, kDotPoints)));
}

QPixmap transparentPixmap(QSize points) {
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap pm(points * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    return pm;
}

QIcon switchDot(const QColor& dot) {
    QPixmap pm = transparentPixmap(QSize(kDotPoints, kDotPoints));
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    drawDot(p, QPointF(0, 0), dot);
    return QIcon(pm);
}

// A place button's icon: the printer (where the platform has one) and the dot.
QIcon placeIcon(const QIcon& printer, const QColor& dot) {
    QPixmap pm = transparentPixmap(QSize(34, 16));
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    if (!printer.isNull()) printer.paint(&p, QRect(0, 0, 16, 16));
    drawDot(p, QPointF(printer.isNull() ? 2 : 22, 4), dot);
    return QIcon(pm);
}

enum class Action { Copy, Cut };
constexpr int kActionIconPoints = 16;

// Copy (two sheets, one over the other) and Cut (scissors) in the pill's
// text colour, and in its disabled text colour while disabled.
QIcon actionIcon(Action a, const QColor& on, const QColor& off) {
    const auto draw = [&](const QColor& colour) {
        QPixmap pm = transparentPixmap(QSize(kActionIconPoints, kActionIconPoints));
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(kActionIconPoints / 16.0, kActionIconPoints / 16.0);   // drawn on a 16-point grid
        QPen pen(colour, 1.4);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (a == Action::Copy) {
            p.drawRoundedRect(QRectF(5.5, 1.5, 8, 10), 1.5, 1.5);
            // The sheet below shows only outside the one above.
            QPainterPath back;
            back.moveTo(5.5, 4.5);
            back.lineTo(2.5, 4.5);
            back.lineTo(2.5, 14.5);
            back.lineTo(10.5, 14.5);
            back.lineTo(10.5, 11.5);
            p.drawPath(back);
        } else {
            p.drawEllipse(QRectF(2, 10, 4, 4));
            p.drawEllipse(QRectF(10, 10, 4, 4));
            p.drawLine(QPointF(5.4, 10.6), QPointF(11.5, 1.5));
            p.drawLine(QPointF(10.6, 10.6), QPointF(4.5, 1.5));
        }
        return pm;
    };
    QIcon icon;
    icon.addPixmap(draw(on), QIcon::Normal);
    icon.addPixmap(draw(off), QIcon::Disabled);
    return icon;
}

// The platform's printer icon; Qt before 6.7 (Ubuntu 24.04's 6.4) knows
// only the theme's name for it.
QIcon printerIcon() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    return QIcon::fromTheme(QIcon::ThemeIcon::DocumentPrint);
#else
    return QIcon::fromTheme(QStringLiteral("document-print"));
#endif
}

} // namespace

// ── PaperPanel ───────────────────────────────────────────────────────────

PaperPanel::PaperPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("paper-panel"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // The header: the place controls left, the owner's name centred, the
    // view control right, each in its own column so none moves another.
    header_ = new QWidget(this);
    header_->setObjectName(QStringLiteral("paper-header"));
    header_->setAttribute(Qt::WA_StyledBackground, true);
    auto* header = new QGridLayout(header_);
    header->setContentsMargins(6, 0, 6, 0);
    placesLayout_ = new QHBoxLayout;
    placesLayout_->setContentsMargins(0, 0, 0, 0);
    header->addLayout(placesLayout_, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    ownerLabel_ = new QLabel(this);
    ownerLabel_->setObjectName(QStringLiteral("paper-owner"));
    header->addWidget(ownerLabel_, 0, 1, Qt::AlignCenter);
    viewToggle_ = new QToolButton(this);
    viewToggle_->setFocusPolicy(Qt::NoFocus);
    viewToggle_->setObjectName(QStringLiteral("paper-view-toggle"));
    viewToggle_->setCheckable(true);
    viewToggle_->setToolTip(tr("Switches between the marks and the text"));
    header->addWidget(viewToggle_, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
    header->setColumnStretch(0, 1);
    header->setColumnStretch(2, 1);
    outer->addWidget(header_);

    stack_ = new QStackedWidget(this);
    view_ = new PaperView(stack_);
    text_ = new QPlainTextEdit(stack_);
    text_->setObjectName(QStringLiteral("paper-text"));
    text_->setReadOnly(true);
    text_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    text_->setFocusPolicy(Qt::NoFocus);
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setStyleHint(QFont::TypeWriter);
    fixed.setFixedPitch(true);
    text_->setFont(fixed);
    text_->setLineWrapMode(QPlainTextEdit::NoWrap);
    stack_->addWidget(view_);
    stack_->addWidget(text_);
    outer->addWidget(stack_, 1);

    // Below the paper, in one row that never wraps: the owner's controls
    // left, Copy and Cut right as icons.
    footer_ = new QWidget(this);
    footer_->setObjectName(QStringLiteral("paper-footer"));
    footer_->setAttribute(Qt::WA_StyledBackground, true);
    footerLayout_ = new QHBoxLayout(footer_);
    controlsBox_ = new QWidget(this);
    controlsLayout_ = new QHBoxLayout(controlsBox_);
    controlsLayout_->setContentsMargins(0, 0, 0, 0);
    footerLayout_->addWidget(controlsBox_);
    footerLayout_->addStretch(1);
    const auto action = [this](const char* name, const QString& label) {
        auto* b = new QToolButton(this);
        b->setFocusPolicy(Qt::NoFocus);
        b->setObjectName(QString::fromLatin1(name));
        b->setText(label);
        b->setToolTip(label);
        b->setAccessibleName(label);
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setIconSize(QSize(kActionIconPoints, kActionIconPoints));
        footerLayout_->addWidget(b);
        return b;
    };
    copy_ = action("paper-copy", tr("Copy"));
    cut_ = action("paper-cut", tr("Cut"));
    copy_->setIcon(actionIcon(Action::Copy, palette().color(QPalette::ButtonText), palette().color(QPalette::PlaceholderText)));
    cut_->setIcon(actionIcon(Action::Cut, palette().color(QPalette::ButtonText), palette().color(QPalette::PlaceholderText)));
    outer->addWidget(footer_);
    applyCompact();

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

bool PaperPanel::paperEmpty() const {
    const Shown* s = shown();
    return !s || s->empty();
}

bool PaperPanel::showsText() const { return stack_->currentWidget() == text_; }

void PaperPanel::showText(bool on) {
    const Shown* s = shown();
    const bool text = on && s && s->description.textLayer;
    stack_->setCurrentWidget(text ? static_cast<QWidget*>(text_) : view_);
    // The control shows the view that is on.
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

// The panel in the product's chrome: the header and the button bar at
// its heights and colours, every button in them a pill (a switch's pill
// stays as it is: its dot shows the position), the paper's body with the
// output border, the place dot's and the icons' colours.
void PaperPanel::setChrome(const shell::ChromeColours& c, const shell::ChromeSizes& s) {
    using namespace qt6::chrome;
    sizes_ = s;
    header_->setStyleSheet(QStringLiteral("#paper-header { %1 } #paper-header QLabel { color: %2; font-weight: bold; }")
                               .arg(header(c, s), qcss(c.headerText)) +
                           pills(c, s, QStringLiteral("QToolButton")));
    footer_->setStyleSheet(panelButtons(c, s, QStringLiteral("QToolButton"), QStringLiteral("paper-footer")) +
                           QStringLiteral("#paper-footer QToolButton[paperSwitch=\"true\"]:checked { background-color: %1; }"
                                          "#paper-footer QToolButton[paperSwitch=\"true\"]:checked:hover { background-color: %2; }")
                               .arg(qcss(c.pill), qcss(c.pillHover)));
    copy_->setIcon(actionIcon(Action::Copy, qcolour(c.pillText), qcolour(c.pillTextOff)));
    cut_->setIcon(actionIcon(Action::Cut, qcolour(c.pillText), qcolour(c.pillTextOff)));
    stack_->setStyleSheet(QStringLiteral("QStackedWidget { border: 1px solid %1; border-top: none; }").arg(qcss(c.outputBorder)));
    plugged_ = qcolour(c.plugged);
    unplugged_ = qcolour(c.unplugged);
    // The paper in both views, whatever the window's palette.
    const QColor paper = qcolour(c.named(shell::colour::kPaper));
    view_->setPaperColour(paper);
    QPalette pal = text_->palette();
    pal.setColor(QPalette::Base, paper);
    pal.setColor(QPalette::Text, qcolour(c.named(shell::colour::kPaperText)));
    text_->setPalette(pal);
    const QIcon printer = printerIcon();
    for (QToolButton* b : header_->findChildren<QToolButton*>())
        if (b != viewToggle_) b->setIcon(placeIcon(printer, b->isChecked() ? plugged_ : unplugged_));
    for (const auto& ctl : controls_)
        if (ctl.form == machine::ControlForm::Switch)
            if (auto* b = controlsBox_->findChild<QToolButton*>(QString::fromStdString("paper-control-" + ctl.id.value)))
                b->setIcon(switchDot(ctl.on ? plugged_ : unplugged_));
    applyCompact();
}

// The row as the library's rule has it for the panel's width: compact, the
// controls' short labels and the compact gap; else their labels and the gap.
void PaperPanel::applyCompact() {
    compact_ = shell::paperRowCompact(sizes_, width());
    const int gap = qRound(compact_ ? sizes_.paperRowCompactGap : sizes_.paperRowGap);
    footerLayout_->setContentsMargins(gap, 0, gap, 0);
    footerLayout_->setSpacing(gap);
    controlsLayout_->setSpacing(gap);
    for (const auto& c : controls_)
        if (auto* b = controlsBox_->findChild<QToolButton*>(QString::fromStdString("paper-control-" + c.id.value)))
            b->setText(QString::fromStdString(compact_ ? c.shortLabel : c.label));
}

void PaperPanel::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (shell::paperRowCompact(sizes_, width()) != compact_) applyCompact();
}

void PaperPanel::setPlaces(std::vector<QWidget*> places) {
    while (QLayoutItem* item = placesLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
            w->setObjectName(QString());   // gone for the tests before it is deleted
            w->deleteLater();
        }
        delete item;
    }
    const QIcon printer = printerIcon();
    for (QWidget* w : places) {
        w->setParent(this);
        if (auto* b = qobject_cast<QToolButton*>(w)) {
            if (b->toolTip().isEmpty()) b->setToolTip(b->text());
            b->setIcon(placeIcon(printer, b->isChecked() ? plugged_ : unplugged_));
            b->setIconSize(QSize(34, 16));
            b->setToolButtonStyle(printer.isNull() ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        }
        placesLayout_->addWidget(w);
        w->show();
    }
}

void PaperPanel::setControls(std::vector<peripherals::ControlInfo> controls) {
    const bool same = controls.size() == controls_.size() &&
                      std::equal(controls.begin(), controls.end(), controls_.begin(),
                                 [](const auto& a, const auto& b) {
                                     return a.id == b.id && a.form == b.form && a.label == b.label &&
                                            a.shortLabel == b.shortLabel;
                                 });
    controls_ = std::move(controls);
    if (!same) rebuildControls();
    // The state of each control: a key down while held, a switch at its position.
    for (const auto& c : controls_) {
        auto* b = controlsBox_->findChild<QToolButton*>(QString::fromStdString("paper-control-" + c.id.value));
        if (!b) continue;
        if (c.form == machine::ControlForm::Switch) {
            b->blockSignals(true);
            b->setChecked(c.on);
            b->blockSignals(false);
            b->setIcon(switchDot(c.on ? plugged_ : unplugged_));
        } else {
            b->setDown(c.on);
        }
    }
}

void PaperPanel::rebuildControls() {
    for (QToolButton* b : controlsBox_->findChildren<QToolButton*>()) delete b;
    for (const auto& c : controls_) {
        auto* b = new QToolButton(controlsBox_);
        b->setFocusPolicy(Qt::NoFocus);
        const std::string id = c.id.value;
        b->setObjectName(QString::fromStdString("paper-control-" + id));
        b->setText(QString::fromStdString(compact_ ? c.shortLabel : c.label));
        const auto args = [this, id](bool on) {
            return Value::object({{"peripheral", owner_ ? owner_->value : std::string()}, {"control", id}, {"on", on}});
        };
        if (c.form == machine::ControlForm::Switch) {
            // The label, then the dot of its position.
            b->setCheckable(true);
            b->setProperty("paperSwitch", true);
            b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            b->setLayoutDirection(Qt::RightToLeft);
            b->setIconSize(QSize(8, 8));
            b->setIcon(switchDot(c.on ? plugged_ : unplugged_));
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
    ownerLabel_->setText(QString::fromStdString(ownerName()));   // no name without an owner
    viewToggle_->setVisible(s && s->description.textLayer);
    controlsBox_->setVisible(owner_.has_value());
    const bool enabled = s && !s->empty();
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
    setFocusPolicy(Qt::NoFocus);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // no scroll bar
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

// The view follows the newest marks while it is at the end or no more
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
    p.fillRect(viewport()->rect(), paperColour_.isValid() ? paperColour_ : palette().color(QPalette::Base));
    if (!paper_) {   // no printer: a hint where the paper would start
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(viewport()->rect().adjusted(8, 12, -8, 0), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                   tr("No printer attached."));
        return;
    }
    if (paper_->description.width <= 0) return;
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
