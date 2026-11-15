#import "TIAppSession.h"
#import "BridgeConvert.h"
#import "TIFileFormats+Cpp.h"
#include <calcu59/app/PrinterFont.hpp>
#include <calcu59/app/ProductRoms.hpp>
#include <calcu59/app/KeyboardLayout.hpp>
#include <calcu59/app/SessionThread.hpp>
#include <calcu59/app/TIAppPart.hpp>
#if DEBUG && (TARGET_OS_OSX || TARGET_OS_SIMULATOR)
#include <calcu59/app/HarnessProduct.hpp>
#include "harness/attach/AttachPort.hpp"
#endif
#include "roms/Bundle.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/TopicMirror.hpp"
#include <memory>

// ── Value objects ────────────────────────────────────────────────────────────

@interface TIPrinterLine ()
@property (nonatomic, readwrite, copy) NSData *codes;
@property (nonatomic, readwrite) BOOL complete;
@property (nonatomic, readwrite) NSInteger drawnRows;
@property (nonatomic, readwrite, copy) NSData *inkedColumns;
@property (nonatomic, readwrite, copy) NSData *faintColumns;
@property (nonatomic, readwrite) CGFloat faintAlpha;
@end
@implementation TIPrinterLine
@end

@interface TIStepEntry ()
@property (nonatomic, readwrite) NSInteger stepNum;
@property (nonatomic, readwrite) uint8_t keycode;
@property (nonatomic, readwrite, copy) NSString *mnemonic;
@property (nonatomic, readwrite) BOOL isCurrent;
@end
@implementation TIStepEntry
@end

@interface TIRegEntry ()
@property (nonatomic, readwrite) NSInteger num;
@property (nonatomic, readwrite) double value;
@property (nonatomic, readwrite) BOOL isHidden;
@end
@implementation TIRegEntry
@end

@interface TILiveDebugSnapshot ()
@property (nonatomic, readwrite, copy) NSArray<TIRegEntry *> *nonZeroRegs;
@property (nonatomic, readwrite) NSInteger dataRegCount;
@property (nonatomic, readwrite) NSInteger programRegCount;
@property (nonatomic, readwrite, copy) NSArray<TIStepEntry *> *programWindow;
@property (nonatomic, readwrite) NSInteger currentStep;
@property (nonatomic, readwrite, copy) NSArray<NSNumber *> *hir;
@property (nonatomic, readwrite) double tRegister;
@property (nonatomic, readwrite) double xValue;
@property (nonatomic, readwrite, copy) NSString *xRaw;
@property (nonatomic, readwrite, copy) NSArray<NSNumber *> *calcFlags;
@property (nonatomic, readwrite, copy) NSString *fixIndicator;
@property (nonatomic, readwrite, copy) NSString *statusRegister;
@property (nonatomic, readwrite) uint16_t fA;
@property (nonatomic, readwrite) uint16_t fB;
@property (nonatomic, readwrite, copy) NSString *engIndicator;
@property (nonatomic, readwrite, copy) NSString *secondIndicator;
@property (nonatomic, readwrite, copy) NSString *invIndicator;
@property (nonatomic, readwrite) NSInteger angleMode;
@property (nonatomic, readwrite) uint8_t prSourceFlag;
@property (nonatomic, readwrite) NSInteger programSource;
@property (nonatomic, readwrite) NSInteger pendingOpsCount;
@property (nonatomic, readwrite, copy) NSArray<NSString *> *scomRows;
@property (nonatomic, readwrite, copy) NSData *returnAddressSourceFlags;
@property (nonatomic, readwrite, copy) NSArray<NSNumber *> *returnAddresses;
@end
@implementation TILiveDebugSnapshot
@end

@interface TIInspectorEntry ()
@property (nonatomic, readwrite) uint16_t pc;
@property (nonatomic, readwrite) uint16_t opcode;
@property (nonatomic, readwrite, copy) NSString *disasm;
@property (nonatomic, readwrite) TICpuFrame frame;
@property (nonatomic, readwrite) BOOL isHistory;
@property (nonatomic, readwrite) BOOL isCurrent;
@end
@implementation TIInspectorEntry
@end

