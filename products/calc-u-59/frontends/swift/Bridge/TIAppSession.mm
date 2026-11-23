#import "TIAppSession.h"
#import "TIShell+Session.h"
#import "BridgeConvert.h"
#import "TIFileFormats+Cpp.h"
#import <UIKit/UIKit.h>
#include <calcu59/core/PC100C.hpp>
#include <calcu59/core/PC100CPaper.hpp>
#include <calcu59/core/TICatalogue.hpp>
#include <calcu59/app/ProductRoms.hpp>
#include <calcu59/app/KeyboardLayout.hpp>
#include "shell/SessionThread.hpp"
#include <calcu59/app/TIAppPart.hpp>
#include <calcu59/app/DebugLog.hpp>
#include <calcu59/app/TIDebugPart.hpp>
#include <calcu59/app/KeyIds.hpp>
#include <calcu59/app/Settings.hpp>
#include <calcu59/app/TIShellProduct.hpp>
#include "typing/TypingTopics.hpp"
#if DEBUG && TARGET_OS_SIMULATOR
#include <calcu59/app/HarnessProduct.hpp>
#include "harness/attach/AttachPort.hpp"
#endif
#include "sessions/Args.hpp"
#include "presets/PresetsPart.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/SettingsPart.hpp"
#include "startup/StartupPart.hpp"
#include "debugger/ButtonRule.hpp"
#include "debugger/RunState.hpp"
#include "control/ControlState.hpp"
#include "opening/OpenRequest.hpp"
#include "paper/PaperTopics.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "media/MediaTopics.hpp"
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

/// The card and constant-memory files as the app had them: read
/// and written through NSFileCoordinator (the media part's file access too);
/// a card that iCloud has not
/// downloaded yet (`.name.icloud`) is started downloading and reads as there
/// but not readable. On the session thread.
class CoordinatedFiles final : public calcu::media::FileAccess {
public:
    explicit CoordinatedFiles(id<TIAppSessionDelegate> d) : delegate_(d) {}
    Read read(const std::string& path) const override {
        NSURL* url = [NSURL fileURLWithPath:ns(path)];
        __block NSData* data = nil;
        NSError* err = nil;
        [[[NSFileCoordinator alloc] init] coordinateReadingItemAtURL:url options:0 error:&err
                                                          byAccessor:^(NSURL* src) { data = [NSData dataWithContentsOfURL:src]; }];
        if (data) return {std::string(static_cast<const char*>(data.bytes), data.length), false};
        NSString* placeholder = [[url URLByDeletingLastPathComponent]
            URLByAppendingPathComponent:[NSString stringWithFormat:@".%@.icloud", url.lastPathComponent]].path;
        const bool exists = [[NSFileManager defaultManager] fileExistsAtPath:url.path];
        if (!exists && [[NSFileManager defaultManager] fileExistsAtPath:placeholder]) {
            [[NSFileManager defaultManager] startDownloadingUbiquitousItemAtURL:url error:nil];
            return {std::nullopt, true};
        }
        return {std::nullopt, exists};
    }
    Written write(const std::string& path, const std::string& text) const override {
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
        if (!ok || err) return {false, "the file cannot be written."};
        __weak id<TIAppSessionDelegate> d = delegate_;
        if (!url.path.lastPathComponent || ![url.lastPathComponent isEqualToString:@"ti58c.memory.yaml"])
            dispatch_async(dispatch_get_main_queue(), ^{ [d cardWritten]; });
        return {true, {}};
    }

private:
    __weak id<TIAppSessionDelegate> delegate_;
};

