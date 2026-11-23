#include "qt6/Shell.hpp"
#include "qt6/FaceplateInput.hpp"
#include "qt6/PanelArea.hpp"
#include "qt6/RomsMissingView.hpp"
#include "qt6/ShellAbout.hpp"
#include "qt6/ShellControlBar.hpp"
#include "qt6/ShellLoading.hpp"
#include "qt6/ShellMenus.hpp"
#include "qt6/ShellNotices.hpp"
#include "qt6/ShellSettingsDialog.hpp"
#include "sessions/LibraryTopics.hpp"
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QTimer>
#include <QtDebug>
#include <algorithm>
#include <chrono>
#include <variant>

namespace calcu::qt6 {

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

ShellChoosers defaultChoosers() {
    ShellChoosers c;
    c.file = [](QWidget* parent, const QString& title, const QString& start, const QString& filter) {
        return QFileDialog::getOpenFileName(parent, title, start, filter);
    };
    c.folder = [](QWidget* parent, const QString& title, const QString& start) {
        return QFileDialog::getExistingDirectory(parent, title, start);
    };
    c.openUrl = [](const QUrl& url) { return QDesktopServices::openUrl(url); };
    c.clipboardText = [] { return QGuiApplication::clipboard()->text(); };
    return c;
}

std::optional<QRect> displayRect(const shell::HitMap& map, double scale) {
    if (!map.display) return std::nullopt;
    const auto& d = *map.display;
    return QRect(static_cast<int>(d.x * scale), static_cast<int>(d.y * scale), static_cast<int>(d.w * scale),
                 static_cast<int>(d.h * scale));
}

} // namespace

Shell::Shell(QMainWindow& window, SessionHost& host, shell::ShellProduct product, shell::DialogSpec spec, ShellOptions options)
    : QObject(&window), window_(window), host_(host), product_(std::move(product)), spec_(std::move(spec)),
      options_(std::move(options)), choosers_(defaultChoosers()), loadingTimer_(new QTimer(this)) {
    menuBar_ = new ShellMenus(*window_.menuBar(), *this);
    bar_ = new ShellControlBar(*this);
    panels_ = new PanelArea(host_);
    panels_->setChoose([this](const shell::MenuItem& i) { choose(i); });
    romsMissing_ = new RomsMissingView();
    noticeView_ = new ShellNotices(&window_);
    loadingView_ = new ShellLoading(window_.centralWidget() ? window_.centralWidget() : &window_);
    loadingTimer_->setSingleShot(true);
    connect(loadingTimer_, &QTimer::timeout, this, &Shell::refreshLoading);
    connect(&host_, &SessionHost::called, this, &Shell::refreshLoading);
    host_.addMessageHandler([this](const sessions::Message& m) { message(m); });
    // Rebuilt after the event loop's turn: a widget whose signal sent the
    // command that changed a topic is not deleted inside its own signal.
    auto* pending = new QTimer(this);
    pending->setSingleShot(true);
    pending->setInterval(0);
    connect(pending, &QTimer::timeout, this, [this] { refresh(QString()); });
    connect(&host_, &SessionHost::topicChanged, this, [pending](const QString&) { pending->start(); });
    askPresetList();
    refresh(QString());
}

Shell::~Shell() = default;

QWidget* Shell::controlBar() { return bar_; }
PanelArea* Shell::panelArea() { return panels_; }
QWidget* Shell::romsMissingView() { return romsMissing_; }

void Shell::setFaceplate(QWidget* faceplate, shell::HitMap map, std::function<double()> scale) {
    setFaceplate(faceplate, std::move(map), FaceplateGeometry{std::move(scale)});
}

void Shell::setFaceplate(QWidget* faceplate, shell::HitMap map, FaceplateGeometry g) {
    faceplate_ = faceplate;
    hitMap_ = map;
    scale_ = g.scale ? std::move(g.scale) : [] { return 1.0; };
    origin_ = std::move(g.origin);
    delete faceplateInput_;
    faceplateInput_ = faceplate ? new FaceplateInput(*faceplate, host_, std::move(map), scale_, origin_, std::move(g.productFirst))
                                : nullptr;
    delete loadingView_;   // the overlay goes onto the faceplate, below its display
    loadingView_ = new ShellLoading(faceplate ? faceplate : &window_);
    refresh(QString());
}

