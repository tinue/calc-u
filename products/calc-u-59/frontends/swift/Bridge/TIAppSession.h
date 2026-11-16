#import <Foundation/Foundation.h>
#import "TITypes.h"
#import "TIFileFormats.h"
#import "TISettings.h"

NS_ASSUME_NONNULL_BEGIN

// Swift face of the library's app session (C59-5 design §3.3): Calc-U-59's
// session runs on a thread of its own (cu59::SessionThread, AC-4); this
// bridge posts commands to it and hands the messages it sends to the main
// thread, one batch at a time and in order (AC-5, AC-6). It translates types
// only (ADR 0014): what it returns is the last message of a topic, kept on
// the main thread for the view model. Main thread only.

/// The topics a batch of messages changed (the interim c59.* topics, LIB-3 59
/// design §5, until their owners' steps; since M-53 also the library's
/// `paper PC-100C` (printer: appended, paperCut: emptied), `run-state`
/// (runState) and `trace` (trace)).
typedef NS_OPTIONS(uint32_t, TIChange) {
    TIChangeModel     NS_SWIFT_NAME(model)     = 1u << 0,
    TIChangeDisplay   NS_SWIFT_NAME(display)   = 1u << 1,
    TIChangeCueCard   NS_SWIFT_NAME(cueCard)   = 1u << 2,
    TIChangePrinter   NS_SWIFT_NAME(printer)   = 1u << 3,
    TIChangePaperCut  NS_SWIFT_NAME(paperCut)  = 1u << 4,
    TIChangeCard      NS_SWIFT_NAME(card)      = 1u << 5,
    TIChangeRunState  NS_SWIFT_NAME(runState)  = 1u << 6,
    TIChangeLive      NS_SWIFT_NAME(live)      = 1u << 7,
    TIChangeCpu       NS_SWIFT_NAME(cpu)       = 1u << 8,
    TIChangeHeatmap   NS_SWIFT_NAME(heatmap)   = 1u << 9,
    TIChangeLog       NS_SWIFT_NAME(log)       = 1u << 10,
    TIChangeAsm       NS_SWIFT_NAME(asm)       = 1u << 11,
    TIChangeTrace     NS_SWIFT_NAME(trace)     = 1u << 12,
    TIChangeListing   NS_SWIFT_NAME(listing)   = 1u << 13,
};

/// The level that owns a pause (LIB-10 RunState::owner; None when running).
/// Cpu is the CPU tab's, Calculator the CALCULATOR tab's (`program`).
typedef NS_ENUM(NSInteger, TIFreezeOwner) {
    TIFreezeOwnerNone = -1,
    TIFreezeOwnerCpu = 0,
    TIFreezeOwnerCalculator = 1,
};

/// Which run-control buttons of a level can be used (LIB-10 AC-18:
/// calcu::debugger::buttonUse, library/debugger/ButtonRule.hpp).
typedef struct {
    bool pause;
    bool step;
    bool resume;
} TIButtonUse;

/// One dot of the paper (LIB-8 S-P1, until C59-8): its centre in the
/// printer's unit (the dot pitch), y down the paper from where it was last
/// empty, and its ink 0–1 (the description's grey of its darkness).
typedef struct {
    double x;
    double y;
    double ink;
} TIPaperDot;

/// What the session's messages ask of the app (main thread): the notices
/// (an alert, LIB-3 AC-30), KEYSTROKES DebugPanel:, the log, and a batch's changes.
@protocol TIAppSessionDelegate <NSObject>
- (void)showDebugPanel:(NSInteger)panel;   ///< 0 CALCULATOR, 1 CPU, 2 LOG
- (void)showErrorTitle:(NSString *)title message:(NSString *)message;
- (void)diagnostic:(NSString *)message;
- (void)changed:(TIChange)changes;
/// A file of the save folder was written (the coordinated write succeeded).
- (void)cardWritten;
@optional
/// LIB-8 AC-20: the outcome of a Copy of the paper: 0 copied, 1 text only, 2 failed.
- (void)paperCopied:(NSInteger)outcome NS_SWIFT_NAME(paperCopied(_:));
@end

