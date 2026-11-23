#include "qt6/ShellControlBar.hpp"
#include "qt6/ChromeStyle.hpp"
#include "qt6/MediaPicker.hpp"
#include "qt6/Shell.hpp"
#include "qt6/SlotPicker.hpp"
#include "shell/ObjectNames.hpp"
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>
#include <QToolButton>
#include <algorithm>
#include <cstdlib>

namespace calcu::qt6 {

namespace {

using shell::BarControl;
using shell::MenuItem;

// The More menu's items, as the menu bar has them but without shortcuts
// (the menu bar's own); a disabled item's reason as its tooltip.
void fillMore(QMenu& menu, const std::vector<MenuItem>& items, const Choose& choose) {
    menu.setToolTipsVisible(true);
    for (const auto& item : items) {
        const QString name = QString::fromStdString(shell::names::moreItem(item.id));
        if (item.kind == MenuItem::Kind::Separator) {
            menu.addSeparator();
            continue;
        }
        if (item.kind == MenuItem::Kind::Heading) {   // a menu's name: bold, not chosen
            if (!menu.isEmpty()) menu.addSeparator();
            QAction* h = menu.addAction(QString::fromStdString(item.label));
            h->setObjectName(name);
            h->setEnabled(false);
            QFont f = h->font();
            f.setBold(true);
            h->setFont(f);
            continue;
        }
        if (item.kind == MenuItem::Kind::Submenu) {
            auto* sub = new QMenu(QString::fromStdString(item.label), &menu);
            sub->setObjectName(name);
            sub->menuAction()->setObjectName(name + QStringLiteral("-menu"));
            sub->menuAction()->setMenuRole(QAction::NoRole);
            fillMore(*sub, item.children, choose);
            menu.addMenu(sub);
            continue;
        }
        QAction* a = menu.addAction(QString::fromStdString(item.label));
        a->setObjectName(name);
        a->setMenuRole(QAction::NoRole);   // the menu bar's items carry the roles
        a->setCheckable(item.checkable);
        a->setChecked(item.checked);
        a->setEnabled(item.enabled);
        if (!item.reason.empty()) a->setToolTip(QString::fromStdString(item.reason));
        QObject::connect(a, &QAction::triggered, &menu, [choose, item] { choose(item); });
    }
}

} // namespace

ShellControlBar::ShellControlBar(Shell& shell, QWidget* parent) : QWidget(parent), shell_(shell) {
    setObjectName(shell::names::kControlBar);
    auto* row = new QHBoxLayout(this);
    row_ = row;
    row->setContentsMargins(6, 4, 6, 4);
    for (std::size_t i = 0; i < groups_.size(); ++i) {
        // Room before the media and the End group, taken up when the model pushes them.
        if (i == static_cast<std::size_t>(shell::BarGroup::Media) || i == static_cast<std::size_t>(shell::BarGroup::End))
            row->addStretch(0);
        auto* g = new QWidget(this);
        g->setObjectName(QStringLiteral("bar-group-%1").arg(i));
        auto* l = new QHBoxLayout(g);
        l->setContentsMargins(0, 0, 0, 0);
        row->addWidget(g);
        g->hide();
        groups_[i] = g;
    }
    row->addStretch(1);
}

QComboBox* shellPicker(const std::vector<MenuItem>& choices, Choose choose, QWidget* parent) {
    auto* box = new QComboBox(parent);
    // No shell widget takes the keyboard focus: the window owns typing, and
    // with a focusable widget in it Qt spends Tab on moving the focus
    // instead of handing it to the keyboard map. Clicks work all the same.
    box->setFocusPolicy(Qt::NoFocus);
    auto* model = qobject_cast<QStandardItemModel*>(box->model());
    int current = -1;
    for (std::size_t i = 0; i < choices.size(); ++i) {
        const auto& c = choices[i];
        box->addItem(QString::fromStdString(c.label), QString::fromStdString(c.id));
        if (model) {
            QStandardItem* item = model->item(static_cast<int>(i));
            item->setEnabled(c.enabled);   // disabled with the reason
            if (!c.reason.empty()) item->setData(QString::fromStdString(c.reason), Qt::ToolTipRole);
        }
        if (c.checked) current = static_cast<int>(i);
    }
    box->setCurrentIndex(current);
    QObject::connect(box, &QComboBox::activated, box, [choose = std::move(choose), choices](int index) {
        if (index < 0 || index >= static_cast<int>(choices.size())) return;
        const MenuItem& c = choices[static_cast<std::size_t>(index)];
        if (!c.command.value.empty() && c.enabled) choose(c);
    });
    return box;
}

QWidget* placeControl(const BarControl& c, Choose choose, QWidget* parent) {
    if (c.kind == BarControl::Kind::PlacePicker) {
        QComboBox* box = shellPicker(c.choices, std::move(choose), parent);
        box->setObjectName(QString::fromStdString(shell::names::barPlace(c.id)));
        return box;
    }
    auto* b = new QToolButton(parent);
    b->setFocusPolicy(Qt::NoFocus);
    b->setObjectName(QString::fromStdString(shell::names::barPlace(c.id)));
    b->setText(QString::fromStdString(c.label));
    b->setCheckable(true);
    b->setChecked(c.plugged);
    const MenuItem choice = c.choices.empty() ? MenuItem{} : c.choices.front();
    b->setEnabled(choice.enabled);
    if (!choice.reason.empty()) b->setToolTip(QString::fromStdString(choice.reason));
    QObject::connect(b, &QToolButton::clicked, b, [choose = std::move(choose), choice] { choose(choice); });
    return b;
}

void ShellControlBar::chooseItem(const MenuItem& i) { shell_.choose(i); }

// One button for the bar's life: its menu may be open (a nested loop)
// while a topic changes, so the menu is filled anew only once it is closed.
QWidget* ShellControlBar::moreButton(const BarControl& c, QWidget* parent) {
    if (!more_) {
        more_ = new QToolButton(parent);
        more_->setFocusPolicy(Qt::NoFocus);
        more_->setObjectName(shell::names::kBarMore);
        more_->setPopupMode(QToolButton::InstantPopup);
        more_->setToolButtonStyle(Qt::ToolButtonTextOnly);
        moreMenu_ = new QMenu(more_);
        moreMenu_->setObjectName(shell::names::kMoreMenu);
        more_->setMenu(moreMenu_);
        connect(moreMenu_, &QMenu::aboutToHide, this, [this] {
            QTimer::singleShot(0, this, [this] { fillMoreMenu(); });   // after the chosen item's signal
        });
    }
    more_->setParent(parent);
    moreIcon_ = c.icon;
    moreLabel_ = c.label;
    more_->setText(moreText(labels_));
    more_->setToolTip(QString::fromStdString(c.label));
    more_->setAccessibleName(QString::fromStdString(c.label));
    styleMore();
    moreItems_ = c.choices;
    fillMoreMenu();
    return more_;
}

void ShellControlBar::setChrome(const shell::Chrome* c) {
    chrome_ = c;
    shown_.reset();   // the slot pickers' width is the chrome's
    styleMore();
}

// More as the bar draws its buttons, a pill where the product's bar has
// pills, else in the bar's own style; never with the platform's menu arrow.
void ShellControlBar::styleMore() {
    if (!more_) return;
    if (!chrome_ || !chrome_->sizes.barPills) {
        more_->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; width: 0; }"));
        return;
    }
    const shell::ChromeColours& c = chrome_->colours(chrome::darkAppearance(this));
    more_->setStyleSheet(chrome::pills(c, chrome_->sizes, QStringLiteral("QToolButton")));
}