namespace {

/// The bundle folder with one file left out (TIRomSet's `leavingOut`).
class LeavingOut final : public calcu::roms::Bundle {
public:
    LeavingOut(const calcu::roms::Bundle& b, std::string name) : bundle_(b), name_(std::move(name)) {}
    std::optional<std::vector<std::uint8_t>> read(std::string_view fileName) const override {
        return fileName == name_ ? std::nullopt : bundle_.read(fileName);
    }
    std::string where() const override { return bundle_.where(); }

private:
    const calcu::roms::Bundle& bundle_;
    std::string name_;
};

NSArray<TIStepEntry*>* stepEntries(const std::vector<cu59::StepEntry>& v) {
    NSMutableArray* a = [NSMutableArray arrayWithCapacity:v.size()];
    for (const auto& e : v) {
        TIStepEntry* t = [[TIStepEntry alloc] init];
        t.stepNum = e.stepNum;
        t.keycode = e.keycode;
        t.mnemonic = ns(e.mnemonic);
        t.isCurrent = e.isCurrent;
        [a addObject:t];
    }
    return a;
}

TILiveDebugSnapshot* snapshotFrom(const cu59::LiveDebugSnapshot& s) {
    TILiveDebugSnapshot* t = [[TILiveDebugSnapshot alloc] init];
    NSMutableArray* regs = [NSMutableArray arrayWithCapacity:s.nonZeroRegs.size()];
    for (const auto& r : s.nonZeroRegs) {
        TIRegEntry* e = [[TIRegEntry alloc] init];
        e.num = r.num;
        e.value = r.value;
        e.isHidden = r.isHidden;
        [regs addObject:e];
    }
    t.nonZeroRegs = regs;
    t.dataRegCount = s.dataRegCount;
    t.programRegCount = s.programRegCount;
    t.programWindow = stepEntries(s.programWindow);
    t.currentStep = s.currentStep;
    NSMutableArray* hir = [NSMutableArray arrayWithCapacity:8];
    for (double v : s.hir) [hir addObject:@(v)];
    t.hir = hir;
    t.tRegister = s.tRegister;
    t.xValue = s.xValue;
    t.xRaw = ns(s.xRaw);
    NSMutableArray* flags = [NSMutableArray arrayWithCapacity:10];
    for (bool f : s.calcFlags) [flags addObject:@(f)];
    t.calcFlags = flags;
    t.fixIndicator = ns(s.fixIndicator);
    t.statusRegister = ns(s.statusRegister);
    t.fA = s.fA;
    t.fB = s.fB;
    t.engIndicator = ns(s.engIndicator);
    t.secondIndicator = ns(s.secondIndicator);
    t.invIndicator = ns(s.invIndicator);
    t.angleMode = s.angleMode ? static_cast<NSInteger>(*s.angleMode) : -1;
    t.prSourceFlag = s.prSourceFlag;
    t.programSource = static_cast<NSInteger>(s.programSource);
    t.pendingOpsCount = s.pendingOpsCount;
    NSMutableArray* rows = [NSMutableArray arrayWithCapacity:16];
    for (const auto& r : s.scomRows) [rows addObject:ns(r)];
    t.scomRows = rows;
    t.returnAddressSourceFlags = [NSData dataWithBytes:s.returnAddressSourceFlags.data() length:6];
    NSMutableArray* ra = [NSMutableArray arrayWithCapacity:6];
    for (int a : s.returnAddresses) [ra addObject:@(a)];
    t.returnAddresses = ra;
    return t;
}

NSArray<TIInspectorEntry*>* inspectorEntries(const std::vector<cu59::InspectorEntry>& v) {
    NSMutableArray* out = [NSMutableArray arrayWithCapacity:v.size()];
    for (const auto& e : v) {
        TIInspectorEntry* t = [[TIInspectorEntry alloc] init];
        t.pc = e.pc;
        t.opcode = e.opcode;
        t.disasm = ns(e.disasm);
        t.frame = tiCpuFrame(e.frame);
        t.isHistory = e.isHistory;
        t.isCurrent = e.isCurrent;
        [out addObject:t];
    }
    return out;
}

namespace cs = calcu::sessions;

/// The card and constant-memory files as the app had them (C59-5 D-6): read
/// and written through NSFileCoordinator; a card that iCloud has not
/// downloaded yet (`.name.icloud`) is started downloading and reads as there
/// but not readable. On the session thread.
class CoordinatedFiles final : public cu59::FileIO {
public:
    explicit CoordinatedFiles(id<TIAppSessionDelegate> d) : delegate_(d) {}
    Read read(const std::string& path) override {
        NSURL* url = [NSURL fileURLWithPath:ns(path)];
        __block NSData* data = nil;
        NSError* err = nil;
        [[[NSFileCoordinator alloc] init] coordinateReadingItemAtURL:url options:0 error:&err
                                                          byAccessor:^(NSURL* src) { data = [NSData dataWithContentsOfURL:src]; }];
        if (data) {
            auto* p = static_cast<const uint8_t*>(data.bytes);
            return {std::vector<uint8_t>(p, p + data.length)};
        }
        NSString* placeholder = [[url URLByDeletingLastPathComponent]
            URLByAppendingPathComponent:[NSString stringWithFormat:@".%@.icloud", url.lastPathComponent]].path;
        const bool exists = [[NSFileManager defaultManager] fileExistsAtPath:url.path];
        if (!exists && [[NSFileManager defaultManager] fileExistsAtPath:placeholder]) {
            [[NSFileManager defaultManager] startDownloadingUbiquitousItemAtURL:url error:nil];
            return {std::nullopt, true};
        }
        return {std::nullopt, exists};
    }
    bool write(const std::string& path, const std::string& text) override {
        NSData* fileData = [NSData dataWithBytes:text.data() length:text.size()];
        NSURL* url = [NSURL fileURLWithPath:ns(path)];
        __block BOOL ok = NO;
        NSError* err = nil;
        [[[NSFileCoordinator alloc] init] coordinateWritingItemAtURL:url options:NSFileCoordinatorWritingForReplacing
                                                               error:&err byAccessor:^(NSURL* dst) {
            ok = [fileData writeToURL:dst atomically:YES];
            // A dropped card outside our folders: the sandbox may grant the
            // file but not its folder, where an atomic write puts its temp file.
            if (!ok) ok = [fileData writeToURL:dst atomically:NO];
        }];
        if (!ok || err) return false;
        __weak id<TIAppSessionDelegate> d = delegate_;
        if (!url.path.lastPathComponent || ![url.lastPathComponent isEqualToString:@"ti58c.mem"])
            dispatch_async(dispatch_get_main_queue(), ^{ [d cardWritten]; });
        return true;
    }

private:
    __weak id<TIAppSessionDelegate> delegate_;
};

TIChange changeOf(const cs::TopicName& t) {
    namespace T = cu59::topics;
    if (t == T::kModel) return static_cast<TIChange>(TIChangeModel | TIChangeCard);
    if (t == T::kLed) return TIChangeDisplay;
    if (t == T::kCueCard) return TIChangeCueCard;
    if (t == T::kPrinter) return TIChangePrinter;
    if (t == T::kCardReader) return TIChangeCard;
    if (t == T::kRunState) return TIChangeRunState;
    if (t == T::kTrace) return TIChangeTrace;
    if (t == T::kDebugLive) return static_cast<TIChange>(TIChangeLive | TIChangeListing);
    if (t == T::kDebugCpu) return TIChangeCpu;
    if (t == T::kDebugHeatmap) return TIChangeHeatmap;
    if (t == T::kDebugLog) return TIChangeLog;
    if (t == T::kDebugAsm) return TIChangeAsm;
    return static_cast<TIChange>(0);
}

// LIB-3 AC-26's services by name (the names of library/web/glue/MessageCodec.cpp).
constexpr std::pair<cs::HostService, const char*> kServiceNames[] = {
    {cs::HostService::AudioSink, "audio-sink"},       {cs::HostService::ClipboardText, "clipboard-text"},
    {cs::HostService::ClipboardImage, "clipboard-image"}, {cs::HostService::FilePicker, "file-picker"},
    {cs::HostService::FolderPicker, "folder-picker"}, {cs::HostService::KeepFile, "keep-file"},
    {cs::HostService::OpenUrl, "open-url"},           {cs::HostService::Haptics, "haptics"},
};

// C59-5 AC-7: the host services of the app; no audio sink, the key click is
// the app's own (LIB-3 AC-33).
cs::HostServices statedServices() {
    cs::HostServices services;
#if TARGET_OS_OSX
    services.offered = {cs::HostService::ClipboardText, cs::HostService::ClipboardImage, cs::HostService::FilePicker,
                        cs::HostService::FolderPicker, cs::HostService::KeepFile, cs::HostService::OpenUrl};
#else
    services.offered = {cs::HostService::ClipboardText, cs::HostService::ClipboardImage, cs::HostService::FilePicker,
                        cs::HostService::KeepFile, cs::HostService::OpenUrl, cs::HostService::Haptics};
#endif
    return services;
}

std::vector<cs::Value::Member> argsWithBytes(std::vector<cs::Value::Member> m, NSData* raw) {
    if (raw) m.push_back({"bytes", cs::Value(bytes(raw))});
    return m;
}

} // namespace

