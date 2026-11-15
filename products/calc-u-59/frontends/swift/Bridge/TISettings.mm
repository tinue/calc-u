#import "TISettings.h"
#import "BridgeConvert.h"
#include <calcu59/app/Settings.hpp>
#include <settings/HostDirs.hpp>
#include <settings/Paths.hpp>
#include <settings/PlatformBase.hpp>
#include <settings/SettingsFile.hpp>
#include <memory>
#include <optional>

namespace cs = calcu::settings;
namespace s59 = cu59::settings;

@implementation TISettings {
    std::unique_ptr<cs::SettingsFile> _file;
    std::unique_ptr<s59::Settings> _settings;
    std::string _settingsBase;
}

- (instancetype)initWithSettingsBase:(NSString *)settingsBase {
    if ((self = [super init])) {
        cs::HostDirs dirs;
        dirs.frontend = cs::FrontendKind::Swift;
        dirs.settingsBase = cpp(settingsBase);
        _settingsBase = dirs.settingsBase;
        _file = std::make_unique<cs::SettingsFile>(
            cs::SettingsFile::open(s59::settingsDecl(), cs::settingsFilePath(s59::settingsDecl(), dirs)));
        _settings = std::make_unique<s59::Settings>(*_file);
    }
    return self;
}

- (instancetype)initDefaultsOnlyWithLaunchSettings:(NSArray<NSString *> *)assignments {
    if ((self = [super init])) {
        _file = std::make_unique<cs::SettingsFile>(cs::SettingsFile::defaultsOnly(s59::settingsDecl()));
        _settings = std::make_unique<s59::Settings>(*_file);
        for (NSString *a in assignments) s59::applyLaunchSetting(*_file, cpp(a));
    }
    return self;
}

- (NSString *)problem { return _file->problem().empty() ? nil : ns(_file->problem()); }
- (NSString *)filePath { return ns(_file->path()); }

- (NSInteger)startupModelChoice {
    const auto m = _settings->startupModelChoice();
    return m ? static_cast<NSInteger>(*m) : -1;
}
- (void)setStartupModelChoice:(NSInteger)raw {
    _settings->setStartupModelChoice(raw >= 0 && raw <= 2 ? std::optional<cu59::Model>(static_cast<cu59::Model>(raw))
                                                          : std::nullopt);
}
- (NSInteger)startupModel { return static_cast<NSInteger>(_settings->startupModel()); }
- (void)setLastUsedModel:(NSInteger)raw {
    if (raw >= 0 && raw <= 2) _settings->setLastUsedModel(static_cast<cu59::Model>(raw));
}

- (NSInteger)traceMaxFileSizeMB { return _settings->traceMaxFileSizeMB(); }
- (void)setTraceMaxFileSizeMB:(NSInteger)mb { _settings->setTraceMaxFileSizeMB(static_cast<int>(mb)); }

- (NSInteger)ledStyle { return static_cast<NSInteger>(_settings->ledStyle()); }
- (void)setLedStyle:(NSInteger)raw {
    _settings->setLedStyle(raw == 1 ? cu59::LedStyle::Classic : cu59::LedStyle::Modernized);
}

- (NSInteger)keyboardFeedback { return static_cast<NSInteger>(_settings->keyboardFeedback()); }
- (void)setKeyboardFeedback:(NSInteger)raw {
    _settings->setKeyboardFeedback(raw == 1   ? s59::KeyboardFeedback::Haptic
                                   : raw == 2 ? s59::KeyboardFeedback::Click
                                              : s59::KeyboardFeedback::Off);
}

- (NSInteger)traceLocation { return static_cast<NSInteger>(_settings->traceLocation()); }
- (void)setTraceLocation:(NSInteger)raw {
    _settings->setTraceLocation(raw == 1   ? s59::TraceLocation::ICloud
                                : raw == 2 ? s59::TraceLocation::Folder
                                           : s59::TraceLocation::AppSupport);
}

- (NSString *)traceFolder { return ns(_settings->traceFolder()); }
- (void)setTraceFolder:(NSString *)dir { _settings->setTraceFolder(cpp(dir)); }

- (BOOL)portraitDebugPage { return _settings->portraitDebugPage(); }
- (void)setPortraitDebugPage:(BOOL)on { _settings->setPortraitDebugPage(on); }

- (NSString *)traceFolderWithSaveFolder:(NSString *)saveFolder
                     applicationSupport:(NSString *)applicationSupport
                           chosenFolder:(NSString *)chosenFolder {
    cs::HostDirs dirs;
    dirs.frontend = cs::FrontendKind::Swift;
    dirs.home = cpp(NSHomeDirectory());
    dirs.settingsBase = _settingsBase;
    dirs.saveFolder = cpp(saveFolder);
    dirs.applicationSupport = cpp(applicationSupport);
    dirs.chosenTraceFolder = cpp(chosenFolder);
    return ns(cs::Paths(*_file, dirs).traceFolder().value_or(dirs.applicationSupport));
}

@end