TIChange changeOf(const cs::TopicName& t) {
    namespace T = cu59::topics;
    if (t == T::kCalcIndicator) return TIChangeDisplay;
    if (t == T::kCueCard) return static_cast<TIChange>(TIChangeCueCard | TIChangeCard);   // on show, the lit corner
    if (t == calcu::media::kMediaTopic) return static_cast<TIChange>(TIChangeCard | TIChangeShell);   // the card reader
    if (t == calcu::debugger::kRunStateTopic) return static_cast<TIChange>(TIChangeRunState | TIChangeShell);
    if (t == calcu::typing::kHeldKeysTopic) return TIChangeRunState;
    if (t == cs::kBusyTopic || t == calcu::shell::kSettingsTopic) return TIChangeShell;
    if (t == calcu::debugger::kTraceTopic) return TIChangeTrace;
    // The debug panel's product topics.
    if (t == T::kCalculatorDebug) return TIChangeLive;
    if (t == T::kProgramListing) return TIChangeListing;
    if (t == T::kCpuDebug) return TIChangeCpu;
    if (t == T::kHeatmap) return TIChangeHeatmap;
    if (t == T::kAsmOverlay) return TIChangeAsm;
    if (t == cs::kLogLevelTopic) return TIChangeLog;   // the level the LOG button shows
    // The PC-100C's controls and TRACE's position too.
    if (t == calcu::control::kControlStateTopic)
        return static_cast<TIChange>(TIChangeControl | TIChangeShell | TIChangeCard | TIChangePrinter);
    return static_cast<TIChange>(0);
}