@implementation TIRomSet {
@public
    std::shared_ptr<const calcu::roms::RomSet> _set;
}

+ (TIRomSet *)lookUpInFolder:(NSString *)folder leavingOut:(NSString *)leavingOut missing:(NSString **)missing {
    const calcu::roms::DirectoryBundle bundle(std::filesystem::path(folder.fileSystemRepresentation));
    const LeavingOut without(bundle, leavingOut ? cpp(leavingOut) : std::string());
    auto r = cu59::lookUpRoms(leavingOut ? static_cast<const calcu::roms::Bundle&>(without) : bundle);
    if (!r.ok()) {
        if (missing) *missing = ns("Calc-U-59 cannot start.\n\n" + calcu::roms::describe(r.missing));
        return nil;
    }
    TIRomSet* set = [[TIRomSet alloc] init];
    set->_set = std::move(r.set);
    return set;
}

@end


@implementation TISessionStart
@end

static cu59::SessionThread::ReplyHandler flagReply(void (^done)(BOOL)) {
    if (!done) return {};
    return [done](const cs::ReplyMessage& r) {
        const auto* f = cs::payloadAs<cu59::TIFlag>(r.result);
        done(r.done && (!r.result || (f && f->value)) ? YES : NO);
    };
}

@implementation TIAppSession {
    __weak id<TIAppSessionDelegate> _delegate;
    std::unique_ptr<cu59::SessionThread> _thread;
    cs::TopicMirror _mirror;
    size_t _printerLines;
}

