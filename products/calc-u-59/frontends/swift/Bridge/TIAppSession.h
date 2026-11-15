#import <Foundation/Foundation.h>
#import "TITypes.h"
#import "TIFileFormats.h"

NS_ASSUME_NONNULL_BEGIN

// Swift face of the library's app session (C59-5 design §3.3): Calc-U-59's
// session runs on a thread of its own (cu59::SessionThread, AC-4); this
// bridge posts commands to it and hands the messages it sends to the main
// thread, one batch at a time and in order (AC-5, AC-6). It translates types
// only (ADR 0014): what it returns is the last message of a topic, kept on
// the main thread for the view model. Main thread only.

/// The topics a batch of messages changed (the interim c59.* topics, LIB-3 59
/// design §5; until their owners' steps).
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

/// The debug panel that owns a freeze (FreezeOwner; None when running).
typedef NS_ENUM(NSInteger, TIFreezeOwner) {
    TIFreezeOwnerNone = -1,
    TIFreezeOwnerCpu = 0,
    TIFreezeOwnerCalculator = 1,
};

/// What the session's messages ask of the app (main thread): the notices
/// (an alert, LIB-3 AC-30), KEYSTROKES DebugPanel:, the log, and a batch's changes.
@protocol TIAppSessionDelegate <NSObject>
- (void)showDebugPanel:(NSInteger)panel;   ///< 0 CALCULATOR, 1 CPU, 2 LOG
- (void)showErrorTitle:(NSString *)title message:(NSString *)message;
- (void)diagnostic:(NSString *)message;
- (void)changed:(TIChange)changes;
/// A card file was written (the coordinated write succeeded).
- (void)cardWritten;
@end

/// What the session is built with (C59-5 D-12, A-12).
@interface TISessionStart : NSObject
@property (nonatomic, copy) NSString *cuecards;
@property (nonatomic, copy) NSString *moduleID;
@property (nonatomic) NSInteger model;                ///< MachineModel raw value
@property (nonatomic, copy) NSString *saveFolder;     ///< cards and ti58c.mem (CardStorage)
@property (nonatomic, copy) NSString *traceDirectory;
@property (nonatomic) uint64_t traceMaxBytes;
@property (nonatomic) BOOL traceAvailable;
@end

/// One printed line's dot codes (Core PrinterCodeLine).
@interface TIPrinterLine : NSObject
@property (nonatomic, readonly, copy) NSData *codes;   ///< 20 character codes (0–63)
@property (nonatomic, readonly) BOOL complete;        ///< all 7 rows printed, followed by the paper advance
@property (nonatomic, readonly) NSInteger drawnRows;   ///< dot rows the line takes on paper (1–7)
@property (nonatomic, readonly, copy) NSData *inkedColumns;  ///< 7 font-row masks: columns that got ink
@property (nonatomic, readonly, copy) NSData *faintColumns;  ///< 7 font-row masks: faint dots (cut early)
@property (nonatomic, readonly) CGFloat faintAlpha;          ///< ink of the faint dots (0–1)
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
/// The reply to a key command, on the main thread (AC-6: replies in order).
- (void)pressKey:(uint8_t)matrixCode completion:(void (^)(BOOL done))done;
- (void)releaseKey:(uint8_t)matrixCode completion:(void (^)(BOOL done))done;
/// The services the session sees offered by its listeners, asked on the
/// session thread (AC-7); `done` on the main thread.
- (void)offeredHostServices:(void (^)(NSArray<NSString *> *names))done;
/// The session's emulated time (the request LIB-11's `ticks` uses); main thread.
- (void)emulatedTime:(void (^)(uint64_t ticks, double seconds))done;
/// The host calls the session thread has made (AC-4, AC-8).
@property (nonatomic, readonly) uint64_t hostCallCount;
#endif

// ── Machine (model = MachineModel raw value) ─────────────────────────────────
- (void)startMachine:(NSInteger)model;
@property (nonatomic, readonly) NSInteger model;
- (void)resetMachine;
- (void)cleanResetMachine;
@property (nonatomic, readonly) NSString *moduleID;
/// Swap to a different module and soft-reset (the Settings choice).
- (void)changeModule:(NSString *)moduleID;
/// The 14 modules from their module files, in menu order (C59-6 AC-8).
@property (nonatomic, readonly) NSArray<TIModuleMetadata *> *allModules;