/// A saved magnetic card, as Arm Card lists it (C59-7 AC-14).
@interface TISavedCard : NSObject
@property (nonatomic, readonly, copy) NSString *name;
@property (nonatomic, readonly, copy) NSString *entryID;   ///< the picker entry `media.choose` takes
@property (nonatomic, readonly) BOOL outdated;
@property (nonatomic, readonly, copy, nullable) NSString *hint;
@end

/// What the session is built with (C59-5 D-12, A-12).
@interface TISessionStart : NSObject
/// The app's settings (LIB-18 AC-17: the startup model, `last-model`, the
/// startup presets); the session's `start` reads them. Kept alive by the caller.
@property (nonatomic, strong) TISettings *settings;
/// The bundled examples (the app's folder `examples`, bundle.yaml; LIB-6 AC-31).
@property (nonatomic, copy) NSString *examplesFolder;
@property (nonatomic, copy) NSString *saveFolder;     ///< cards and ti58c.memory.yaml (CardStorage)
@property (nonatomic, copy) NSString *traceDirectory;
@property (nonatomic) uint64_t traceMaxBytes;
@property (nonatomic) BOOL traceAvailable;
@end

/// ProgramSource raw values follow the C++ enumerator order:
/// 0 userProgram, 1 solidState, 2 solidStateReturn, 3 fastMode, 4 rom.
@interface TIStepEntry : NSObject
@property (nonatomic, readonly) NSInteger stepNum;
@property (nonatomic, readonly) uint8_t keycode;
@property (nonatomic, readonly, copy) NSString *mnemonic;
@property (nonatomic, readonly) BOOL isCurrent;
@end

@interface TIRegEntry : NSObject
@property (nonatomic, readonly) NSInteger num;
@property (nonatomic, readonly) double value;
@property (nonatomic, readonly) BOOL isHidden;
@end

/// The CALCULATOR tab's snapshot (Shared/DebugSnapshots.hpp LiveDebugSnapshot).
@interface TILiveDebugSnapshot : NSObject
@property (nonatomic, readonly, copy) NSArray<TIRegEntry *> *nonZeroRegs;
@property (nonatomic, readonly) NSInteger dataRegCount;
@property (nonatomic, readonly) NSInteger programRegCount;
@property (nonatomic, readonly, copy) NSArray<TIStepEntry *> *programWindow;
@property (nonatomic, readonly) NSInteger currentStep;
@property (nonatomic, readonly, copy) NSArray<NSNumber *> *hir;          ///< HIR 1–8
@property (nonatomic, readonly) double tRegister;
@property (nonatomic, readonly) double xValue;
@property (nonatomic, readonly, copy) NSString *xRaw;
@property (nonatomic, readonly, copy) NSArray<NSNumber *> *calcFlags;    ///< 10 BOOLs
@property (nonatomic, readonly, copy) NSString *fixIndicator;
@property (nonatomic, readonly, copy) NSString *statusRegister;
@property (nonatomic, readonly) uint16_t fA;
@property (nonatomic, readonly) uint16_t fB;
@property (nonatomic, readonly, copy) NSString *engIndicator;
@property (nonatomic, readonly, copy) NSString *secondIndicator;
@property (nonatomic, readonly, copy) NSString *invIndicator;
@property (nonatomic, readonly) NSInteger angleMode;       ///< -1 none, 0 deg, 1 rad, 2 grad
@property (nonatomic, readonly) uint8_t prSourceFlag;
@property (nonatomic, readonly) NSInteger programSource;
@property (nonatomic, readonly) NSInteger pendingOpsCount;
@property (nonatomic, readonly, copy) NSArray<NSString *> *scomRows;
@property (nonatomic, readonly, copy) NSData *returnAddressSourceFlags;  ///< 6 bytes, L1–L6
@property (nonatomic, readonly, copy) NSArray<NSNumber *> *returnAddresses;
@end