- (instancetype)initWithRoms:(TIRomSet *)roms start:(TISessionStart *)start delegate:(id<TIAppSessionDelegate>)delegate {
    if ((self = [super init])) {
        _delegate = delegate;
        cu59::TIAppSetup setup;
        setup.roms = roms->_set;
        setup.cuecards = cpp(start.cuecards);
        setup.moduleID = cpp(start.moduleID);
        setup.startModel = static_cast<cu59::Model>(start.model);
        setup.saveFolder = cpp(start.saveFolder);
        setup.fileIO = std::make_shared<CoordinatedFiles>(delegate);
        setup.trace = {std::filesystem::path(start.traceDirectory.fileSystemRepresentation), start.traceMaxBytes};
        const bool traceAvailable = start.traceAvailable == YES;
        setup.traceAvailable = [traceAvailable] { return traceAvailable; };
        // C59-5 AC-7 (the session asks none of them yet).
        const cs::HostServices services = statedServices();
        __weak TIAppSession* weakSelf = self;
        _thread = std::make_unique<cu59::SessionThread>(
            [setup] { return cu59::makeAppSession(setup, cu59::steadyWallTime).session; },
            services, cu59::steadyWallTime,
            // One hop to the main thread per batch (FS 3); a batch that comes
            // after the bridge is gone is dropped.
            [weakSelf] { dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf drain]; }); });
    }
    return self;
}

- (void)dealloc { _thread.reset(); }

- (const cu59::TIPrinterState *)printer { return _mirror.stateOf<cu59::TIPrinterState>(cu59::topics::kPrinter); }
- (const cu59::TIRunState *)run { return _mirror.stateOf<cu59::TIRunState>(cu59::topics::kRunState); }
- (const cu59::TICardReaderState *)card { return _mirror.stateOf<cu59::TICardReaderState>(cu59::topics::kCardReader); }
- (const cu59::TIDebugLiveState *)live { return _mirror.stateOf<cu59::TIDebugLiveState>(cu59::topics::kDebugLive); }
- (const cu59::TIDebugLogState *)logState { return _mirror.stateOf<cu59::TIDebugLogState>(cu59::topics::kDebugLog); }
- (const cu59::TIDebugAsmState *)asmState { return _mirror.stateOf<cu59::TIDebugAsmState>(cu59::topics::kDebugAsm); }
- (const cu59::TIModelState *)modelState { return _mirror.stateOf<cu59::TIModelState>(cu59::topics::kModel); }

