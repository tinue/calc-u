#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// The library's shell models for the Swift frontend: the menus, the More button's items, the control bar, the settings
// dialog, "Loading…", the notice queue and the pointer rule on the
// faceplate, computed by library/shell/ from the last Value of each input
// topic of the session (control-state, busy, run-state, settings, media, the
// preset list) with Calc-U-59's data (app/TIShellProduct). The bridge
// decides nothing: Swift draws what comes back and sends an item's command
// through `choose:`. Main thread only; the inputs change with the session's
// batches (TIChangeShell).

@class TIAppSession;

typedef NS_ENUM(NSInteger, TIMenuItemKind) {
    TIMenuItemKindAction,
    TIMenuItemKindSubmenu,
    TIMenuItemKindSeparator,
    TIMenuItemKindHeading,   ///< a menu's name in More
};

/// A menu item (calcu::shell::MenuItem).
@interface TIMenuItem : NSObject
@property (nonatomic, readonly) TIMenuItemKind kind;
@property (nonatomic, readonly, copy) NSString *itemID;   ///< stable: `item-<id>`, `more-item-<id>`
@property (nonatomic, readonly, copy) NSString *label;
@property (nonatomic, readonly) BOOL enabled;
@property (nonatomic, readonly) BOOL checked;
@property (nonatomic, readonly, copy) NSString *reason;   ///< why it cannot be used now
/// A question asked before it is carried out (Cold boot behind More), "" for none.
@property (nonatomic, readonly, copy) NSString *confirm;
@property (nonatomic, readonly) BOOL destructive;
@property (nonatomic, readonly, copy) NSArray<TIMenuItem *> *children;
/// The command it sends, "" for a shell action (load-preset, settings, help,
/// about, quit) or a product item (arm-card, arm-fresh-card).
@property (nonatomic, readonly, copy) NSString *command;
@end

@interface TIMenu : NSObject
@property (nonatomic, readonly, copy) NSString *menuID;   ///< file, machine, help
@property (nonatomic, readonly, copy) NSString *label;
@property (nonatomic, readonly, copy) NSArray<TIMenuItem *> *items;
@end

typedef NS_ENUM(NSInteger, TIBarControlKind) {
    TIBarControlKindModelPicker,
    TIBarControlKindSlotPicker,
    TIBarControlKindPlaceButton,
    TIBarControlKindPlacePicker,
    TIBarControlKindMedia,
    TIBarControlKindMore,
};

typedef NS_ENUM(NSInteger, TIBarGroup) {
    TIBarGroupAfterModel,
    TIBarGroupSlots,
    TIBarGroupPlaces,
    TIBarGroupMedia,
    TIBarGroupEnd,
};

/// The bar below the faceplate (calcu::shell::barFit): its width and whether
/// it shows its labels.
@interface TIBarFit : NSObject
@property (nonatomic, readonly) CGFloat width;
@property (nonatomic, readonly) BOOL labels;
+ (TIBarFit *)fitForFaceplate:(CGFloat)faceplate withoutLabels:(CGFloat)without withLabels:(CGFloat)with
    NS_SWIFT_NAME(fit(faceplate:withoutLabels:withLabels:));
@end

/// A control of the bar (calcu::shell::BarControl).
@interface TIBarControl : NSObject
@property (nonatomic, readonly) TIBarControlKind kind;
@property (nonatomic, readonly) TIBarGroup group;
@property (nonatomic, readonly, copy) NSString *controlID;   ///< "model", a place, a media place
@property (nonatomic, readonly, copy) NSString *label;
@property (nonatomic, readonly) BOOL plugged;                ///< a place button: shown as pressed
/// A picker: what it shows closed where `shortShown` (the model's bar name, the chosen ID, "—").
@property (nonatomic, readonly, copy) NSString *shown;
@property (nonatomic, readonly) BOOL shortShown;
/// Its group takes up the room before it (a spread bar: the media group, End).
@property (nonatomic, readonly) BOOL groupPushed;
/// A picker: its entries (the running one checked); a button: its one choice; More: its items.
@property (nonatomic, readonly, copy) NSArray<TIMenuItem *> *choices;
@end

typedef NS_ENUM(NSInteger, TIDialogWidget) {
    TIDialogWidgetStartupModel,
    TIDialogWidgetStartupPreset,
    TIDialogWidgetFolder,
    TIDialogWidgetInteger,
    TIDialogWidgetChoice,
    TIDialogWidgetText,
    TIDialogWidgetNote,
};

/// A row of the settings dialog as shown now (calcu::shell::ShownRow).
@interface TIDialogRow : NSObject
@property (nonatomic, readonly) TIDialogWidget widget;
@property (nonatomic, readonly, copy) NSString *section;
@property (nonatomic, readonly, copy) NSString *key;
@property (nonatomic, readonly, copy) NSString *label;
@property (nonatomic, readonly, copy) NSString *value;
@property (nonatomic, readonly, copy) NSString *shown;   ///< a folder with `~`
@property (nonatomic, readonly, copy) NSArray<NSString *> *choiceValues;
@property (nonatomic, readonly, copy) NSArray<NSString *> *choiceLabels;
/// A choice of `true` and `false`, drawn as a Toggle.
@property (nonatomic, readonly) BOOL isToggle;
/// A startup preset row: the label of its last entry ("Choose…"), which is
/// no value but opens the file importer; empty: none.
@property (nonatomic, readonly, copy) NSString *chooseLabel;
@end