/// One CPU-inspector row (frame zeroed for the predicted next instruction).
@interface TIInspectorEntry : NSObject
@property (nonatomic, readonly) uint16_t pc;
@property (nonatomic, readonly) uint16_t opcode;
@property (nonatomic, readonly, copy) NSString *disasm;
@property (nonatomic, readonly) TICpuFrame frame;
@property (nonatomic, readonly) BOOL isHistory;
@property (nonatomic, readonly) BOOL isCurrent;
@end

/// The ROM files of the app, looked up once at the start through the
/// library's ROM lookup (LIB-12; app/ProductRoms.hpp).
@interface TIRomSet : NSObject
/// The lookup over the files directly in `folder` (the bundle's resource
/// folder). nil when a file of the full set is missing or bad; `missing`
/// then holds the text to show in place of the calculator (AC-5, AC-10).
/// `leavingOut`: a file to treat as missing (DEBUG builds' UI test only).
+ (nullable TIRomSet *)lookUpInFolder:(NSString *)folder
                           leavingOut:(nullable NSString *)leavingOut
                              missing:(NSString *_Nullable *_Nullable)missing;
@end

@interface TIAppSession : NSObject

/// The session thread starts and builds the session on it; `delegate` is held weakly.
- (instancetype)initWithRoms:(TIRomSet *)roms start:(TISessionStart *)start delegate:(id<TIAppSessionDelegate>)delegate;
/// AC-8: suspend is carried out (the TI-58C memory written), then no more
/// calls; `done` on the main thread once the suspend was carried out.
- (void)suspendWithCompletion:(nullable void (^)(void))done;
/// Calls again, then continue.
- (void)continueSession NS_SWIFT_NAME(continueSession());
#if DEBUG && (TARGET_OS_OSX || TARGET_OS_SIMULATOR)
/// Harness attach on 127.0.0.1, the port `--attach-port N` names, else 59059
/// (AC-10). NO, and a line of the session's log, when it is not available (EC-1).
- (BOOL)openAttachWithArguments:(NSArray<NSString *> *)arguments;
#endif
/// The host services this frontend states to its session (C59-5 AC-7), by
/// the names of LIB-3 AC-26; the session is built with exactly these.
@property (class, nonatomic, readonly) NSArray<NSString *> *statedHostServices;
#if DEBUG
// ── For the Swift unit tests (Calc-U-59Tests, C59-5 S1, S2) ───────────────────
/// The reply to LIB-3's raw key-down / key-up (no typing: these test the
/// message order, LIB-5 AC-39), on the main thread (AC-6: replies in order).
- (void)rawKeyDown:(uint8_t)matrixCode completion:(void (^)(BOOL done))done;
- (void)rawKeyUp:(uint8_t)matrixCode completion:(void (^)(BOOL done))done;
/// The services the session sees offered by its listeners, asked on the
/// session thread (AC-7); `done` on the main thread.
- (void)offeredHostServices:(void (^)(NSArray<NSString *> *names))done;
/// The session's emulated time (the request LIB-11's `ticks` uses); main thread.
- (void)emulatedTime:(void (^)(uint64_t ticks, double seconds))done;
/// The host calls the session thread has made (AC-4, AC-8).
@property (nonatomic, readonly) uint64_t hostCallCount;
#endif

// ── Machine (model = MachineModel raw value): LIB-18's commands ──────────────
/// The model switch (`model {name}`); the start is the session's own `start`.
- (void)startMachine:(NSInteger)model;
@property (nonatomic, readonly) NSInteger model;
- (void)resetMachine;        ///< `reset`
- (void)cleanResetMachine;   ///< `cold-boot`
@property (nonatomic, readonly) NSString *moduleID;
/// Another module into slot-1 (`attach`), or none (`detach`); a power cycle follows (row 59-11).
- (void)changeModule:(NSString *)moduleID;
/// The 14 modules from their module files, in menu order (C59-6 AC-8).
@property (nonatomic, readonly) NSArray<TIModuleMetadata *> *allModules;