- (void)drain {
    TIChange changes = static_cast<TIChange>(0);
    id<TIAppSessionDelegate> d = _delegate;
    for (auto& item : _thread->drain()) {
        const cs::Message& m = item.message;
        if (const auto* r = std::get_if<cs::ReplyMessage>(&m)) {
            if (item.onReply) item.onReply(*r);
            continue;
        }
        if (const auto* s = std::get_if<cs::StreamMessage>(&m)) {
            if (s->topic == cs::kNoticesTopic) {
                for (const auto& p : s->appended)
                    if (const auto* n = cs::payloadAs<cs::Notice>(p)) {
                        const auto parts = cu59::splitNotice(n->sentence);
                        [d showErrorTitle:ns(parts.title) message:ns(parts.message)];
                    }
                continue;
            }
            if (s->topic == cs::kLogTopic) {
                for (const auto& p : s->appended)
                    if (const auto* l = cs::payloadAs<cs::LogLine>(p)) [d diagnostic:ns(l->text)];
                continue;
            }
            if (s->topic == cu59::topics::kShowDebugPanel) {
                for (const auto& p : s->appended)
                    if (const auto* x = cs::payloadAs<cu59::TIShowDebugPanel>(p)) [d showDebugPanel:static_cast<NSInteger>(x->panel)];
                continue;
            }
        }
        if (const auto* st = std::get_if<cs::StateMessage>(&m)) changes = static_cast<TIChange>(changes | changeOf(st->topic));
        _mirror.deliver(m);
    }
    if (const auto* p = [self printer]) {
        if (p->lines.size() < _printerLines) changes = static_cast<TIChange>(changes | TIChangePaperCut);
        _printerLines = p->lines.size();
    }
    if (changes) [d changed:changes];
}

- (void)send:(const cs::CommandName&)name args:(std::vector<cs::Value::Member>)args reply:(cu59::SessionThread::ReplyHandler)reply {
    _thread->send(cs::Command{name, cs::Value(std::move(args))}, std::move(reply));
}
- (void)send:(const cs::CommandName&)name args:(std::vector<cs::Value::Member>)args {
    _thread->send(cs::Command{name, cs::Value(std::move(args))});
}
- (void)send:(const cs::CommandName&)name { _thread->send(cs::Command{name, cs::Value::object({})}); }

- (void)suspendWithCompletion:(void (^)(void))done {
    _thread->suspendCalls();
    if (done) _thread->post([done](cs::AppSession&) { dispatch_async(dispatch_get_main_queue(), done); });
}
- (void)continueSession { _thread->continueCalls(); }

#if DEBUG && (TARGET_OS_OSX || TARGET_OS_SIMULATOR)
- (BOOL)openAttachWithArguments:(NSArray<NSString *> *)arguments {
    std::vector<std::string> args;
    for (NSString* a in arguments) args.push_back(cpp(a));
    const std::uint16_t port = calcu::harness::attachPort(args);
    static std::unique_ptr<calcu::harness::HarnessProduct> product = cu59::makeHarnessProduct();
    const std::string problem = _thread->openAttach(*product, port);
    if (problem.empty()) {
        [_delegate diagnostic:[NSString stringWithFormat:@"Harness attach on 127.0.0.1:%u.", port]];
        return YES;
    }
    // EC-1: the app runs on; a line of the session's log says why.
    const std::string line = "Attach is not available on port " + std::to_string(port) + ": " + problem + ".";
    _thread->post([line](cs::AppSession& s) { s.context().log(line); });
    return NO;
}
#endif

+ (NSArray<NSString *> *)statedHostServices {
    NSMutableArray* out = [NSMutableArray array];
    const cs::HostServices s = statedServices();
    for (const auto& [service, name] : kServiceNames)
        if (s.offers(service)) [out addObject:@(name)];
    return out;
}

#if DEBUG
- (void)pressKey:(uint8_t)code completion:(void (^)(BOOL))done {
    [self send:cu59::commands::kPressKey args:{{"code", static_cast<int>(code)}}
         reply:[done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); }];
}
- (void)releaseKey:(uint8_t)code completion:(void (^)(BOOL))done {
    [self send:cu59::commands::kReleaseKey args:{{"code", static_cast<int>(code)}}
         reply:[done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); }];
}
- (void)offeredHostServices:(void (^)(NSArray<NSString *> *))done {
    _thread->post([done](cs::AppSession& s) {
        NSMutableArray* out = [NSMutableArray array];
        for (const auto& [service, name] : kServiceNames)
            if (s.context().hostOffers(service)) [out addObject:@(name)];
        dispatch_async(dispatch_get_main_queue(), ^{ done(out); });
    });
}
- (void)emulatedTime:(void (^)(uint64_t, double))done {
    _thread->ask(cs::Request{cs::kTicksRequest, cs::Value::object({})}, [done](const cs::ReplyMessage& r) {
        const auto* t = cs::payloadAs<cs::EmulatedTimeReply>(r.result);
        done(t ? t->ticks : 0, t ? t->seconds : 0);
    });
}
- (uint64_t)hostCallCount { return _thread->calls(); }
#endif

