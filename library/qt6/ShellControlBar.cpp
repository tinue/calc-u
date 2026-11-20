#include "qt6/ShellControlBar.hpp"
#include "qt6/Shell.hpp"
#include "shell/ObjectNames.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QToolButton>
#include <algorithm>

namespace calcu::qt6 {

namespace {

using shell::BarControl;
using shell::MenuItem;

std::string describe(const std::vector<shell::BarGroupModel>& groups) {
    std::string s;
    for (const auto& g : groups) {
        s += "[";
        for (const auto& c : g.controls) {
            s += std::to_string(static_cast<int>(c.kind)) + c.id + "|" + c.label + (c.plugged ? "1" : "0") + "|" +
                 sessions::toText(c.entry) + "|";
            for (const auto& i : c.choices)
                s += i.id + i.label + (i.enabled ? "1" : "0") + (i.checked ? "1" : "0") + i.reason + ",";
            s += ";";
        }
        s += "]";
    }
    return s;
}

} // namespace

ShellControlBar::ShellControlBar(Shell& shell, QWidget* parent) : QWidget(parent), shell_(shell) {
    setObjectName(shell::names::kControlBar);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(6, 4, 6, 4);
    for (std::size_t i = 0; i < groups_.size(); ++i) {
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

QComboBox* ShellControlBar::picker(const std::vector<MenuItem>& choices, QWidget* parent) {
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
    connect(box, &QComboBox::activated, this, [this, choices](int index) {
        if (index < 0 || index >= static_cast<int>(choices.size())) return;
        const MenuItem& c = choices[static_cast<std::size_t>(index)];
        if (!c.command.value.empty() && c.enabled) shell_.choose(c);
    });
    return box;
}

QWidget* ShellControlBar::build(const BarControl& c, QWidget* parent) {
    switch (c.kind) {
    case BarControl::Kind::ModelPicker: {
        QComboBox* box = picker(c.choices, parent);
        box->setObjectName(shell::names::kBarModel);
        return box;
    }
    case BarControl::Kind::SlotPicker: {   // the library's module picker, "n:" before it
        auto* w = new QWidget(parent);
        w->setObjectName(QString::fromStdString(shell::names::barSlot(c.id)));
        auto* l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        if (!c.label.empty()) l->addWidget(new QLabel(QString::fromStdString(c.label), w));
        l->addWidget(slotFactory_ ? slotFactory_(c.entry, w) : picker(c.choices, w));
        return w;
    }
    case BarControl::Kind::PlaceButton: {   // pressed while plugged in
        auto* b = new QToolButton(parent);
        b->setFocusPolicy(Qt::NoFocus);
        b->setObjectName(QString::fromStdString(shell::names::barPlace(c.id)));
        b->setText(QString::fromStdString(c.label));
        b->setCheckable(true);
        b->setChecked(c.plugged);
        const MenuItem choice = c.choices.empty() ? MenuItem{} : c.choices.front();
        b->setEnabled(choice.enabled);
        if (!choice.reason.empty()) b->setToolTip(QString::fromStdString(choice.reason));
        connect(b, &QToolButton::clicked, this, [this, choice] { shell_.choose(choice); });
        return b;
    }
    case BarControl::Kind::PlacePicker: {
        QComboBox* box = picker(c.choices, parent);
        box->setObjectName(QString::fromStdString(shell::names::barPlace(c.id)));
        return box;
    }
    case BarControl::Kind::Media: {   // the owner's controls, by medium kind
        auto it = mediaFactories_.find(c.mediumKind);
        if (it == mediaFactories_.end() || !it->second) return nullptr;
        QWidget* w = it->second(c.entry, parent);
        if (w) w->setObjectName(QString::fromStdString(shell::names::barMedia(c.id)));
        return w;
    }
    case BarControl::Kind::Action:
    case BarControl::Kind::More:   // no Qt6 frontend is without a menu bar
        return nullptr;
    }
    return nullptr;
}

void ShellControlBar::set(const std::vector<shell::BarGroupModel>& model) {
    const std::string text = describe(model);
    if (text == shown_) return;
    shown_ = text;
    for (std::size_t g = 0; g < groups_.size(); ++g) {
        for (auto& [id, w] : shellWidgets_[g]) {
            w->hide();
            w->setObjectName(QString());   // gone for the scenarios before it is deleted
            w->deleteLater();
        }
        shellWidgets_[g].clear();
    }
    for (const auto& group : model) {
        const auto g = static_cast<std::size_t>(group.group);
        for (const auto& c : group.controls)
            if (QWidget* w = build(c, groups_[g])) shellWidgets_[g].emplace_back(c.id, w);
    }
    for (std::size_t g = 0; g < groups_.size(); ++g) layoutGroup(static_cast<shell::BarGroup>(g));
}

void ShellControlBar::addProductControl(shell::BarGroup group, std::string after, QWidget* w) {
    const auto g = static_cast<std::size_t>(group);
    w->setParent(groups_[g]);
    w->show();   // shown until its product hides it
    productWidgets_[g].push_back(Product{std::move(after), w});
    layoutGroup(group);
}

void ShellControlBar::layoutGroup(shell::BarGroup group) {
    const auto g = static_cast<std::size_t>(group);
    auto* l = static_cast<QHBoxLayout*>(groups_[g]->layout());
    while (l->count() > 0) delete l->takeAt(0);   // the items only; the widgets stay
    std::vector<QWidget*> order;
    auto productsAfter = [&](const std::string& id) {
        for (const auto& p : productWidgets_[g])
            if (p.after == id) order.push_back(p.widget);
    };
    for (const auto& [id, w] : shellWidgets_[g]) {
        order.push_back(w);
        productsAfter(id);
    }
    for (const auto& p : productWidgets_[g]) {   // after "" or a control that is not there: at the end
        bool placed = std::find(order.begin(), order.end(), p.widget) != order.end();
        if (!placed) order.push_back(p.widget);
    }
    bool any = false;
    for (QWidget* w : order) {
        l->addWidget(w);
        // A product's control keeps the visibility its product gives it
        // (shown from its own product entry); the shell's are shown.
        const bool product = std::any_of(productWidgets_[g].begin(), productWidgets_[g].end(),
                                         [w](const Product& p) { return p.widget == w; });
        if (!product) w->show();
        any = any || !w->isHidden();
    }
    groups_[g]->setVisible(any);   // no gap
    groups_[g]->updateGeometry();             // the bar makes room for what came
}

} // namespace calcu::qt6
