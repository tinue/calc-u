#import "TIAppSession.h"
#import "BridgeConvert.h"
#import "TIFileFormats+Cpp.h"
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif
#include <calcu59/core/PC100CPaper.hpp>
#include <calcu59/app/ProductRoms.hpp>
#include <calcu59/app/KeyboardLayout.hpp>
#include <calcu59/app/SessionThread.hpp>
#include <calcu59/app/TIAppPart.hpp>
#include <calcu59/app/DebugLog.hpp>
#include <calcu59/app/TIDebugPart.hpp>
#include <calcu59/app/KeyIds.hpp>
#include "typing/TypingTopics.hpp"
#if DEBUG && (TARGET_OS_OSX || TARGET_OS_SIMULATOR)
#include <calcu59/app/HarnessProduct.hpp>
#include "harness/attach/AttachPort.hpp"
#endif
#include "sessions/Args.hpp"
#include "presets/PresetsPart.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/StartupPart.hpp"
#include "debugger/ButtonRule.hpp"
#include "debugger/RunState.hpp"
#include "control/ControlState.hpp"
#include "opening/OpenRequest.hpp"
#include "paper/PaperTopics.hpp"
#include "roms/Bundle.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/TopicMirror.hpp"
#include <memory>

// ── Value objects ────────────────────────────────────────────────────────────

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
        if (!url.path.lastPathComponent || ![url.lastPathComponent isEqualToString:@"ti58c.memory.yaml"])
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
    if (t == T::kRunState || t == calcu::typing::kHeldKeysTopic || t == calcu::debugger::kRunStateTopic)
        return TIChangeRunState;
    if (t == calcu::debugger::kTraceTopic) return TIChangeTrace;
    // The debug panel's product topics (C59-4 AC-25, M-56).
    if (t == T::kCalculatorDebug) return TIChangeLive;
    if (t == T::kProgramListing) return TIChangeListing;
    if (t == T::kCpuDebug) return TIChangeCpu;
    if (t == T::kHeatmap) return TIChangeHeatmap;
    if (t == T::kAsmOverlay) return TIChangeAsm;
    if (t == T::kDebugView) return TIChangeLog;   // the log level it carries
    if (t == calcu::control::kControlStateTopic) return TIChangeControl;
    return static_cast<TIChange>(0);
}

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

// The key ID of a matrix code (row × 10 + column), "" for none.
std::string keyIdOf(uint8_t code) { return cu59::keyIdAt(code).value_or(calcu::machine::KeyId{}).value; }

std::vector<cs::Value::Member> argsWithBytes(std::vector<cs::Value::Member> m, NSData* raw) {
    if (raw) m.push_back({"bytes", cs::Value(bytes(raw))});
    return m;
}

const std::string kPC100COwner = "PC-100C";
const cs::TopicName kPaper = calcu::paper::paperTopic(kPC100COwner);

calcu::debugger::LevelName levelOf(TIFreezeOwner l) {
    return l == TIFreezeOwnerCpu ? calcu::debugger::kCpu : calcu::debugger::kProgram;
}

TIButtonUse buttonUseOf(const calcu::debugger::ButtonUse& u) {
    TIButtonUse t;
    t.pause = u.pause;
    t.step = u.step;
    t.resume = u.resume;
    return t;
}