// ── Machine ──────────────────────────────────────────────────────────────────

- (void)startMachine:(NSInteger)model { [self send:cu59::commands::kStartMachine args:{{"model", static_cast<int>(model)}}]; }
- (NSInteger)model { const auto* m = [self modelState]; return m ? static_cast<NSInteger>(m->model) : 0; }
- (void)resetMachine { [self send:cu59::commands::kReset]; }
- (void)cleanResetMachine { [self send:cu59::commands::kCleanReset]; }
- (NSString *)moduleID { const auto* m = [self modelState]; return m ? ns(m->moduleID) : @""; }
- (void)changeModule:(NSString *)moduleID { [self send:cu59::commands::kChangeModule args:{{"id", cpp(moduleID)}}]; }
- (NSArray<TIModuleMetadata *> *)allModules {
    NSMutableArray* out = [NSMutableArray array];
    if (const auto* m = [self modelState])
        for (const auto& x : m->modules) [out addObject:TIModuleMetadataFrom(x)];
    return out;
}

// ── Display, keys, speed ─────────────────────────────────────────────────────

- (TIDisplaySnapshot)display {
    TIDisplaySnapshot d{};
    if (const auto* l = _mirror.stateOf<cu59::TILedState>(cu59::topics::kLed)) {
        memcpy(d.segments, l->led.segments, sizeof d.segments);
        memcpy(d.digitLevel, l->led.digitLevel, sizeof d.digitLevel);
        memcpy(d.dpLevel, l->led.dpLevel, sizeof d.dpLevel);
    }
    return d;
}
- (double)calcIndicatorOpacity {
    const auto* l = _mirror.stateOf<cu59::TILedState>(cu59::topics::kLed);
    return l ? l->calcIndicatorOpacity : 0;
}
- (void)pressKey:(uint8_t)code { [self send:cu59::commands::kPressKey args:{{"code", static_cast<int>(code)}}]; }
- (void)releaseKey:(uint8_t)code { [self send:cu59::commands::kReleaseKey args:{{"code", static_cast<int>(code)}}]; }
- (void)keyboardDown:(NSData *)codes { [self send:cu59::commands::kKeyboardDown args:{{"codes", cs::Value(bytes(codes))}}]; }
- (void)keyboardUp { [self send:cu59::commands::kKeyboardUp]; }
- (void)keyboardRelease { [self send:cu59::commands::kKeyboardRelease]; }
- (NSInteger)heldKey { const auto* r = [self run]; return r ? r->heldKey : -1; }
- (BOOL)fullSpeed { const auto* r = [self run]; return r && r->fullSpeed; }
- (void)setFullSpeed:(BOOL)on { [self send:cu59::commands::kFullSpeed args:{{"on", on == YES}}]; }
- (BOOL)keystrokesPlaying { const auto* r = [self run]; return r && r->keystrokesPlaying; }

// ── Cue card, printer ────────────────────────────────────────────────────────