void Shell::addBarControl(shell::BarGroup group, QWidget* w) {
    bar_->addProductControl(group, w);
}

void Shell::registerMediaControls(std::string kind, Factory f) {
    bar_->setMediaFactory(std::move(kind), std::move(f));
    refresh(QString());
}

void Shell::setSlotPicker(SlotPickerOptions o) {
    bar_->setSlotOptions(std::move(o));
    refresh(QString());
}

void Shell::registerPanel(std::string kind, std::function<QWidget*(QWidget*)> f) {
    panels_->registerPanel(std::move(kind), std::move(f));
    panels_->set(shell::panels(inputs_, product_));
}

void Shell::setChoosers(ShellChoosers c) {
    const ShellChoosers d = defaultChoosers();
    if (!c.file) c.file = d.file;
    if (!c.folder) c.folder = d.folder;
    if (!c.openUrl) c.openUrl = d.openUrl;
    if (!c.clipboardText) c.clipboardText = d.clipboardText;
    choosers_ = std::move(c);
}

void Shell::setChrome(const shell::Chrome& c) {
    bar_->setChrome(&c);
    panels_->setChrome(&c);
    refresh(QString());
}

bool Shell::loadingShown() const { return loadingView_ && !loadingView_->isHidden(); }

void Shell::askPresetList() {
    host_.ask(options_.names.presetList, {}, [this](const sessions::ReplyMessage& r) {
        inputs_.presetList = r.done && r.result ? r.result->toValue() : sessions::Value();
        refresh(QString());
    });
}

void Shell::message(const sessions::Message& m) {
    if (const auto* s = std::get_if<sessions::StreamMessage>(&m)) {
        if (s->topic == sessions::kNoticesTopic) {   // each, in order
            for (const auto& p : s->appended)
                if (p) noticeQueue_.push(shell::read::text(p->toValue(), "sentence"));
            if (!s->appended.empty()) QTimer::singleShot(0, this, [this] { showNotice(); });
        }
        if (s->topic == sessions::kLogTopic && options_.logToPlatform)   // no view of its own
            for (const auto& p : s->appended)
                if (p) qInfo().noquote() << q(shell::read::text(p->toValue(), "text"));
    }
    if (const auto* s = std::get_if<sessions::StateMessage>(&m); s && s->topic == options_.names.busy) {
        const shell::Busy b = shell::busyOf(s->state ? s->state->toValue() : sessions::Value());
        loading_.busy(b.operationRuns, b.full, host_.now());   // at the change, on the host's wall clock
        refreshLoading();
    }
    panels_->deliver(m);
}

void Shell::refreshLoading() {
    const pacing::WallTime now = host_.now();
    const bool show = loading_.shown(now);
    if (show && loadingView_->isHidden()) {
        std::optional<QRect> at = faceplate_ ? displayRect(hitMap_, scale_ ? scale_() : 1.0) : std::nullopt;
        if (at && origin_) at->translate(origin_().toPoint());
        loadingView_->place(at);
        loadingView_->show();
        loadingView_->raise();
    } else if (!show && !loadingView_->isHidden()) {
        loadingView_->hide();
    }
    if (auto due = loading_.due(); due && !show) {
        const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(*due - now).count();
        loadingTimer_->start(static_cast<int>(std::max<long long>(0, static_cast<long long>(wait)) + 1));
    }
}

void Shell::refresh(const QString&) {
    const auto& n = options_.names;
    sessions::Value cs = host_.value(n.controlState);
    const std::string model = shell::read::text(cs, "model");
    if (faceplateInput_ && !runningModel_.empty() && model != runningModel_) faceplateInput_->releaseAll();
    runningModel_ = model;
    inputs_.controlState = std::move(cs);
    inputs_.busy = host_.value(n.busy);
    inputs_.runState = host_.value(n.runState);
    inputs_.romsMissing = host_.value(n.romsMissing);
    inputs_.settings = host_.value(n.settings);
    inputs_.media = host_.value(n.media);
    for (const auto& t : shell::productTopics(product_)) inputs_.productTopics[t.value] = host_.value(t);
    menus_ = shell::menus(inputs_, product_, options_.traits);
    menuBar_->set(menus_);
    bar_->set(shell::bar(inputs_, product_, options_.traits));
    bar_->setVisible(inputs_.started());   // no bar before the start (ROMs missing)
    panels_->set(shell::panels(inputs_, product_));
    const bool missing = !inputs_.started() && !inputs_.romsMissing.isNull();
    static_cast<RomsMissingView*>(romsMissing_)->setMessage(q(shell::read::text(inputs_.romsMissing, "text")));
    romsMissing_->setVisible(missing);
    if (faceplate_) faceplate_->setVisible(!missing);
    if (settings_ && settings_->isVisible()) settings_->refresh();
}