void ShellControlBar::changeEvent(QEvent* e) {
    if (e->type() == QEvent::PaletteChange) styleMore();
    QWidget::changeEvent(e);
}

BarLabels::BarLabels(QWidget* control, std::function<int()> width, std::function<void(bool)> show)
    : QObject(control), control_(control), width_(std::move(width)), show_(std::move(show)) {}

BarLabels* barLabel(QLabel* label) {
    return new BarLabels(
        label->parentWidget(),
        [label] {
            const QLayout* l = label->parentWidget() ? label->parentWidget()->layout() : nullptr;
            return label->sizeHint().width() + (l ? std::max(0, l->spacing()) : 0);
        },
        [label](bool shown) { label->setVisible(shown); });
}

QString ShellControlBar::moreText(bool labels) const {
    return QString::fromStdString(labels && !moreLabel_.empty() ? moreIcon_ + " " + moreLabel_ : moreIcon_);
}

void ShellControlBar::fitBelow(double faceplate, double room) {
    faceplate_ = faceplate;
    room_ = room;
    refit();
}

// Again whenever a control came, went or changed its size (also after
// the labels switched: the same result then, no further round).
bool ShellControlBar::event(QEvent* e) {
    const bool handled = QWidget::event(e);
    if (e->type() == QEvent::LayoutRequest) refit();
    return handled;
}