- (TICueCard *)cueCard {
    const auto* c = _mirror.stateOf<cu59::TICueCardState>(cu59::topics::kCueCard);
    return c && c->card ? TICueCardFrom(*c->card) : nil;
}
- (NSInteger)printerLineCount { const auto* p = [self printer]; return p ? static_cast<NSInteger>(p->lines.size()) : 0; }
- (NSArray<NSString *> *)printerLinesFrom:(NSInteger)start {
    NSMutableArray* out = [NSMutableArray array];
    if (const auto* p = [self printer])
        for (size_t i = static_cast<size_t>(MAX(start, 0)); i < p->lines.size(); ++i) [out addObject:ns(p->lines[i])];
    return out;
}
- (NSInteger)printerCodeLineCount { const auto* p = [self printer]; return p ? static_cast<NSInteger>(p->codeLines.size()) : 0; }
- (NSArray<TIPrinterLine *> *)printerCodeLinesFrom:(NSInteger)start {
    NSMutableArray* out = [NSMutableArray array];
    const auto* p = [self printer];
    if (!p) return out;
    const auto& lines = p->codeLines;
    for (size_t i = static_cast<size_t>(MAX(start, 0)); i < lines.size(); ++i) {
        TIPrinterLine* line = [[TIPrinterLine alloc] init];
        line.codes = [NSData dataWithBytes:lines[i].codes.data() length:lines[i].codes.size()];
        const auto dots = cu59::PrinterGeometry::lineDots(&lines[i]);
        line.complete = dots.complete;
        line.drawnRows = dots.drawnRows;
        line.inkedColumns = [NSData dataWithBytes:dots.inked length:7];
        line.faintColumns = [NSData dataWithBytes:dots.faint length:7];
        line.faintAlpha = dots.faintAlpha;
        [out addObject:line];
    }
    return out;
}
- (double)printerStripHeightUnits { const auto* p = [self printer]; return p ? p->stripHeightUnits : 0; }
- (BOOL)printerConnected { const auto* p = [self printer]; return !p || p->connected; }
- (BOOL)printerTrace { const auto* p = [self printer]; return p && p->trace; }
- (void)togglePrinterConnection { [self send:cu59::commands::kTogglePrinterConnection]; }
- (void)togglePrinterTrace { [self send:cu59::commands::kTogglePrinterTrace]; }
- (void)pressPrinterPrint:(BOOL)pressed { [self send:cu59::commands::kPrinterPrint args:{{"pressed", pressed == YES}}]; }
- (void)pressPrinterAdv:(BOOL)pressed { [self send:cu59::commands::kPrinterAdv args:{{"pressed", pressed == YES}}]; }
- (void)cutPaper { [self send:cu59::commands::kCutPaper]; }

// ── Card reader ──────────────────────────────────────────────────────────────

- (BOOL)cardSwiping { const auto* c = [self card]; return c && c->state == cu59::CardState::Swiping; }
- (NSString *)cardFileName { const auto* c = [self card]; return c ? ns(c->fileName) : @"card.U59"; }
- (void)fitCard:(NSData *)raw path:(NSString *)path fileName:(NSString *)fileName completion:(void (^)(BOOL))done {
    [self send:cu59::commands::kFitCard args:argsWithBytes({{"path", cpp(path)}, {"name", cpp(fileName)}}, raw)
         reply:flagReply(done)];
}
- (void)fitBlankCard:(NSString *)savePath fileName:(NSString *)fileName completion:(void (^)(BOOL))done {
    [self send:cu59::commands::kFitBlankCard args:{{"path", cpp(savePath)}, {"name", cpp(fileName)}} reply:flagReply(done)];
}
- (BOOL)cardFitted { const auto* c = [self card]; return c && c->fitted; }
- (NSInteger)litCardCorner { const auto* c = [self card]; return c && c->litCorner ? *c->litCorner : -1; }
- (BOOL)tapCardAtX:(double)x y:(double)y {
    const auto* c = [self card];
    if (!c || !c->onShow) return NO;
    const auto side = cu59::cardCornerAt(x, y);
    if (!side) return NO;
    [self send:cu59::commands::kFeedSide args:{{"side", *side}}];
    return YES;
}
- (void)ejectCard { [self send:cu59::commands::kEjectCard]; }

// ── State files ──────────────────────────────────────────────────────────────

- (void)loadStateFile:(NSData *)raw { [self send:cu59::commands::kLoadState args:argsWithBytes({}, raw)]; }

// ── Dropped files ────────────────────────────────────────────────────────────

- (BOOL)acceptsDroppedFile:(NSString *)fileName {
    using Drop = cu59::DropAction;
    const Drop a = cu59::dropActionFor(cpp(fileName), static_cast<cu59::Model>(self.model));
    return a == Drop::LoadPreset || a == Drop::FitCard ? YES : NO;
}
- (void)openDroppedFile:(NSData *)raw path:(NSString *)path fileName:(NSString *)fileName completion:(void (^)(BOOL))done {
    [self send:cu59::commands::kOpenDropped args:argsWithBytes({{"path", cpp(path)}, {"name", cpp(fileName)}}, raw)
         reply:flagReply(done)];
}

// ── Debugger ─────────────────────────────────────────────────────────────────