// ── Display, keys, speed ─────────────────────────────────────────────────────
@property (nonatomic, readonly) TIDisplaySnapshot display;
@property (nonatomic, readonly) double calcIndicatorOpacity;
// Keys are the library's typing (LIB-5): on-screen keys by matrix code, host
// keys as the host reports them (the product's keyboard map, app/TITyping,
// decides), focus loss.
- (void)screenKeyDown:(uint8_t)matrixCode;
- (void)screenKeyUp;
- (void)hostKeyDownPhysical:(NSString *)physical named:(NSString *)named produced:(uint32_t)produced
                    carried:(uint32_t)carried shift:(BOOL)shift control:(BOOL)control alt:(BOOL)alt
                    command:(BOOL)command;
- (void)hostKeyUpPhysical:(NSString *)physical;
- (void)focusLost;
/// The key drawn pressed (the last one live input holds, `held-keys`): row×5 + col, -1 none.
@property (nonatomic, readonly) NSInteger heldKey;
@property (nonatomic) BOOL fullSpeed;
/// An operation runs (a preset, a build; the topic's `keystrokes-playing`).
@property (nonatomic, readonly) BOOL keystrokesPlaying;

// ── Cue card, printer ────────────────────────────────────────────────────────
@property (nonatomic, readonly, nullable) TICueCard *cueCard;
// The PC-100C's paper (LIB-8 adapter S-P1, until C59-8): the library's topic
// `paper PC-100C`, kept here as it arrives; emptied by a cut (paperCut).
/// Its text layer: one line per printed line or feed ("" for a feed).
@property (nonatomic, readonly) NSInteger paperLineCount;
- (NSArray<NSString *> *)paperLinesFrom:(NSInteger)start NS_SWIFT_NAME(paperLines(from:));
/// Its dots in the order made, from index `start` on: an array of TIPaperDot.
@property (nonatomic, readonly) NSInteger paperDotCount;
- (NSData *)paperDotsFrom:(NSInteger)start NS_SWIFT_NAME(paperDots(from:));
/// How far the paper reaches, in the unit (0 while empty).
@property (nonatomic, readonly) double paperReachUnits;
/// Copy (the library makes the picture and asks for the clipboard) and Cut
/// (a copy, then the paper emptied), LIB-8 AC-16, AC-18.
- (void)copyPaper;
- (void)cutPaper;
@property (nonatomic, readonly) BOOL printerConnected;
@property (nonatomic, readonly) BOOL printerTrace;
/// The connect button: plug the PC-100C in or out and reset the calculator.
- (void)togglePrinterConnection;
/// PRINT, ADV (held) and TRACE (a switch) of the PC-100C (`set-control`, LIB-8 P-E2).
- (void)setPrinterControl:(NSString *)control on:(BOOL)on NS_SWIFT_NAME(setPrinterControl(_:on:));

// ── Magnetic cards (C59-7; the library's `media` topic and `media.*`
// commands for the place `card-reader`, LIB-7 59, M-55) ─────────────────────
/// A side of the fitted card passes through the reader now.
@property (nonatomic, readonly) BOOL cardPassing;
/// A magnetic card is fitted (armed, or given back after a pass).
@property (nonatomic, readonly) BOOL cardFitted;
/// The fitted card's name, nil when none.
@property (nonatomic, readonly, nullable) NSString *fittedCardName;
/// The saved cards of the save folder by the names inside them, as Arm Card
/// lists them (C59-7 AC-14): an outdated one cannot be chosen and has a hint.
@property (nonatomic, readonly) NSArray<TISavedCard *> *savedCards;
/// Arm Card: the saved card fitted for the user to feed (`media.choose`).
- (void)armCard:(TISavedCard *)card;
/// Arm Fresh Card (AC-15, `media.start-new` from the fresh card): `question`
/// non-nil when a card of that name is saved (ask, then call again with
/// confirmed YES); `refusal` when it was refused for another reason.
- (void)armFreshCard:(NSString *)name confirmed:(BOOL)confirmed
          completion:(nullable void (^)(NSString *_Nullable question, NSString *_Nullable refusal))done;