// The host services the bridge answers (LIB-3 AC-27): the clipboard (LIB-8
// AC-16, LIB-8 D-28: the session's Copy makes the picture; the entry is the
// platform's). A picture keeps its physical size (`dotsPerInch`).
cs::ServiceAnswer answerService(const cs::ServiceAsk& ask) {
    using cs::HostService;
    if (ask.service == HostService::ClipboardImage) {
        const cs::Value* b = ask.args.find("bytes");
        if (!b || !b->bytes()) return cs::ServiceAnswer{false, cs::Value(), "No picture to copy."};
        NSData* png = [NSData dataWithBytes:b->bytes()->data() length:b->bytes()->size()];
        const cs::Value* t = ask.args.find("text");
        NSString* text = t && t->string() ? ns(*t->string()) : nil;
#if TARGET_OS_OSX
        NSImage* image = [[NSImage alloc] initWithData:png];
        if (!image) return cs::ServiceAnswer{false, cs::Value(), "The picture cannot be read."};
        const double dpi = cs::realArg(ask.args, "dotsPerInch", 0);
        NSBitmapImageRep* rep = image.representations.count ? (NSBitmapImageRep*)image.representations.firstObject : nil;
        if (dpi > 0 && rep) image.size = NSMakeSize(rep.pixelsWide * 72.0 / dpi, rep.pixelsHigh * 72.0 / dpi);
        NSPasteboard* pb = NSPasteboard.generalPasteboard;
        [pb clearContents];
        NSMutableArray<id<NSPasteboardWriting>>* objects = [NSMutableArray arrayWithObject:image];   // the picture first
        if (text) [objects addObject:text];
        if (![pb writeObjects:objects]) return cs::ServiceAnswer{false, cs::Value(), "The clipboard refused it."};
#else
        NSMutableDictionary* item = [NSMutableDictionary dictionaryWithObject:png forKey:@"public.png"];
        if (text) item[@"public.utf8-plain-text"] = text;
        UIPasteboard.generalPasteboard.items = @[item];
#endif
        return cs::ServiceAnswer{true, cs::Value(), {}};
    }
    if (ask.service == HostService::ClipboardText) {
        if (const cs::Value* t = ask.args.find("text"); t && t->string()) {
#if TARGET_OS_OSX
            [NSPasteboard.generalPasteboard clearContents];
            [NSPasteboard.generalPasteboard setString:ns(*t->string()) forType:NSPasteboardTypeString];
#else
            UIPasteboard.generalPasteboard.string = ns(*t->string());
#endif
            return cs::ServiceAnswer{true, cs::Value(), {}};
        }
#if TARGET_OS_OSX
        NSString* now = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
#else
        NSString* now = UIPasteboard.generalPasteboard.string;
#endif
        return cs::ServiceAnswer{true, cs::Value::object({{"text", now ? cpp(now) : std::string()}}), {}};
    }
    return cs::ServiceAnswer{false, cs::Value(), "This frontend does not answer " + std::string(cs::serviceName(ask.service)) + "."};
}

// A product entry of the control state (C59-4 AC-25): {available, reason}.
bool entryAvailable(const calcu::control::ControlState* c, const char* name, std::string* reason = nullptr) {
    const auto* e = c ? c->productEntry(name) : nullptr;
    if (!e || !e->value) return false;
    const cs::Value v = e->value->toValue();
    const cs::Value* a = v.find("available");
    const cs::Value* why = v.find("reason");
    if (reason && why && why->string()) *reason = *why->string();
    return a && a->boolean() && *a->boolean();
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
    TISettings* _settings;   // the settings file the session reads (TIAppSetup::settings): kept alive with it
    std::unique_ptr<cu59::SessionThread> _thread;
    cs::TopicMirror _mirror;
    // The PC-100C's paper as the library's topic sent it (S-P1).
    std::vector<std::string> _paperLines;
    std::vector<TIPaperDot> _paperDots;
    double _paperReach;
    // The LOG tab's view of LIB-3's `log` (C59-4 AC-9): the last 2000 lines.
    cu59::RingLog _log;
    std::string _logText;
    // debug-view's count of requests already shown (AC-2).
    std::uint64_t _viewRequests;
}