// The host services of the app; no audio sink, the key click is
// the app's own.
cs::HostServices statedServices() {
    cs::HostServices services;
    services.offered = {cs::HostService::ClipboardText, cs::HostService::ClipboardImage, cs::HostService::FilePicker,
                        cs::HostService::KeepFile, cs::HostService::OpenUrl, cs::HostService::Haptics};
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

calcu::debugger::LevelName levelOf(TIRunLevel l) {
    return l == TIRunLevelCpu ? calcu::debugger::kCpu : calcu::debugger::kProgram;
}

TIButtonUse buttonUseOf(const calcu::debugger::ButtonUse& u) {
    TIButtonUse t;
    t.pause = u.pause;
    t.step = u.step;
    t.resume = u.resume;
    return t;
}

// The host services the bridge answers: the clipboard (the session's Copy
// makes the picture; the entry is the platform's). A picture keeps its physical size (`dotsPerInch`).
cs::ServiceAnswer answerService(const cs::ServiceAsk& ask) {
    using cs::HostService;
    if (ask.service == HostService::ClipboardImage) {
        const cs::Value* b = ask.args.find("bytes");
        if (!b || !b->bytes()) return cs::ServiceAnswer{false, cs::Value(), "No picture to copy."};
        NSData* png = [NSData dataWithBytes:b->bytes()->data() length:b->bytes()->size()];
        const cs::Value* t = ask.args.find("text");
        NSString* text = t && t->string() ? ns(*t->string()) : nil;
        NSMutableDictionary* item = [NSMutableDictionary dictionaryWithObject:png forKey:@"public.png"];
        if (text) item[@"public.utf8-plain-text"] = text;
        UIPasteboard.generalPasteboard.items = @[item];
        return cs::ServiceAnswer{true, cs::Value(), {}};
    }
    if (ask.service == HostService::ClipboardText) {
        if (const cs::Value* t = ask.args.find("text"); t && t->string()) {
            UIPasteboard.generalPasteboard.string = ns(*t->string());
            return cs::ServiceAnswer{true, cs::Value(), {}};
        }
        NSString* now = UIPasteboard.generalPasteboard.string;
        return cs::ServiceAnswer{true, cs::Value::object({{"text", now ? cpp(now) : std::string()}}), {}};
    }
    return cs::ServiceAnswer{false, cs::Value(), "This frontend does not answer " + std::string(cs::serviceName(ask.service)) + "."};
}

// A product entry of the control state: {available, reason}.
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

@interface TIPickerEntry ()
@property (nonatomic, readwrite, copy) NSString *entryID;
@property (nonatomic, readwrite, copy) NSString *label;
@property (nonatomic, readwrite, copy) NSString *shortLabel;
@property (nonatomic, readwrite) BOOL canChoose;
@property (nonatomic, readwrite) BOOL chosen;
@end
@implementation TIPickerEntry
@end

@implementation TISavedCard
- (instancetype)initWithName:(NSString *)name entryID:(NSString *)entryID canChoose:(BOOL)canChoose hint:(NSString *)hint {
    if ((self = [super init])) {
        _name = [name copy];
        _entryID = [entryID copy];
        _canChoose = canChoose;
        _hint = [hint copy];
    }
    return self;
}
@end

static calcu::shell::SessionThread::ReplyHandler flagReply(void (^done)(BOOL)) {
    if (!done) return {};
    return [done](const cs::ReplyMessage& r) {
        const auto* f = cs::payloadAs<cu59::TIFlag>(r.result);
        done(r.done && (!r.result || (f && f->value)) ? YES : NO);
    };
}

@implementation TIAppSession {
    __weak id<TIAppSessionDelegate> _delegate;
    std::unique_ptr<calcu::shell::SessionThread> _thread;
    // The shell's inputs: the last Value of each, on the main thread.
    calcu::shell::ShellInputs _inputs;
    std::string _model;   // the control state's model, to let go of the pointers at a switch
    cs::TopicMirror _mirror;
    // The PC-100C's paper as the library's topic sent it (the paper panel).
    std::vector<std::string> _paperLines;
    std::vector<TIPaperDot> _paperDots;
    double _paperReach;
    // The LOG tab's view of the library's `log`: the last 2000 lines.
    cu59::RingLog _log;
    std::string _logText;
    // c59.debug-view's count of requests already shown.
    std::uint64_t _viewRequests;
    // The session's library parts, set by the factory on the session thread
    // (the attached harness's commands use them).
    std::shared_ptr<calcu::parts::LibraryParts*> _libraryParts;
}

- (instancetype)initWithRoms:(TIRomSet *)roms start:(TISessionStart *)start delegate:(id<TIAppSessionDelegate>)delegate {
    if ((self = [super init])) {
        _delegate = delegate;
        namespace st = calcu::settings;
        // The settings file is the session's: opened here, then
        // read and written on the session thread only, by the library's start
        // and its SettingsPart; the main thread does not touch it again.
        st::HostDirs dirs;
        dirs.frontend = st::FrontendKind::Swift;
        dirs.home = cpp(NSHomeDirectory());
        dirs.settingsBase = cpp(start.settingsBase);
        dirs.saveFolder = cpp(start.saveFolder);
        dirs.applicationSupport = cpp([start.settingsBase stringByAppendingPathComponent:@"Calc-U-59"]);
        // The iCloud trace location only while signed in, asked at each
        // trace start.
        dirs.iCloudAvailable = [] { return NSFileManager.defaultManager.ubiquityIdentityToken != nil; };
        const auto& decl = cu59::settings::settingsDecl();
        std::optional<std::string> path;
        if (start.settingsFile) path = cpp(start.settingsFile);
        // A settings file the run names (the tests) starts bare, without
        // the startup presets a fresh install gets.
        const bool firstValues = !path;
        auto settings = std::make_shared<calcu::shell::HostSettings>(decl, dirs, path, firstValues);
        st::SettingsFile* file = &settings->file();
        const st::Paths* paths = &settings->paths();
        _settingsProblem = file->problem().empty() ? nil : ns(file->problem());

        cu59::TIAppSetup setup;
        setup.roms.set = roms->_set;
        // The settings file decides the startup model; the
        // session's `start` (makeAppSession) builds it. The save folder, the
        // trace folder by [trace] location, its limit and `~/`
        // are the places'.
        setup.settings = file;
        setup.paths = paths;
        setup.examplesFolder = cpp(start.examplesFolder);
        setup.files = std::make_shared<CoordinatedFiles>(delegate);
        // Where traces go is the places' by [trace] location,
        // read at each trace start with the folder made and written once;
        // iCloud only while signed in (dirs, above). The stated host
        // services (the session asks none of them yet).
        const cs::HostServices services = statedServices();
        __weak TIAppSession* weakSelf = self;
        _shell = [[TIShell alloc] initWithSession:self];
        auto libraryParts = std::make_shared<calcu::parts::LibraryParts*>(nullptr);
        _libraryParts = libraryParts;
        _thread = std::make_unique<calcu::shell::SessionThread>(
            // The file and its places live as long as the factory, which
            // outlives the session it made (SessionThread::run); the library's
            // SettingsPart is the session's (calcu::shell::makeHostSession).
            [setup, settings, libraryParts] {
                calcu::shell::HostSessionSetup host;
                host.settings = settings.get();
                host.settingsAlsoShown = {cu59::kPortraitDebugPage};   // the iOS row "Debug Page in Portrait"
                host.clock = cu59::steadyWallTime;
                host.hostTime = [] { return std::chrono::system_clock::now(); };
                host.start = true;
                auto app = cu59::makeAppSession(setup, std::move(host));
                *libraryParts = &app.part->libraryParts();
                return std::move(app.session);
            },
            services, cu59::steadyWallTime,
            // One hop to the main thread per batch; a batch that comes
            // after the bridge is gone is dropped.
            [weakSelf] { dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf drain]; }); });
        [self askPresetList];
    }
    return self;
}

