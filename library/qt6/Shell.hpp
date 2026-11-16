#pragma once
#include "qt6/SessionHost.hpp"
#include "shell/BarModel.hpp"
#include "shell/HitMap.hpp"
#include "shell/Inputs.hpp"
#include "shell/Loading.hpp"
#include "shell/MenuModel.hpp"
#include "shell/Notices.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class QMainWindow;
class QTimer;
class QWidget;

// The shell of the Qt6 component set (LIB-14 design §3.8; spec AC-1 to AC-23,
// AC-42, AC-43): puts the menu bar, the control bar, the settings and About
// dialogs, the notices, "Loading…", the area for panels and the pointer on
// the faceplate on a product's QMainWindow, drawn from the session's topics
// through a SessionHost. It names no model and no product (ADR 0023): all
// of that is in the ShellProduct. The product places controlBar(),
// panelArea() and romsMissingView() in its own layout.

namespace calcu::qt6 {

class FaceplateInput;
class PanelArea;
class ShellAbout;
class ShellControlBar;
class ShellLoading;
class ShellMenus;
class ShellNotices;
class ShellSettingsDialog;

struct ShellOptions {
    bool logToPlatform = true;   // G1: each line of the log to qInfo() (AC-18)
    shell::Traits traits{true, true, false};
    shell::InputNames names{};
};

// The file and folder dialogs and the browser (the tests replace them).
struct ShellChoosers {
    // A file to open (Load Preset…, "Choose…"): empty when cancelled.
    std::function<QString(QWidget* parent, const QString& title, const QString& start, const QString& filter)> file;
    std::function<QString(QWidget* parent, const QString& title, const QString& start)> folder;   // "Change…"
    std::function<bool(const QUrl&)> openUrl;   // Help; its result is ignored (EC-6)
    std::function<QString()> clipboardText;     // Paste Text
};

class Shell : public QObject {
    Q_OBJECT
public:
    Shell(QMainWindow&, SessionHost&, shell::ShellProduct, shell::DialogSpec, ShellOptions = {});
    ~Shell() override;

    QWidget* controlBar();            // the product places it below its faceplate
    PanelArea* panelArea();           // the product places it in its layout
    QWidget* romsMissingView();       // shown by the shell in place of the faceplate (AC-19)
    void setFaceplate(QWidget*, shell::HitMap, std::function<double()> scale);   // AC-24, AC-43
    void addBarControl(shell::BarGroup, std::string after, QWidget*);           // AC-42
    using Factory = std::function<QWidget*(const sessions::Value& entry, QWidget* parent)>;
    void registerMediaControls(std::string mediumKind, Factory);   // LIB-7's kinds; C59-7's card
    void registerSlotPicker(Factory);                              // LIB-7's module picker
    void registerPanel(std::string kind, std::function<QWidget*(QWidget* parent)>);   // LIB-8's paper panel by default
    void setChoosers(ShellChoosers);

    // What the shell shows now (the tests, the scenarios).
    const shell::ShellInputs& inputs() const { return inputs_; }
    const shell::ShellProduct& product() const { return product_; }
    const shell::DialogSpec& dialogSpec() const { return spec_; }
    bool loadingShown() const;
    shell::NoticeQueue& notices() { return noticeQueue_; }
    ShellSettingsDialog* settingsDialog() const { return settings_; }
    SessionHost& host() { return host_; }
    QMainWindow& window() { return window_; }

    // An item of a menu or a picker chosen: its command, or the shell's
    // action, or the product's (AC-7).
    void choose(const shell::MenuItem&);
    // A command the shell sends: its refusal becomes a notice or a question
    // (D-7, N4).
    void sendChecked(const sessions::CommandName&, sessions::Value args);
    void openSettings();
    void openAbout();
    void loadPreset();
    void openHelp();
    // The start folder of a file dialog of `kind` ([folders], LIB-9 AC-29).
    QString startFolder(const std::string& kind) const;
    void rememberFolder(const std::string& kind, const QString& file);
    const ShellChoosers& choosers() const { return choosers_; }

private:
    void refresh(const QString& topic);
    void refreshLoading();
    void showNotice();
    void message(const sessions::Message&);
    void askPresetList();

    QMainWindow& window_;
    SessionHost& host_;
    shell::ShellProduct product_;
    shell::DialogSpec spec_;
    ShellOptions options_;
    ShellChoosers choosers_;
    shell::ShellInputs inputs_;
    std::vector<shell::Menu> menus_;
    shell::LoadingRule loading_;
    shell::NoticeQueue noticeQueue_;

    ShellMenus* menuBar_;
    ShellControlBar* bar_;
    PanelArea* panels_;
    QWidget* romsMissing_;
    ShellLoading* loadingView_ = nullptr;
    ShellNotices* noticeView_;
    ShellSettingsDialog* settings_ = nullptr;
    ShellAbout* about_ = nullptr;
    QWidget* faceplate_ = nullptr;
    FaceplateInput* faceplateInput_ = nullptr;
    shell::HitMap hitMap_;
    std::function<double()> scale_;
    QTimer* loadingTimer_;
    std::string runningModel_;
};

} // namespace calcu::qt6