/// The side whose corner is lit (0 left, 1 right), -1 if none.
@property (nonatomic, readonly) NSInteger litCardCorner;
/// A press at canvas position (x, y), each 0…1: YES if it hit a corner of the
/// fitted card on show (a view rule, C59-5 D-5); the side is then fed.
- (BOOL)tapCardAtX:(double)x y:(double)y;
/// Eject Card: a passing card is taken out and stays fitted (`media.eject`).
- (void)ejectCard;

// ── Presets (LIB-6 `load-preset {content, name}`, D-41) ─────────────────────
- (void)loadPresetContent:(nullable NSData *)raw name:(NSString *)name;

// ── Dropped files ────────────────────────────────────────────────────────────
/// dropAction: the file's name makes it a preset, or a card this model can fit.
- (BOOL)acceptsDroppedFile:(NSString *)fileName;
/// raw nil: the file could not be read. `done(NO)`: nothing was done.
- (void)openDroppedFile:(nullable NSData *)raw path:(NSString *)path fileName:(NSString *)fileName
             completion:(nullable void (^)(BOOL opened))done;

// ── Debugger ─────────────────────────────────────────────────────────────────
- (void)setLiveDebugEnabled:(BOOL)on;
- (void)setCpuDebugEnabled:(BOOL)on;
// The run control (LIB-10 59; the library's `run-state` and its commands):
// level Cpu is `cpu`, Calculator `program`.
@property (nonatomic, readonly) BOOL frozen;            ///< paused
@property (nonatomic, readonly) TIFreezeOwner freezeOwner;
@property (nonatomic, readonly) BOOL pcChangeArmed;     ///< pause-on-start program
@property (nonatomic, readonly) BOOL scanLoopExitArmed; ///< pause-on-start cpu
/// The buttons of a level by the library's rule (AC-18, AC-34).
- (TIButtonUse)buttonUseFor:(TIFreezeOwner)level NS_SWIFT_NAME(buttonUse(for:));
/// The rule itself, for a run state given by its fields (the unit tests).
+ (TIButtonUse)buttonUseFor:(TIFreezeOwner)level hasState:(BOOL)hasState paused:(BOOL)paused
                      owner:(TIFreezeOwner)owner NS_SWIFT_NAME(buttonUse(for:hasState:paused:owner:));
- (void)pause:(TIFreezeOwner)level NS_SWIFT_NAME(pause(_:));
- (void)step:(TIFreezeOwner)level NS_SWIFT_NAME(step(_:));
- (void)resume;
- (void)pauseOnStart:(TIFreezeOwner)level NS_SWIFT_NAME(pause(onStart:));   ///< None: off
@property (nonatomic, readonly) TILiveDebugSnapshot *liveSnapshot;
@property (nonatomic, readonly) NSArray<TIStepEntry *> *programListing;
@property (nonatomic, readonly) NSInteger programListingSource;
@property (nonatomic, readonly) NSInteger programListingCurrentIndex;   ///< -1 none
@property (nonatomic, readonly) NSArray<TIInspectorEntry *> *cpuHistory;
@property (nonatomic, readonly) NSData *heatmapCounts;   ///< 0x1400 uint32 counts
- (void)clearHeatmap;

@property (nonatomic, readonly) NSInteger debugLevel;   ///< 0 off, 1 info, 2 debug
- (void)cycleDebugLevel;
@property (nonatomic, readonly) NSString *logText;
- (void)clearLog;
- (void)dumpVars;
- (void)dumpScom;
- (void)dumpProg;
- (void)dumpMemory;

/// A trace of the library's trace session runs (the TRACE button's, AC-23).
@property (nonatomic, readonly) BOOL manualTrace;
- (void)setManualTrace:(BOOL)on;

// ── ASM overlay ──────────────────────────────────────────────────────────────
/// text nil: the file could not be read.
- (void)loadAsm:(nullable NSString *)text fileName:(NSString *)fileName;
- (void)runAsm;
- (void)clearAsm;
@property (nonatomic, readonly) NSString *asmFileName;
@property (nonatomic, readonly) NSInteger asmWordCount;
@property (nonatomic, readonly) NSString *asmStatus;
@property (nonatomic, readonly) BOOL asmActive;

@end

NS_ASSUME_NONNULL_END