void Shell::sendChecked(const sessions::CommandName& name, sessions::Value args) {
    sessions::Command sent{name, args};
    host_.send(name, std::move(args), [this, sent](const sessions::ReplyMessage& r) {
        if (r.done) return;
        noticeQueue_.refusal(r.reason, sent);   // a notice, or a question for `asks:`
        QTimer::singleShot(0, this, [this] { showNotice(); });
    });
}

void Shell::showNotice() {
    if (noticeView_->showing()) return;
    const shell::NoticeQueue::Shown* s = noticeQueue_.current();
    if (!s) return;
    noticeView_->show(*s, [this](bool yes) {
        if (auto again = noticeQueue_.answer(yes)) sendChecked(again->name, again->args);
        QTimer::singleShot(0, this, [this] { showNotice(); });   // the next one, in order
    });
}

void Shell::choose(const shell::MenuItem& item) {
    if (!item.enabled) return;
    if (!item.command.value.empty()) {
        sendChecked(item.command, item.args);
        return;
    }
    namespace a = shell::actions;
    if (item.id == a::kLoadPreset) loadPreset();
    else if (item.id == a::kSettings) openSettings();
    else if (item.id == a::kAbout) openAbout();
    else if (item.id == a::kHelp) openHelp();
    else if (item.id == a::kQuit) window_.close();
    else if (item.id == a::kCopyScreen) {
        if (product_.copyScreen) product_.copyScreen();
    } else if (item.id == a::kPasteText) {
        sendChecked(shell::commands::kPaste, sessions::Value::object({{"text", choosers_.clipboardText().toStdString()}}));
    } else {
        // The product's own item, in a submenu of the product's too.
        const std::function<void(const std::vector<shell::ProductItem>&)> act = [&](const auto& items) {
            for (const auto& p : items) {
                if (p.id == item.id && p.act) p.act();
                act(p.children);
            }
        };
        act(product_.items);
    }
}

void Shell::openSettings() {
    if (!settings_) settings_ = new ShellSettingsDialog(*this, &window_);
    askPresetList();   // the startup presets as the preset list has them now
    settings_->refresh();
    settings_->open();   // window-modal, no nested loop
}

void Shell::openAbout() {
    delete about_;
    about_ = new ShellAbout(product_, &window_);
    about_->open();
}

void Shell::openHelp() {
    if (product_.helpUrl.empty()) return;
    (void)choosers_.openUrl(QUrl(q(product_.helpUrl)));   // whatever happens, nothing else does
}

QString Shell::startFolder(const std::string& kind) const {
    for (const auto& r : shell::read::elements(inputs_.settings, "rows"))
        if (shell::read::text(r, "section") == "folders" && shell::read::text(r, "key") == kind) {
            const std::string inUse = shell::read::text(r, "inUse");
            if (!inUse.empty()) return q(inUse);
        }
    return QDir::homePath();
}

void Shell::rememberFolder(const std::string& kind, const QString& file) {
    host_.send(shell::commands::kRememberFolder,
               sessions::Value::object({{"kind", kind}, {"folder", QFileInfo(file).absolutePath().toStdString()}}));
}

void Shell::loadPreset() {
    const QString file = choosers_.file(&window_, QStringLiteral("Load Preset"), startFolder("presets"), q(product_.presetFilter));
    if (file.isEmpty()) return;
    rememberFolder("presets", file);
    sendChecked(shell::commands::kLoadPreset, sessions::Value::object({{"ref", file.toStdString()}}));
}

} // namespace calcu::qt6
