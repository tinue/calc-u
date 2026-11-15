#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Swift face of app/Settings.hpp over the library's settings file
/// (calcu::settings::SettingsFile, LIB-9): Calc-U-59.ini, read once when it
/// is opened and written through at each change. Type translation only: the
/// keys, defaults and rules are app/Settings.hpp's. Main thread only.
/// Raw values: models as MachineModel (0 TI-59, 1 TI-58, 2 TI-58C; -1 =
/// last used), LED style as LEDFontStyle, feedback as KeyboardFeedbackType,
/// trace location as TraceLocation.
@interface TISettings : NSObject

/// Opens `<applicationSupport base>/Calc-U-59/Calc-U-59.ini` (LIB-9 AC-2;
/// created with every default when missing, AC-5). `problem` is set when the
/// file could not be read or written.
- (instancetype)initWithSettingsBase:(NSString *)settingsBase;
/// A file in memory, never read or written (LIB-9 AC-18): the UI tests'
/// runs, with `<section>.<key>=<value>` launch values applied.
- (instancetype)initDefaultsOnlyWithLaunchSettings:(NSArray<NSString *> *)assignments;
- (instancetype)init NS_UNAVAILABLE;

@property (readonly, nullable) NSString *problem;
@property (readonly) NSString *filePath;

/// [startup] model: -1 = last, else a MachineModel raw value.
@property (nonatomic) NSInteger startupModelChoice;
/// The model to start (the chosen one, else the last used, else TI-59).
@property (readonly) NSInteger startupModel;
- (void)setLastUsedModel:(NSInteger)model;

@property (nonatomic) NSInteger traceMaxFileSizeMB;
@property (nonatomic) NSInteger ledStyle;
@property (nonatomic) NSInteger keyboardFeedback;
@property (nonatomic) NSInteger traceLocation;
/// [trace] folder: the folder of `traceLocation = folder` (macOS).
@property (nonatomic, copy) NSString *traceFolder;
@property (nonatomic) BOOL portraitDebugPage;

/// Where traces go (calcu::settings::Paths, AC-24): `saveFolder` is the
/// iCloud or local Documents folder (CardStorage), `applicationSupport` the
/// app's folder in Application Support, `chosenFolder` the folder the
/// security-scoped bookmark grants (AC-34), or nil.
- (NSString *)traceFolderWithSaveFolder:(NSString *)saveFolder
                     applicationSupport:(NSString *)applicationSupport
                           chosenFolder:(nullable NSString *)chosenFolder;

@end

NS_ASSUME_NONNULL_END