@interface TIDialogSection : NSObject
@property (nonatomic, readonly, copy) NSString *title;
@property (nonatomic, readonly, copy) NSArray<TIDialogRow *> *rows;
@end

/// The ids of the shell's own actions (calcu::shell::actions).
extern NSString *const TIShellActionLoadPreset;
extern NSString *const TIShellActionSettings;
extern NSString *const TIShellActionHelp;
extern NSString *const TIShellActionShowAppLog;

@interface TIShell : NSObject

- (instancetype)init NS_UNAVAILABLE;

/// A control state came (before it, the bar is empty).
@property (nonatomic, readonly) BOOL started;
/// File, Machine, Help with their items.
@property (nonatomic, readonly) NSArray<TIMenu *> *menus;
/// The items of the More button.
@property (nonatomic, readonly) NSArray<TIMenuItem *> *moreItems;
/// The bar, group by group, left to right.
@property (nonatomic, readonly) NSArray<TIBarControl *> *bar;
/// The sections and rows of the settings, with their values now.
@property (nonatomic, readonly) NSArray<TIDialogSection *> *settingsDialog;
/// The running model (the control state's `model`), "" before the start.
@property (nonatomic, readonly) NSString *modelID;
/// The paper panel's owner (the paper panel shows its paper), "" while none is attached.
@property (nonatomic, readonly) NSString *paperOwner;
/// What the paper panel shows where the paper would start while none is attached, else "".
@property (nonatomic, readonly) NSString *paperHint;
/// The place controls on the paper panel (the product's placesOnPaper: the PC-100C), group Places.
@property (nonatomic, readonly) NSArray<TIBarControl *> *paperPlaces;
/// A setting's value now (the `settings` topic), "" when it has none.
- (NSString *)settingValueInSection:(NSString *)section key:(NSString *)key NS_SWIFT_NAME(setting(_:_:));
/// A setting changed, applied and written at once (`set-setting`); nil: its default.
- (void)setSetting:(NSString *)section key:(NSString *)key value:(nullable NSString *)value
    NS_SWIFT_NAME(setSetting(_:_:value:));

/// The item by id, depth first, in the menus or the More items.
- (nullable TIMenuItem *)itemWithID:(NSString *)itemID NS_SWIFT_NAME(item(_:));
/// Carries out an item: its command is sent (a refusal becomes a notice)
/// and nil comes back; a shell action or a product item comes
/// back as its id for the frontend (a file dialog, the settings, the help).
- (nullable NSString *)choose:(TIMenuItem *)item;

/// "Loading…": shown now; when to look again (seconds
/// from now, negative: no need).
@property (nonatomic, readonly) BOOL loadingShown;
@property (nonatomic, readonly) double loadingDueIn;

/// The notice shown now, nil when none; each in turn.
@property (nonatomic, readonly, nullable) NSString *currentNotice;
/// The notice shown is a question (Yes, No).
@property (nonatomic, readonly) BOOL currentNoticeIsQuestion;
/// OK (or No): the next one follows.
- (void)closeNotice;
/// Yes to a question: the command is sent again with `confirmed: true`.
- (void)answerNotice:(BOOL)yes;
/// A sentence of the frontend's own (a refusal it got as a reply), queued like a notice.
- (void)pushNotice:(NSString *)sentence;

/// The pointer on the faceplate (the library's PointerTracker over
/// cu59::faceplateHitMap); x, y in points of the faceplate as shown, `scale`
/// its points per pixel of the base image. YES when a key went down now
/// (the frontend's key feedback).
- (BOOL)pointerDown:(int64_t)pointer x:(double)x y:(double)y scale:(double)scale
    NS_SWIFT_NAME(pointerDown(_:x:y:scale:));
- (BOOL)pointerMove:(int64_t)pointer x:(double)x y:(double)y scale:(double)scale
    NS_SWIFT_NAME(pointerMove(_:x:y:scale:));
- (void)pointerUp:(int64_t)pointer NS_SWIFT_NAME(pointerUp(_:));
/// Every pointer lets go (the view goes away, a model switch).
- (void)releaseAllPointers;
/// The key a pointer holds now (row × 5 + col), -1 for none.
@property (nonatomic, readonly) NSInteger pointerKey;

/// The About texts and links of every frontend (TIShellProduct).
/// The version with the build number (3.0.0-0) and the build ID
/// (tools/build-id.cmake: the commit, pre- while the CHANGELOG entry is
/// open, -dirty), as the About and the Settings show them.
@property (class, nonatomic, readonly) NSArray<NSString *> *aboutLines;
@property (class, nonatomic, readonly) NSArray<NSArray<NSString *> *> *aboutLinks;   ///< [label, address]
@property (class, nonatomic, readonly) NSString *helpURL;

@end

NS_ASSUME_NONNULL_END