// ── Display, keys, speed ─────────────────────────────────────────────────────
@property (nonatomic, readonly) TIDisplaySnapshot display;
@property (nonatomic, readonly) double calcIndicatorOpacity;
- (void)pressKey:(uint8_t)matrixCode;
- (void)releaseKey:(uint8_t)matrixCode;
- (void)keyboardDown:(NSData *)codes;
- (void)keyboardUp;
- (void)keyboardRelease;
@property (nonatomic, readonly) NSInteger heldKey;   ///< row×5 + col, -1 none
@property (nonatomic) BOOL fullSpeed;
@property (nonatomic, readonly) BOOL keystrokesPlaying;

// ── Cue card, printer ────────────────────────────────────────────────────────
@property (nonatomic, readonly, nullable) TICueCard *cueCard;
@property (nonatomic, readonly) NSInteger printerLineCount;
/// Lines from index `start` on (text and dot codes are parallel; codes can lag a frame).
- (NSArray<NSString *> *)printerLinesFrom:(NSInteger)start;
@property (nonatomic, readonly) NSInteger printerCodeLineCount;
- (NSArray<TIPrinterLine *> *)printerCodeLinesFrom:(NSInteger)start;
/// The strip's height in dot pitches (for copy-as-image).
@property (nonatomic, readonly) double printerStripHeightUnits;
@property (nonatomic, readonly) BOOL printerConnected;
@property (nonatomic, readonly) BOOL printerTrace;
/// The connect button: plug the PC-100C in or out and reset the calculator.
- (void)togglePrinterConnection;
- (void)togglePrinterTrace;
- (void)pressPrinterPrint:(BOOL)pressed;
- (void)pressPrinterAdv:(BOOL)pressed;
- (void)cutPaper;

// ── Card reader ──────────────────────────────────────────────────────────────
@property (nonatomic, readonly) BOOL cardSwiping;
@property (nonatomic, readonly) NSString *cardFileName;
/// raw nil: the file could not be read. `done(YES)` on the main thread when it was fitted.
- (void)fitCard:(nullable NSData *)raw path:(NSString *)path fileName:(NSString *)fileName
     completion:(nullable void (^)(BOOL fitted))done;
- (void)fitBlankCard:(NSString *)savePath fileName:(NSString *)fileName completion:(nullable void (^)(BOOL fitted))done;
@property (nonatomic, readonly) BOOL cardFitted;
/// The side whose corner is lit (0 left, 1 right), -1 if none.
@property (nonatomic, readonly) NSInteger litCardCorner;
/// A press at canvas position (x, y), each 0…1: YES if it hit a corner of the
/// fitted card on show (a view rule, C59-5 D-5); the side is then fed.
- (BOOL)tapCardAtX:(double)x y:(double)y;
- (void)ejectCard;

// ── State files ──────────────────────────────────────────────────────────────
- (void)loadStateFile:(nullable NSData *)raw;

// ── Dropped files ────────────────────────────────────────────────────────────
/// dropAction: the file's name makes it a preset, or a card this model can fit.
- (BOOL)acceptsDroppedFile:(NSString *)fileName;
/// raw nil: the file could not be read. `done(NO)`: nothing was done.
- (void)openDroppedFile:(nullable NSData *)raw path:(NSString *)path fileName:(NSString *)fileName
             completion:(nullable void (^)(BOOL opened))done;

// ── Debugger ─────────────────────────────────────────────────────────────────
- (void)setLiveDebugEnabled:(BOOL)on;
- (void)setCpuDebugEnabled:(BOOL)on;
@property (nonatomic, readonly) BOOL frozen;
@property (nonatomic, readonly) TIFreezeOwner freezeOwner;
@property (nonatomic, readonly) BOOL pcChangeArmed;
@property (nonatomic, readonly) BOOL scanLoopExitArmed;
- (void)freeze:(TIFreezeOwner)owner;
- (void)resume;
- (void)armPcChange;
- (void)armScanLoopExit;
- (void)disarmPcChange;
- (void)disarmScanLoopExit;
- (void)stepKeycode;
- (void)stepInstruction;
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