- (instancetype)initWithRoms:(TIRomSet *)roms start:(TISessionStart *)start delegate:(id<TIAppSessionDelegate>)delegate {
    if ((self = [super init])) {
        _delegate = delegate;
        _settings = start.settings;
        cu59::TIAppSetup setup;
        setup.roms = roms->_set;
        setup.cuecards = cpp(start.cuecards);
        // LIB-18 AC-17: the settings file decides the startup model; the
        // session's `start` (makeAppSession) builds it.
        setup.settings = static_cast<calcu::settings::SettingsFile*>([_settings cppSettingsFile]);
        setup.examplesFolder = cpp(start.examplesFolder);
        setup.home = cpp(NSHomeDirectory());
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
- (const calcu::debugger::RunState *)runControl {
    return _mirror.stateOf<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
}
- (const cu59::TICardReaderState *)card { return _mirror.stateOf<cu59::TICardReaderState>(cu59::topics::kCardReader); }
- (const cu59::TICalculatorDebug *)live { return _mirror.stateOf<cu59::TICalculatorDebug>(cu59::topics::kCalculatorDebug); }
- (const cu59::TIProgramListing *)listing { return _mirror.stateOf<cu59::TIProgramListing>(cu59::topics::kProgramListing); }
- (const cu59::TIDebugView *)debugView { return _mirror.stateOf<cu59::TIDebugView>(cu59::topics::kDebugView); }
- (const cu59::TIAsmOverlayState *)asmState { return _mirror.stateOf<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay); }
- (const calcu::control::ControlState *)controlState {
    return _mirror.stateOf<calcu::control::ControlState>(calcu::control::kControlStateTopic);
}
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
        if (const auto* a = std::get_if<cs::ServiceAsk>(&m)) {
            _thread->answer(a->ask, answerService(*a));
            continue;
        }
        if (const auto* s = std::get_if<cs::StreamMessage>(&m)) {
            if (s->topic == kPaper) {   // LIB-8 AC-6: the reset marker: the paper was emptied
                if (s->reset) {
                    _paperLines.clear();
                    _paperDots.clear();
                    _paperReach = 0;
                    changes = static_cast<TIChange>(changes | TIChangePaperCut);
                }
                const auto& greys = pc100cpaper::description().darknessGreys;
                for (const auto& p : s->appended) {
                    const auto* x = cs::payloadAs<calcu::paper::PaperAppended>(p);
                    if (!x) continue;
                    for (const auto& l : x->lines) _paperLines.push_back(l.text);
                    for (const auto& mark : x->marks) {
                        const auto* dot = std::get_if<calcu::paper::Dot>(&mark);
                        if (!dot) continue;
                        const double grey = dot->darkness < greys.size() ? greys[dot->darkness] : 0;
                        const double unit = static_cast<double>(calcu::paper::kPerUnit);
                        _paperDots.push_back(TIPaperDot{dot->at.x / unit, dot->at.y / unit, 1.0 - grey / 255.0});
                    }
                    _paperReach = static_cast<double>(x->reach.to) / static_cast<double>(calcu::paper::kPerUnit);
                }
                changes = static_cast<TIChange>(changes | TIChangePrinter);
                continue;
            }
            if (s->topic == calcu::paper::kPaperCopiesTopic) {   // AC-20
                for (const auto& p : s->appended)
                    if (const auto* o = cs::payloadAs<calcu::paper::CopyOutcome>(p);
                        o && [d respondsToSelector:@selector(paperCopied:)])
                        [d paperCopied:static_cast<NSInteger>(o->outcome)];
                continue;
            }
            if (s->topic == cs::kNoticesTopic) {
                for (const auto& p : s->appended)
                    if (const auto* n = cs::payloadAs<cs::Notice>(p)) {
                        const auto parts = cu59::splitNotice(n->sentence);
                        [d showErrorTitle:ns(parts.title) message:ns(parts.message)];
                    }
                continue;
            }
            if (s->topic == cs::kLogTopic) {   // C59-4 AC-9: the LOG tab's lines (a message may hold several)
                bool any = false;
                for (const auto& p : s->appended)
                    if (const auto* l = cs::payloadAs<cs::LogLine>(p)) {
                        [d diagnostic:ns(l->text)];
                        std::size_t from = 0;
                        for (std::size_t at; (at = l->text.find('\n', from)) != std::string::npos; from = at + 1)
                            _log.write(l->text.substr(from, at - from));
                        _log.write(l->text.substr(from));
                        any = true;
                    }
                if (any) {
                    _logText = _log.displayText();
                    changes = static_cast<TIChange>(changes | TIChangeLog);
                }
                continue;
            }
        }
        if (const auto* st = std::get_if<cs::StateMessage>(&m)) changes = static_cast<TIChange>(changes | changeOf(st->topic));
        _mirror.deliver(m);
    }
    // The tab a `debug-panel:` step asked for, each time debug-view's count grows (AC-2, EC-4).
    if (const auto* v = [self debugView]; v && v->requests > _viewRequests) {
        _viewRequests = v->requests;
        [d showDebugPanel:static_cast<NSInteger>(v->view)];
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
- (void)send:(const cs::CommandName&)name value:(cs::Value)args { _thread->send(cs::Command{name, std::move(args)}); }

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
    for (cs::HostService service : cs::kHostServices)
        if (s.offers(service)) [out addObject:@(cs::serviceName(service))];
    return out;
}