void ShellControlBar::refit() {
    if (faceplate_ <= 0) return;
    // Every control's labels as the bar shows them now (a control made
    // since comes with its own).
    const QList<BarLabels*> all = findChildren<BarLabels*>();
    for (BarLabels* l : all) l->show(labels_);
    // The bar with and without its labels, from the text widths, so that
    // switching them asks for no second round.
    int labels = 0;
    for (BarLabels* l : all)
        if (l->control() && l->control()->isVisibleTo(this)) labels += l->width();
    if (more_ && more_->isVisibleTo(this) && !moreLabel_.empty()) {
        const QFontMetrics fm = more_->fontMetrics();
        labels += fm.horizontalAdvance(moreText(true)) - fm.horizontalAdvance(moreText(false));
    }
    const int shown = sizeHint().width();
    const int without = labels_ ? shown - labels : shown, with = without + labels;
    // The two measurements differ by rounding; a change smaller than that
    // is no change, or a faceplate as wide as that difference would switch
    // the labels at every round.
    if (std::abs(without - without_) >= 4 || std::abs(with - with_) >= 4) {
        without_ = without;
        with_ = with;
    }
    const shell::BarFit fit = shell::barFit(faceplate_, without_, with_, room_);
    if (fit.labels != labels_) {
        labels_ = fit.labels;
        for (BarLabels* l : all) l->show(labels_);
        if (more_) more_->setText(moreText(labels_));
    }
    const int width = static_cast<int>(fit.width);
    if (maximumWidth() != width) setMaximumWidth(std::max(1, width));
}

void ShellControlBar::fillMoreMenu() {
    if (!moreMenu_ || moreMenu_->isVisible()) return;
    if (moreShown_ == moreItems_) return;
    moreShown_ = moreItems_;
    for (QMenu* sub : moreMenu_->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly)) delete sub;
    moreMenu_->clear();
    fillMore(*moreMenu_, moreItems_, [this](const MenuItem& i) { chooseMore(i); });
}

void ShellControlBar::chooseMore(const MenuItem& i) {
    if (i.confirm.empty()) {
        shell_.choose(i);
        return;
    }
    // Asked first, window-modal and without a nested loop: the item's
    // label and Cancel.
    auto* box = new QMessageBox(window());
    box->setObjectName(shell::names::kMoreConfirm);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setIcon(i.destructive ? QMessageBox::Warning : QMessageBox::Question);
    box->setText(QString::fromStdString(i.confirm));
    QPushButton* yes = box->addButton(QString::fromStdString(i.label), i.destructive ? QMessageBox::DestructiveRole
                                                                                     : QMessageBox::AcceptRole);
    yes->setObjectName(shell::names::kMoreConfirmYes);
    QPushButton* no = box->addButton(QMessageBox::Cancel);
    no->setObjectName(shell::names::kMoreConfirmNo);
    box->setDefaultButton(no);
    connect(box, &QMessageBox::buttonClicked, this, [this, yes, i](QAbstractButton* clicked) {
        if (clicked == yes) shell_.choose(i);
    });
    box->open();
}