// The preset list: the Examples menu and the startup preset rows.
- (void)askPresetList {
    __weak TIAppSession* weakSelf = self;
    _thread->ask(cs::Request{calcu::presets::kPresetListRequest, cs::Value::object({})}, [weakSelf](const cs::ReplyMessage& r) {
        TIAppSession* me = weakSelf;
        if (!me) return;
        [me setPresetList:r.done && r.result ? r.result->toValue() : cs::Value()];
    });
}

- (void)dealloc { _thread.reset(); }

- (const calcu::debugger::RunState *)runControl {
    return _mirror.stateOf<calcu::debugger::RunState>(calcu::debugger::kRunStateTopic);
}
- (const calcu::media::PlaceState *)cardReader {
    const auto* m = _mirror.stateOf<calcu::media::MediaState>(calcu::media::kMediaTopic);
    return m ? m->place("card-reader") : nullptr;
}
- (const cu59::TICueCardState *)cue { return _mirror.stateOf<cu59::TICueCardState>(cu59::topics::kCueCard); }
- (const cu59::TICalculatorDebug *)live { return _mirror.stateOf<cu59::TICalculatorDebug>(cu59::topics::kCalculatorDebug); }
- (const cu59::TIProgramListing *)listing { return _mirror.stateOf<cu59::TIProgramListing>(cu59::topics::kProgramListing); }
- (const cu59::TIDebugView *)debugView { return _mirror.stateOf<cu59::TIDebugView>(cu59::topics::kDebugView); }
- (const cu59::TIAsmOverlayState *)asmState { return _mirror.stateOf<cu59::TIAsmOverlayState>(cu59::topics::kAsmOverlay); }
- (const calcu::control::ControlState *)controlState {
    return _mirror.stateOf<calcu::control::ControlState>(calcu::control::kControlStateTopic);
}

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
            if (s->topic == kPaper) {   // the reset marker: the paper was emptied
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
            if (s->topic == calcu::paper::kPaperCopiesTopic) {   // the outcome of a Copy
                for (const auto& p : s->appended)
                    if (const auto* o = cs::payloadAs<calcu::paper::CopyOutcome>(p);
                        o && [d respondsToSelector:@selector(paperCopied:)])
                        [d paperCopied:static_cast<NSInteger>(o->outcome)];
                continue;
            }
            if (s->topic == cs::kNoticesTopic) {   // the shell's queue, each in turn
                bool any = false;
                for (const auto& p : s->appended)
                    if (const auto* n = cs::payloadAs<cs::Notice>(p)) {
                        [_shell noticeArrived:n->sentence];
                        any = true;
                    }
                if (any) [d noticesChanged];
                continue;
            }
            if (s->topic == cs::kLogTopic) {   // the LOG tab's lines (a message may hold several)
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
        if (const auto* st = std::get_if<cs::StateMessage>(&m)) {
            changes = static_cast<TIChange>(changes | changeOf(st->topic));
            if ([self takeInput:*st]) changes = static_cast<TIChange>(changes | TIChangeShell);
        }
        _mirror.deliver(m);
    }
    // The tab a `debug-panel:` step asked for, each time c59.debug-view's count grows.
    if (const auto* v = [self debugView]; v && v->requests > _viewRequests) {
        _viewRequests = v->requests;
        [d showDebugPanel:static_cast<NSInteger>(v->view)];
    }
    if (changes) [d changed:changes];
}