#if DEBUG
- (void)rawKeyDown:(uint8_t)code completion:(void (^)(BOOL))done {
    [self send:cs::kKeyDownCommand args:{{"key", keyIdOf(code)}}
         reply:[done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); }];
}
- (void)rawKeyUp:(uint8_t)code completion:(void (^)(BOOL))done {
    [self send:cs::kKeyUpCommand args:{{"key", keyIdOf(code)}}
         reply:[done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); }];
}
- (void)offeredHostServices:(void (^)(NSArray<NSString *> *))done {
    _thread->post([done](cs::AppSession& s) {
        NSMutableArray* out = [NSMutableArray array];
        for (cs::HostService service : cs::kHostServices)
            if (s.context().hostOffers(service)) [out addObject:@(cs::serviceName(service))];
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

- (void)startMachine:(NSInteger)model {
    if (model < 0 || model > 2) return;
    [self send:calcu::startup::kModelCommand args:{{"name", cu59::modelId(static_cast<cu59::Model>(model)).value}}];
}
- (NSInteger)model { const auto* m = [self modelState]; return m ? static_cast<NSInteger>(m->model) : 0; }
- (void)resetMachine { [self send:calcu::startup::kResetCommand]; }
- (void)cleanResetMachine { [self send:calcu::startup::kColdBootCommand]; }
- (NSString *)moduleID { const auto* m = [self modelState]; return m ? ns(m->moduleID) : @""; }
- (void)changeModule:(NSString *)moduleID {
    if ([moduleID isEqualToString:[self moduleID]]) return;
    if (moduleID.length == 0) [self send:calcu::startup::kDetachCommand args:{{"place", "slot-1"}}];
    else [self send:calcu::startup::kAttachCommand args:{{"place", "slot-1"}, {"peripheral", cpp(moduleID)}}];
}
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
- (void)screenKeyDown:(uint8_t)code {
    [self send:calcu::typing::kScreenKeyDown args:{{"key", keyIdOf(code)}, {"pointer", 1}}];
}
- (void)screenKeyUp { [self send:calcu::typing::kScreenKeyUp args:{{"key", ""}, {"pointer", 1}}]; }
- (void)hostKeyDownPhysical:(NSString *)physical named:(NSString *)named produced:(uint32_t)produced
                    carried:(uint32_t)carried shift:(BOOL)shift control:(BOOL)control alt:(BOOL)alt
                    command:(BOOL)command {
    calcu::typing::HostKeyEvent e;
    e.physical = cpp(physical);
    e.named = calcu::typing::namedKeyOf(cpp(named));
    e.produced = produced;
    e.carried = carried;
    e.mods.shift = shift == YES;
    e.mods.control = control == YES;
    e.mods.alt = alt == YES;
    e.mods.command = command == YES;
    e.mods.macHost = true;
    [self send:calcu::typing::kHostKeyDown value:calcu::typing::toValue(e)];
}
- (void)hostKeyUpPhysical:(NSString *)physical {
    [self send:calcu::typing::kHostKeyUp args:{{"physical", cpp(physical)}}];
}
- (void)focusLost { [self send:calcu::typing::kFocusLost]; }
- (NSInteger)heldKey {
    const auto* h = _mirror.stateOf<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic);
    if (!h || h->live.empty()) return -1;
    const auto code = cu59::matrixCodeOf(h->live.back());
    return code ? (*code / 10 - 1) * 5 + (*code % 10 - 1) : -1;
}
- (BOOL)fullSpeed { const auto* r = [self run]; return r && r->fullSpeed; }
- (void)setFullSpeed:(BOOL)on { [self send:cu59::commands::kFullSpeed args:{{"on", on == YES}}]; }
- (BOOL)keystrokesPlaying { const auto* r = [self run]; return r && r->keystrokesPlaying; }

// ── Cue card, printer ────────────────────────────────────────────────────────

- (TICueCard *)cueCard {
    const auto* c = _mirror.stateOf<cu59::TICueCardState>(cu59::topics::kCueCard);
    return c && c->card ? TICueCardFrom(*c->card) : nil;
}
- (NSInteger)paperLineCount { return static_cast<NSInteger>(_paperLines.size()); }
- (NSArray<NSString *> *)paperLinesFrom:(NSInteger)start {
    NSMutableArray* out = [NSMutableArray array];
    for (size_t i = static_cast<size_t>(MAX(start, 0)); i < _paperLines.size(); ++i) [out addObject:ns(_paperLines[i])];
    return out;
}
- (NSInteger)paperDotCount { return static_cast<NSInteger>(_paperDots.size()); }
- (NSData *)paperDotsFrom:(NSInteger)start {
    const size_t from = std::min(static_cast<size_t>(MAX(start, 0)), _paperDots.size());
    return [NSData dataWithBytes:_paperDots.data() + from length:(_paperDots.size() - from) * sizeof(TIPaperDot)];
}
- (double)paperReachUnits { return _paperReach; }
- (void)copyPaper { [self send:calcu::paper::kPaperCopyCommand args:{{"owner", kPC100COwner}}]; }
- (void)cutPaper { [self send:calcu::paper::kPaperCutCommand args:{{"owner", kPC100COwner}, {"copy", true}}]; }
- (BOOL)printerConnected { const auto* p = [self printer]; return p && p->connected; }
- (BOOL)printerTrace { const auto* p = [self printer]; return p && p->trace; }
- (void)togglePrinterConnection {
    if ([self printerConnected]) [self send:calcu::startup::kDetachCommand args:{{"place", "printer"}}];
    else [self send:calcu::startup::kAttachCommand args:{{"place", "printer"}, {"peripheral", "PC-100C"}}];
}
- (void)setPrinterControl:(NSString *)control on:(BOOL)on {
    [self send:cu59::commands::kSetControl args:{{"peripheral", kPC100COwner}, {"control", cpp(control)}, {"on", on == YES}}];
}

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

// ── Presets (LIB-6 AC-42, D-41: the content, on iOS and on macOS) ──────────

- (void)loadPresetContent:(NSData *)raw name:(NSString *)name {
    if (!raw) {
        [self send:calcu::presets::kLoadPresetCommand args:{{"content", ""}, {"name", cpp(name)}}];
        return;
    }
    const std::string text(static_cast<const char*>(raw.bytes), raw.length);
    [self send:calcu::presets::kLoadPresetCommand args:{{"content", text}, {"name", cpp(name)}}];
}

// ── Opening a file (LIB-13, M-56) ────────────────────────────────────────────

- (void)wouldOpenFile:(NSString *)fileName completion:(void (^)(BOOL))done {
    _thread->ask(cs::Request{calcu::opening::kWouldTakeRequest, cs::Value::object({{"name", cpp(fileName)}})},
                 [done](const cs::ReplyMessage& r) {
                     const auto* w = cs::payloadAs<calcu::opening::WouldTake>(r.result);
                     done(r.done && w && w->takes ? YES : NO);
                 });
}
- (void)openFile:(NSData *)raw path:(NSString *)path fileName:(NSString *)fileName way:(NSString *)way
      completion:(void (^)(BOOL))done {
    calcu::opening::OpenRequest q;
    calcu::opening::OpenedFile f;
    f.name = cpp(fileName);
    if (path.length) f.path = cpp(path);
    if (raw) f.bytes = bytes(raw);
    q.files = {std::move(f)};
    q.way = calcu::opening::wayNamed(cpp(way)).value_or(calcu::opening::Way::Drop);
    cu59::SessionThread::ReplyHandler reply;
    if (done) reply = [done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); };
    _thread->send(cs::Command{calcu::opening::kOpenCommand, q.toValue()}, std::move(reply));
}

// ── Debugger ─────────────────────────────────────────────────────────────────

- (void)setLiveDebugEnabled:(BOOL)on { [self send:cu59::commands::kLiveDebug args:{{"on", on == YES}}]; }
- (void)setCpuDebugEnabled:(BOOL)on { [self send:cu59::commands::kCpuDebug args:{{"on", on == YES}}]; }
- (BOOL)frozen { const auto* r = [self runControl]; return r && r->paused; }
- (TIFreezeOwner)freezeOwner {
    const auto* r = [self runControl];
    if (!r || !r->paused || !r->owner) return TIFreezeOwnerNone;
    return *r->owner == calcu::debugger::kCpu ? TIFreezeOwnerCpu : TIFreezeOwnerCalculator;
}
- (BOOL)pcChangeArmed { const auto* r = [self runControl]; return r && r->armed == calcu::debugger::kProgram; }
- (BOOL)scanLoopExitArmed { const auto* r = [self runControl]; return r && r->armed == calcu::debugger::kCpu; }
- (TIButtonUse)buttonUseFor:(TIFreezeOwner)level {
    return buttonUseOf(calcu::debugger::buttonUse(levelOf(level), [self runControl]));
}
- (TIFreezeButtons)freezeButtonsFor:(TIFreezeOwner)level {
    const cu59::FreezeButtons b = cu59::freezeButtons(levelOf(level), [self runControl]);
    TIFreezeButtons t;
    t.freeze = b.freeze;
    t.freezeOnStart = b.freezeOnStart;
    t.armed = b.armed;
    return t;
}
- (BOOL)asmRunAvailable { return entryAvailable([self controlState], cu59::entries::kAsmRun) ? YES : NO; }
- (BOOL)traceButtonAvailable { return entryAvailable([self controlState], cu59::entries::kTrace) ? YES : NO; }
- (BOOL)calculatorTabAvailable {
    const auto* c = [self controlState];
    return !c || entryAvailable(c, cu59::entries::kCalculatorTab) ? YES : NO;   // before the first state: yes
}
- (NSString *)calculatorTabReason {
    std::string why;
    entryAvailable([self controlState], cu59::entries::kCalculatorTab, &why);
    return ns(why);
}
+ (TIButtonUse)buttonUseFor:(TIFreezeOwner)level hasState:(BOOL)hasState paused:(BOOL)paused owner:(TIFreezeOwner)owner {
    calcu::debugger::RunState s;
    s.paused = paused == YES;
    if (owner != TIFreezeOwnerNone) s.owner = levelOf(owner);
    return buttonUseOf(calcu::debugger::buttonUse(levelOf(level), hasState ? &s : nullptr));
}
- (void)pause:(TIFreezeOwner)level { [self send:calcu::debugger::kPauseCommand args:{{"level", levelOf(level).value}}]; }
- (void)step:(TIFreezeOwner)level {
    [self send:calcu::debugger::kStepCommand args:{{"level", levelOf(level).value}, {"count", 1}}];
}
- (void)resume { [self send:calcu::debugger::kResumeCommand]; }
- (void)pauseOnStart:(TIFreezeOwner)level {
    if (level == TIFreezeOwnerNone) [self send:calcu::debugger::kPauseOnStartCommand args:{{"off", true}}];
    else [self send:calcu::debugger::kPauseOnStartCommand args:{{"level", levelOf(level).value}}];
}
- (TILiveDebugSnapshot *)liveSnapshot {
    const auto* l = [self live];
    return snapshotFrom(l ? l->live : cu59::LiveDebugSnapshot{});
}
- (NSArray<TIStepEntry *> *)programListing { const auto* l = [self listing]; return l ? stepEntries(l->steps) : @[]; }
- (NSInteger)programListingSource { const auto* l = [self listing]; return l ? static_cast<NSInteger>(l->source) : 0; }
- (NSInteger)programListingCurrentIndex { const auto* l = [self listing]; return l ? l->currentIndex : -1; }
- (NSArray<TIInspectorEntry *> *)cpuHistory {
    const auto* c = _mirror.stateOf<cu59::TICpuDebug>(cu59::topics::kCpuDebug);
    return c ? inspectorEntries(c->entries) : @[];
}
- (NSData *)heatmapCounts {
    const auto* h = _mirror.stateOf<cu59::TIHeatmap>(cu59::topics::kHeatmap);
    if (!h || h->counts.empty()) return [NSMutableData dataWithLength:0x1400 * sizeof(uint32_t)];
    return [NSData dataWithBytes:h->counts.data() length:h->counts.size() * sizeof(uint32_t)];
}
- (void)clearHeatmap { [self send:cu59::commands::kHeatmapReset]; }

- (NSInteger)debugLevel { const auto* v = [self debugView]; return v ? static_cast<NSInteger>(v->logLevel) : 0; }
- (void)cycleDebugLevel {
    const auto next = static_cast<cu59::DebugLevel>(([self debugLevel] + 1) % 3);
    [self send:cu59::commands::kLogLevel args:{{"level", std::string(cu59::debugLevelName(next))}}];
}
- (NSString *)logText { return ns(_logText); }
- (void)clearLog {
    _log.clear();
    _logText.clear();
}
- (void)dumpVars { [self send:cu59::commands::kDump args:{{"what", "vars"}}]; }
- (void)dumpScom { [self send:cu59::commands::kDump args:{{"what", "scom"}}]; }
- (void)dumpProg { [self send:cu59::commands::kDump args:{{"what", "prog"}}]; }
- (void)dumpMemory { [self send:cu59::commands::kDump args:{{"what", "memory"}}]; }

- (BOOL)manualTrace {
    const auto* t = _mirror.stateOf<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic);
    return t && t->running;
}
// The TRACE button (LIB-10 AC-23): the trace session's file of the button's name.
- (void)setManualTrace:(BOOL)on {
    if (on) [self send:calcu::debugger::kTraceCommand args:{{"button", true}}];
    else [self send:calcu::debugger::kTraceCommand args:{{"off", true}}];
}

// ── ASM overlay ──────────────────────────────────────────────────────────────

- (void)loadAsm:(NSString *)text fileName:(NSString *)fileName {
    std::vector<cs::Value::Member> a{{"name", cpp(fileName)}};
    if (text) a.push_back({"text", cpp(text)});
    [self send:cu59::commands::kAsmLoad args:std::move(a)];
}
- (void)runAsm { [self send:cu59::commands::kAsmRun]; }
- (void)clearAsm { [self send:cu59::commands::kAsmClear]; }
- (NSString *)asmFileName { const auto* a = [self asmState]; return a ? ns(a->fileName) : @"No file selected"; }
- (NSInteger)asmWordCount { const auto* a = [self asmState]; return a ? a->instructions : 0; }
- (NSString *)asmStatus { const auto* a = [self asmState]; return a ? ns(a->status) : @""; }
- (BOOL)asmActive { const auto* a = [self asmState]; return a && a->running; }

@end
