#include "qt6/PanelArea.hpp"
#include "qt6/ChromeStyle.hpp"
#include <QEvent>
#include "control/ControlState.hpp"
#include "qt6/PaperPanel.hpp"
#include "qt6/SessionHost.hpp"
#include "shell/ObjectNames.hpp"
#include <QHBoxLayout>

namespace calcu::qt6 {

PanelArea::PanelArea(SessionHost& host, QWidget* parent) : QWidget(parent), host_(host) {
    setObjectName(shell::names::kPanelArea);
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    // The library's paper panel, its commands through the host.
    factories_["paper"] = [this](QWidget* p) -> QWidget* {
        auto* panel = new PaperPanel(p);
        panel->setSender([this](const sessions::CommandName& n, sessions::Value a) { host_.send(n, std::move(a)); });
        return panel;
    };
    // The paper panel is there from the start, so it takes the paper topics
    // from the listen on (the host hands them on after the event loop's next
    // turn).
    QWidget* paper = factories_["paper"](this);
    panels_["paper"] = paper;
    layout_->addWidget(paper, 1);
}

void PanelArea::registerPanel(std::string kind, Factory f) {
    if (QWidget* old = panel(kind)) {   // a product's own panel of that kind replaces the default
        panels_.erase(kind);
        old->deleteLater();
    }
    factories_[std::move(kind)] = std::move(f);
}

void PanelArea::addPanel(QWidget* w) {
    w->setParent(this);
    layout_->addWidget(w);
    w->show();
}

QWidget* PanelArea::panel(const std::string& kind) const {
    auto it = panels_.find(kind);
    return it == panels_.end() ? nullptr : it->second;
}

void PanelArea::set(const std::vector<shell::Panel>& panels) {
    for (const auto& p : panels) {
        QWidget* w = panel(p.kind);
        if (!w) {   // made once, kept (the other panels keep their content)
            auto f = factories_.find(p.kind);
            if (f == factories_.end() || !f->second) continue;
            w = f->second(this);
            if (!w) continue;
            panels_[p.kind] = w;
            layout_->addWidget(w, 1);
            styleChrome();
        }
        if (auto* paper = qobject_cast<PaperPanel*>(w)) {   // always, with or without an owner
            paper->setOwner(p.owner.empty() ? std::nullopt : std::optional<machine::PeripheralId>(machine::PeripheralId{p.owner}));
            if (p.places != placesShown_) {   // built anew only when they changed
                placesShown_ = p.places;
                std::vector<QWidget*> places;
                for (const auto& c : p.places) places.push_back(placeControl(c, choose_, paper));
                paper->setPlaces(std::move(places));
            }
        }
        w->show();
    }
}

void PanelArea::setChrome(const shell::Chrome* c) {
    chrome_ = c;
    styleChrome();
}

void PanelArea::styleChrome() {
    if (!chrome_) return;
    const shell::ChromeColours& c = chrome_->colours(chrome::darkAppearance(this));
    for (auto& [kind, w] : panels_)
        if (auto* paper = qobject_cast<PaperPanel*>(w)) paper->setChrome(c, chrome_->sizes);
}

void PanelArea::changeEvent(QEvent* e) {
    if (e->type() == QEvent::PaletteChange) styleChrome();
    QWidget::changeEvent(e);
}

void PanelArea::deliver(const sessions::Message& m) {
    for (auto& [kind, w] : panels_)
        if (auto* paper = qobject_cast<PaperPanel*>(w)) paper->deliver(m);
}

} // namespace calcu::qt6