// The shell's inputs: the last Value of each topic it draws from. YES: a
// topic the product's items follow changed (the shell's models draw anew).
- (BOOL)takeInput:(const cs::StateMessage&)st {
    static const calcu::shell::InputNames n;
    const cs::Value v = st.state ? st.state->toValue() : cs::Value();
    if (st.topic == n.controlState) {
        _inputs.controlState = v;
        const std::string model = calcu::shell::read::text(v, "model");
        if (model != _model) {
            if (!_model.empty()) [_shell modelChanged];
            _model = model;
        }
    } else if (st.topic == n.busy) {
        _inputs.busy = v;
        [_shell busyChanged];
    } else if (st.topic == n.runState) {
        _inputs.runState = v;
    } else if (st.topic == n.romsMissing) {
        _inputs.romsMissing = v;
    } else if (st.topic == n.settings) {
        _inputs.settings = v;
    } else if (st.topic == n.media) {
        _inputs.media = v;
    } else {
        for (const auto& t : [TIShell productTopics])
            if (st.topic == t) {
                cs::Value& kept = _inputs.productTopics[t.value];
                if (kept == v) return NO;
                kept = v;
                return YES;
            }
    }
    return NO;
}

- (const calcu::shell::ShellInputs&)shellInputs { return _inputs; }
- (void)setPresetList:(cs::Value)list {
    _inputs.presetList = std::move(list);
    [self shellInputsChanged];
}
- (void)sendCommand:(cs::Command)command reply:(std::function<void(const cs::ReplyMessage&)>)reply {
    _thread->send(std::move(command), std::move(reply));
}
- (void)askRequest:(cs::Request)request reply:(std::function<void(const cs::ReplyMessage&)>)reply {
    _thread->ask(std::move(request), std::move(reply));
}
- (void)shellNoticesChanged { [_delegate noticesChanged]; }
- (void)shellInputsChanged { [_delegate changed:TIChangeShell]; }