- (void)setLiveDebugEnabled:(BOOL)on { [self send:cu59::commands::kLiveDebug args:{{"on", on == YES}}]; }
- (void)setCpuDebugEnabled:(BOOL)on { [self send:cu59::commands::kCpuDebug args:{{"on", on == YES}}]; }
- (BOOL)frozen { const auto* r = [self run]; return r && r->debug.frozen; }
- (TIFreezeOwner)freezeOwner {
    const auto* r = [self run];
    if (!r || !r->debug.owner) return TIFreezeOwnerNone;
    return *r->debug.owner == cu59::FreezeOwner::Cpu ? TIFreezeOwnerCpu : TIFreezeOwnerCalculator;
}
- (BOOL)pcChangeArmed { const auto* r = [self run]; return r && r->debug.pcChangeArmed; }
- (BOOL)scanLoopExitArmed { const auto* r = [self run]; return r && r->debug.scanLoopExitArmed; }
- (void)freeze:(TIFreezeOwner)owner {
    if (owner != TIFreezeOwnerNone) [self send:cu59::commands::kFreeze args:{{"owner", owner == TIFreezeOwnerCpu ? 0 : 1}}];
}
- (void)resume { [self send:cu59::commands::kResume]; }
- (void)armPcChange { [self send:cu59::commands::kArmPcChange]; }
- (void)armScanLoopExit { [self send:cu59::commands::kArmScanLoopExit]; }
- (void)disarmPcChange { [self send:cu59::commands::kDisarmPcChange]; }
- (void)disarmScanLoopExit { [self send:cu59::commands::kDisarmScanLoopExit]; }
- (void)stepKeycode { [self send:cu59::commands::kStepKeycode]; }
- (void)stepInstruction { [self send:cu59::commands::kStepInstruction]; }
- (TILiveDebugSnapshot *)liveSnapshot {
    const auto* l = [self live];
    return snapshotFrom(l ? l->live : cu59::LiveDebugSnapshot{});
}
- (NSArray<TIStepEntry *> *)programListing { const auto* l = [self live]; return l ? stepEntries(l->listing) : @[]; }
- (NSInteger)programListingSource { const auto* l = [self live]; return l ? static_cast<NSInteger>(l->listingSource) : 0; }
- (NSInteger)programListingCurrentIndex { const auto* l = [self live]; return l ? l->listingCurrentIndex : -1; }
- (NSArray<TIInspectorEntry *> *)cpuHistory {
    const auto* c = _mirror.stateOf<cu59::TIDebugCpuState>(cu59::topics::kDebugCpu);
    return c ? inspectorEntries(c->history) : @[];
}
- (NSData *)heatmapCounts {
    const auto* h = _mirror.stateOf<cu59::TIDebugHeatmapState>(cu59::topics::kDebugHeatmap);
    if (!h) return [NSMutableData dataWithLength:0x1400 * sizeof(uint32_t)];
    return [NSData dataWithBytes:h->counts.data() length:h->counts.size() * sizeof(uint32_t)];
}
- (void)clearHeatmap { [self send:cu59::commands::kClearHeatmap]; }

- (NSInteger)debugLevel { const auto* l = [self logState]; return l ? static_cast<NSInteger>(l->level) : 0; }
- (void)cycleDebugLevel { [self send:cu59::commands::kCycleDebugLevel]; }
- (NSString *)logText { const auto* l = [self logState]; return l ? ns(l->text) : @""; }
- (void)clearLog { [self send:cu59::commands::kClearLog]; }
- (void)dumpVars { [self send:cu59::commands::kDumpVars]; }
- (void)dumpScom { [self send:cu59::commands::kDumpScom]; }
- (void)dumpProg { [self send:cu59::commands::kDumpProg]; }
- (void)dumpMemory { [self send:cu59::commands::kDumpMemory]; }

- (BOOL)manualTrace {
    const auto* t = _mirror.stateOf<cu59::TITraceState>(cu59::topics::kTrace);
    return t && t->manualTrace;
}
- (void)setManualTrace:(BOOL)on { [self send:cu59::commands::kManualTrace args:{{"on", on == YES}}]; }

// ── ASM overlay ──────────────────────────────────────────────────────────────

- (void)loadAsm:(NSString *)text fileName:(NSString *)fileName {
    std::vector<cs::Value::Member> a{{"name", cpp(fileName)}};
    if (text) a.push_back({"text", cpp(text)});
    [self send:cu59::commands::kLoadAsm args:std::move(a)];
}
- (void)runAsm { [self send:cu59::commands::kRunAsm]; }
- (void)clearAsm { [self send:cu59::commands::kClearAsm]; }
- (NSString *)asmFileName { const auto* a = [self asmState]; return a ? ns(a->fileName) : @"No file selected"; }
- (NSInteger)asmWordCount { const auto* a = [self asmState]; return a ? a->wordCount : 0; }
- (NSString *)asmStatus { const auto* a = [self asmState]; return a ? ns(a->status) : @""; }
- (BOOL)asmActive { const auto* a = [self asmState]; return a && a->active; }

@end