QWidget* ShellControlBar::build(const BarControl& c, QWidget* parent) {
    switch (c.kind) {
    case BarControl::Kind::ModelPicker: {
        QComboBox* box = shellPicker(c.choices, choose(), parent);
        box->setObjectName(shell::names::kBarModel);
        return box;
    }
    case BarControl::Kind::SlotPicker: {   // the library's module picker, "n:" before it
        auto* w = new QWidget(parent);
        w->setObjectName(QString::fromStdString(shell::names::barSlot(c.id)));
        auto* l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        if (!c.label.empty()) l->addWidget(new QLabel(QString::fromStdString(c.label), w));
        std::function<void(const std::string&)> save;
        if (shell_.product().nameAndSave.slotButtons) save = [this](const std::string& place) { shell_.nameAndSave(place); };
        auto* picker = new SlotPicker(
            shell_.host(), c.entry, std::move(save), chrome_ ? chrome_->sizes.slotPickerEm : 0,
            [this](const sessions::CommandName& n, sessions::Value args) { shell_.sendChecked(n, std::move(args)); }, w);
        picker->picker()->setShowShort(c.shortShown);
        l->addWidget(picker);
        return w;
    }
    case BarControl::Kind::PlaceButton:
    case BarControl::Kind::PlacePicker:
        return placeControl(c, choose(), parent);
    case BarControl::Kind::Media: {   // the owner's controls, by medium kind
        auto it = mediaFactories_.find(c.mediumKind);
        if (it == mediaFactories_.end() || !it->second) return nullptr;
        QWidget* w = it->second(c.entry, parent);
        if (w) w->setObjectName(QString::fromStdString(shell::names::barMedia(c.id)));
        return w;
    }
    case BarControl::Kind::More:   // a menu of what the bar does not show
        return moreButton(c, parent);
    }
    return nullptr;
}

void ShellControlBar::set(const std::vector<shell::BarGroupModel>& model) {
    if (shown_ == model) return;
    shown_ = model;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        for (auto& [id, w] : shellWidgets_[g]) {
            if (w == more_) continue;   // kept, see moreButton()
            w->hide();
            w->setObjectName(QString());   // gone for the scenarios before it is deleted
            w->deleteLater();
        }
        shellWidgets_[g].clear();
    }
    // The room before a pushed group takes the space, the room of a group
    // that is not pushed none (else a group would move when another one
    // grows or shrinks); without a pushed group the last stretch.
    bool spread = false;
    for (const auto& group : model) spread = spread || group.pushed;
    const auto pushed = [&](QWidget* w) {
        for (const auto& group : model)
            if (groups_[static_cast<std::size_t>(group.group)] == w) return group.pushed;
        return false;
    };
    for (int i = 0; i < row_->count(); ++i) {
        if (!row_->itemAt(i)->spacerItem()) continue;
        const bool last = i == row_->count() - 1;
        QWidget* next = last ? nullptr : row_->itemAt(i + 1)->widget();
        row_->setStretch(i, last ? (spread ? 0 : 1) : (next && pushed(next) ? 1 : 0));
    }
    for (const auto& group : model) {
        const auto g = static_cast<std::size_t>(group.group);
        for (const auto& c : group.controls)
            if (QWidget* w = build(c, groups_[g])) shellWidgets_[g].emplace_back(c.id, w);
    }
    for (std::size_t g = 0; g < groups_.size(); ++g) layoutGroup(static_cast<shell::BarGroup>(g));
}

void ShellControlBar::addProductControl(shell::BarGroup group, QWidget* w) {
    const auto g = static_cast<std::size_t>(group);
    w->setParent(groups_[g]);
    w->show();   // shown until its product hides it
    productWidgets_[g].push_back(w);
    layoutGroup(group);
}

void ShellControlBar::layoutGroup(shell::BarGroup group) {
    const auto g = static_cast<std::size_t>(group);
    auto* l = static_cast<QHBoxLayout*>(groups_[g]->layout());
    while (l->count() > 0) delete l->takeAt(0);   // the items only; the widgets stay
    // The shell's controls, then the product's: theirs at the end of the
    // group, so no shell control moves when one comes or goes.
    std::vector<QWidget*> order;
    for (const auto& [id, w] : shellWidgets_[g]) order.push_back(w);
    for (QWidget* w : productWidgets_[g]) order.push_back(w);
    bool any = false;
    for (QWidget* w : order) {
        l->addWidget(w);
        // A product's control keeps the visibility its product gives it
        // (shown from its own product entry); the shell's are shown.
        const bool product = std::find(productWidgets_[g].begin(), productWidgets_[g].end(), w) != productWidgets_[g].end();
        if (!product) w->show();
        any = any || !w->isHidden();
    }
    groups_[g]->setVisible(any);   // no gap
    groups_[g]->updateGeometry();             // the bar makes room for what came
}

} // namespace calcu::qt6