- (void)send:(const cs::CommandName&)name args:(std::vector<cs::Value::Member>)args reply:(calcu::shell::SessionThread::ReplyHandler)reply {
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

#if DEBUG && TARGET_OS_SIMULATOR
- (BOOL)openAttachWithArguments:(NSArray<NSString *> *)arguments {
    std::vector<std::string> args;
    for (NSString* a in arguments) args.push_back(cpp(a));
    const std::uint16_t port = calcu::harness::attachPort(args);
    static std::unique_ptr<calcu::harness::HarnessProduct> product = cu59::makeHarnessProduct();
    const std::string problem = _thread->openAttach(*product, port, [parts = _libraryParts]() -> calcu::parts::LibraryParts& { return **parts; });
    if (problem.empty()) {
        [_delegate diagnostic:[NSString stringWithFormat:@"Harness attach on 127.0.0.1:%u.", port]];
        return YES;
    }
    // The app runs on; a line of the session's log says why.
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

// ── Display, keys, speed ─────────────────────────────────────────────────────

- (TIDisplaySnapshot)display {
    TIDisplaySnapshot d{};
    if (const auto* l = _mirror.stateOf<cu59::TICalcIndicatorState>(cu59::topics::kCalcIndicator)) {
        memcpy(d.segments, l->led.segments, sizeof d.segments);
        memcpy(d.digitLevel, l->led.digitLevel, sizeof d.digitLevel);
        memcpy(d.dpLevel, l->led.dpLevel, sizeof d.dpLevel);
    }
    return d;
}
- (double)calcIndicatorOpacity {
    const auto* l = _mirror.stateOf<cu59::TICalcIndicatorState>(cu59::topics::kCalcIndicator);
    return l ? l->calcIndicatorOpacity : 0;
}
- (void)focusLost { [self send:calcu::typing::kFocusLost]; }
- (NSInteger)heldKey {
    const auto* h = _mirror.stateOf<calcu::typing::HeldKeys>(calcu::typing::kHeldKeysTopic);
    if (!h || h->live.empty()) return -1;
    const auto code = cu59::matrixCodeOf(h->live.back());
    return code ? (*code / 10 - 1) * 5 + (*code % 10 - 1) : -1;
}

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
- (BOOL)printerTrace {
    const auto* c = [self controlState];
    if (!c) return NO;
    for (const auto& i : c->controlsAt(calcu::machine::PlacePath{{::kTIPrinterPlace}}))
        if (i.id == ::PC100C::kTrace) return i.on ? YES : NO;
    return NO;
}
- (void)setPrinterControl:(NSString *)control on:(BOOL)on {
    [self send:calcu::peripherals::kSetControlCommand args:{{"peripheral", kPC100COwner}, {"control", cpp(control)}, {"on", on == YES}}];
}

// ── Card reader ──────────────────────────────────────────────────────────────

static const char* const kCardReaderPlace = "card-reader";

- (BOOL)cardPassing { const auto* p = [self cardReader]; return p && p->passing ? YES : NO; }
- (BOOL)cardFitted { const auto* p = [self cardReader]; return p && p->holds ? YES : NO; }
- (NSString *)fittedCardName { const auto* p = [self cardReader]; return p && p->holds ? ns(p->holds->name) : nil; }
- (NSArray<TISavedCard *> *)savedCards {
    NSMutableArray<TISavedCard *>* out = [NSMutableArray array];
    const auto* p = [self cardReader];
    if (!p) return out;
    using G = calcu::media::PickerEntry::Group;
    // The saved cards only; the fresh card is Arm Fresh Card's.
    for (const auto& e : p->picker)
        if (e.group == G::Instance || e.group == G::UserTemplate)
            [out addObject:[[TISavedCard alloc] initWithName:ns(e.label) entryID:ns(e.id) canChoose:e.canChoose
                                                        hint:e.hint ? ns(*e.hint) : nil]];
    return out;
}
- (void)armCard:(TISavedCard *)card {
    [self send:calcu::media::kChooseCommand args:{{"place", kCardReaderPlace}, {"entry", cpp(card.entryID)}}];
}
- (void)armFreshCard:(NSString *)name confirmed:(BOOL)confirmed completion:(void (^)(NSString *, NSString *))done {
    [self send:calcu::media::kStartNewCommand
          args:{{"place", kCardReaderPlace}, {"template", "implicit:Fresh card"}, {"name", cpp(name)}, {"confirmed", confirmed == YES}}
         reply:[done](const cs::ReplyMessage& r) {
             if (!done) return;
             static const std::string kAsks = "asks: ";
             if (r.done) done(nil, nil);
             else if (r.reason.rfind(kAsks, 0) == 0) done(ns(r.reason.substr(kAsks.size())), nil);
             else done(nil, ns(r.reason));
         }];
}
- (NSInteger)litCardCorner { const auto* c = [self cue]; return c && c->litCorner ? *c->litCorner : -1; }
- (BOOL)tapCardAtX:(double)x y:(double)y {
    const auto* c = [self cue];
    if (!c || !c->onShow || !cu59::cardCornerAt(x, y)) return NO;
    [self send:cu59::commands::kTapCard args:{{"x", x}, {"y", y}}];
    return YES;
}
- (void)ejectCard { [self send:calcu::media::kEjectCommand args:{{"place", kCardReaderPlace}}]; }

- (NSArray<TIPickerEntry *> *)pickerEntriesForPlace:(NSString *)place {
    NSMutableArray<TIPickerEntry *>* out = [NSMutableArray array];
    const auto* m = _mirror.stateOf<calcu::media::MediaState>(calcu::media::kMediaTopic);
    const auto* p = m ? m->place(cpp(place)) : nullptr;
    if (!p) return out;
    using G = calcu::media::PickerEntry::Group;
    for (const auto& e : p->picker) {
        TIPickerEntry *t = [[TIPickerEntry alloc] init];
        t.entryID = ns(e.id);
        t.label = e.group == G::Empty ? @"None" : ns(e.label);
        t.shortLabel = ns(calcu::media::shortLabel(e));
        t.canChoose = e.canChoose;
        t.chosen = e.chosen;
        [out addObject:t];
    }
    return out;
}
- (void)chooseEntry:(TIPickerEntry *)entry place:(NSString *)place {
    [self send:calcu::media::kChooseCommand args:{{"place", cpp(place)}, {"entry", cpp(entry.entryID)}}];
}

// ── Presets (the content) ────────────────────────────────────────────

- (void)loadPresetContent:(NSData *)raw name:(NSString *)name {
    if (!raw) {
        [self send:calcu::presets::kLoadPresetCommand args:{{"content", ""}, {"name", cpp(name)}}];
        return;
    }
    const std::string text(static_cast<const char*>(raw.bytes), raw.length);
    [self send:calcu::presets::kLoadPresetCommand args:{{"content", text}, {"name", cpp(name)}}];
}

// ── Opening a file ───────────────────────────────────────────────────────────

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
    calcu::shell::SessionThread::ReplyHandler reply;
    if (done) reply = [done](const cs::ReplyMessage& r) { done(r.done ? YES : NO); };
    _thread->send(cs::Command{calcu::opening::kOpenCommand, q.toValue()}, std::move(reply));
}

// ── Debugger ─────────────────────────────────────────────────────────────────

- (void)setLiveDebugEnabled:(BOOL)on { [self send:cu59::commands::kShowCalculatorDebug args:{{"on", on == YES}}]; }
- (void)setCpuDebugEnabled:(BOOL)on { [self send:cu59::commands::kShowCpuDebug args:{{"on", on == YES}}]; }
- (BOOL)frozen { const auto* r = [self runControl]; return r && r->paused; }
- (TIRunLevel)pausedLevel {
    const auto* r = [self runControl];
    if (!r || !r->paused || !r->owner) return TIRunLevelNone;
    return *r->owner == calcu::debugger::kCpu ? TIRunLevelCpu : TIRunLevelCalculator;
}
- (BOOL)pcChangeArmed { const auto* r = [self runControl]; return r && r->armed == calcu::debugger::kProgram; }
- (BOOL)scanLoopExitArmed { const auto* r = [self runControl]; return r && r->armed == calcu::debugger::kCpu; }
- (TIButtonUse)buttonUseFor:(TIRunLevel)level {
    return buttonUseOf(calcu::debugger::buttonUse(levelOf(level), [self runControl]));
}
- (TIFreezeButtons)freezeButtonsFor:(TIRunLevel)level {
    const cu59::FreezeButtons b = cu59::freezeButtons(levelOf(level), [self runControl]);
    TIFreezeButtons t;
    t.freeze = b.freeze;
    t.freezeOnStart = b.freezeOnStart;
    t.armed = b.armed;
    return t;
}
- (BOOL)asmRunAvailable { return entryAvailable([self controlState], cu59::entries::kAsmRun) ? YES : NO; }
- (BOOL)calculatorTabAvailable {
    const auto* c = [self controlState];
    return !c || entryAvailable(c, cu59::entries::kCalculatorTab) ? YES : NO;   // before the first state: yes
}
+ (TIButtonUse)buttonUseFor:(TIRunLevel)level hasState:(BOOL)hasState paused:(BOOL)paused owner:(TIRunLevel)owner {
    calcu::debugger::RunState s;
    s.paused = paused == YES;
    if (owner != TIRunLevelNone) s.owner = levelOf(owner);
    return buttonUseOf(calcu::debugger::buttonUse(levelOf(level), hasState ? &s : nullptr));
}
- (void)pause:(TIRunLevel)level { [self send:calcu::debugger::kPauseCommand args:{{"level", levelOf(level).value}}]; }
- (void)step:(TIRunLevel)level {
    [self send:calcu::debugger::kStepCommand args:{{"level", levelOf(level).value}, {"count", 1}}];
}
- (void)resume { [self send:calcu::debugger::kResumeCommand]; }
- (void)pauseOnStart:(TIRunLevel)level {
    if (level == TIRunLevelNone) [self send:calcu::debugger::kPauseOnStartCommand args:{{"off", true}}];
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

- (NSInteger)debugLevel {
    const auto* l = _mirror.stateOf<cs::LogLevelState>(cs::kLogLevelTopic);
    return l ? static_cast<NSInteger>(l->level) : 0;
}
- (void)cycleDebugLevel {
    const auto next = static_cast<cs::LogLevel>(([self debugLevel] + 1) % 3);
    [self send:cs::kLogLevelCommand args:{{"level", std::string(cs::logLevelName(next))}}];
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
// The TRACE button: the trace session's file of the button's name.
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
