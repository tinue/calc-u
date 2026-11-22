#import "TIShell+Session.h"
#import "BridgeConvert.h"
#include <calcu59/app/KeyIds.hpp>
#include <calcu59/app/TIShellProduct.hpp>
#include "pacing/WallClock.hpp"
#include "shell/BarModel.hpp"
#include "shell/HitMap.hpp"
#include "shell/Loading.hpp"
#include "shell/MenuModel.hpp"
#include "shell/Notices.hpp"
#include "shell/SettingsModel.hpp"
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace shell = calcu::shell;
namespace cs = calcu::sessions;

NSString *const TIShellActionLoadPreset = @"load-preset";
NSString *const TIShellActionSettings = @"settings";
NSString *const TIShellActionHelp = @"help";

// ── Value objects (type translation only) ────────────────────────────────────

@interface TIMenuItem () {
@public
    shell::MenuItem _item;   // what `choose:` carries out
}
@property (nonatomic, readwrite) TIMenuItemKind kind;
@property (nonatomic, readwrite, copy) NSString *itemID;
@property (nonatomic, readwrite, copy) NSString *label;
@property (nonatomic, readwrite) BOOL enabled;
@property (nonatomic, readwrite) BOOL checkable;
@property (nonatomic, readwrite) BOOL checked;
@property (nonatomic, readwrite, copy) NSString *reason;
@property (nonatomic, readwrite) TIMenuRole role;
@property (nonatomic, readwrite) TIMenuShortcut shortcut;
@property (nonatomic, readwrite, copy) NSString *confirm;
@property (nonatomic, readwrite) BOOL destructive;
@property (nonatomic, readwrite, copy) NSArray<TIMenuItem *> *children;
@property (nonatomic, readwrite, copy) NSString *command;
@end
@implementation TIMenuItem
@end

@interface TIMenu ()
@property (nonatomic, readwrite, copy) NSString *menuID;
@property (nonatomic, readwrite, copy) NSString *label;
@property (nonatomic, readwrite, copy) NSArray<TIMenuItem *> *items;
@end
@implementation TIMenu
@end

@interface TIBarControl ()
@property (nonatomic, readwrite) TIBarControlKind kind;
@property (nonatomic, readwrite) TIBarGroup group;
@property (nonatomic, readwrite, copy) NSString *controlID;
@property (nonatomic, readwrite, copy) NSString *label;
@property (nonatomic, readwrite) BOOL plugged;
@property (nonatomic, readwrite, copy) NSArray<TIMenuItem *> *choices;
@property (nonatomic, readwrite, copy) NSString *mediumKind;
@property (nonatomic, readwrite, nullable) TIMenuItem *item;
@end
@implementation TIBarControl
@end

@interface TIDialogRow ()
@property (nonatomic, readwrite) TIDialogWidget widget;
@property (nonatomic, readwrite, copy) NSString *section;
@property (nonatomic, readwrite, copy) NSString *key;
@property (nonatomic, readwrite, copy) NSString *label;
@property (nonatomic, readwrite, copy) NSString *value;
@property (nonatomic, readwrite, copy) NSString *shown;
@property (nonatomic, readwrite, copy) NSArray<NSString *> *choiceValues;
@property (nonatomic, readwrite, copy) NSArray<NSString *> *choiceLabels;
@property (nonatomic, readwrite) BOOL resettable;
@property (nonatomic, readwrite, nullable) NSNumber *minimum;
@property (nonatomic, readwrite, nullable) NSNumber *maximum;
@property (nonatomic, readwrite) BOOL isToggle;
@end
@implementation TIDialogRow
@end

@interface TIDialogSection ()
@property (nonatomic, readwrite, copy) NSString *title;
@property (nonatomic, readwrite, copy) NSArray<TIDialogRow *> *rows;
@end
@implementation TIDialogSection
@end

namespace {

TIMenuItem *itemFrom(const shell::MenuItem &i) {
    TIMenuItem *t = [[TIMenuItem alloc] init];
    t->_item = i;
    t.kind = static_cast<TIMenuItemKind>(i.kind);
    t.itemID = ns(i.id);
    t.label = ns(i.label);
    t.enabled = i.enabled;
    t.checkable = i.checkable;
    t.checked = i.checked;
    t.reason = ns(i.reason);
    t.role = static_cast<TIMenuRole>(i.role);
    t.shortcut = static_cast<TIMenuShortcut>(i.shortcut);
    t.confirm = ns(i.confirm);
    t.destructive = i.destructive;
    NSMutableArray *children = [NSMutableArray arrayWithCapacity:i.children.size()];
    for (const auto &c : i.children) [children addObject:itemFrom(c)];
    t.children = children;
    t.command = ns(i.command.value);
    return t;
}

NSArray<TIMenuItem *> *itemsFrom(const std::vector<shell::MenuItem> &v) {
    NSMutableArray *out = [NSMutableArray arrayWithCapacity:v.size()];
    for (const auto &i : v) [out addObject:itemFrom(i)];
    return out;
}

TIMenuItem *findIn(NSArray<TIMenuItem *> *items, NSString *itemID) {
    for (TIMenuItem *i in items) {
        if (i.kind != TIMenuItemKindSeparator && [i.itemID isEqualToString:itemID]) return i;
        if (TIMenuItem *c = findIn(i.children, itemID)) return c;
    }
    return nil;
}

std::string bundleString(NSString *key, const char *fallback) {
    id v = [NSBundle.mainBundle objectForInfoDictionaryKey:key];
    return [v isKindOfClass:NSString.class] ? cpp((NSString *)v) : std::string(fallback);
}

// The version with the build number (3.0.0-0), and the build ID in
// parentheses after it: build-id.txt of tools/build-id.cmake, which the
// app target's phase "Build ID" puts into the app.
std::string buildId() {
    NSString *file = [NSBundle.mainBundle pathForResource:@"build-id" ofType:@"txt"];
    NSString *text = file ? [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] : nil;
    text = [text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    return text.length ? cpp(text) : std::string("unknown");
}

const shell::ShellProduct &product() {
    static const shell::ShellProduct p = cu59::shellProduct(
        bundleString(@"CFBundleShortVersionString", "1.0.0") + "-" + bundleString(@"CFBundleVersion", "1"), buildId());
    return p;
}

// row × 5 + col of a key ID, -1 for none.
NSInteger keyIndexOf(const std::string &key) {
    const auto code = cu59::matrixCodeOf(calcu::machine::KeyId{key});
    return code ? (*code / 10 - 1) * 5 + (*code % 10 - 1) : -1;
}

} // namespace

@implementation TIShell {
    __weak TIAppSession *_session;
    shell::NoticeQueue _notices;
    shell::LoadingRule _loading;
    std::unique_ptr<shell::PointerTracker> _tracker;
    std::shared_ptr<double> _scale;   // points per pixel of the base image, as shown now
    std::map<std::int64_t, NSInteger> _pointerKeys;   // what each pointer holds, from the commands it sent
    NSInteger _lastKey;
}

- (instancetype)initWithSession:(TIAppSession *)session {
    if ((self = [super init])) {
        _session = session;
        _scale = std::make_shared<double>(1.0);
        _lastKey = -1;
        // Every TI model has the same 45 keys (cu59::faceplateHitMap).
        std::shared_ptr<double> scale = _scale;
        _tracker = std::make_unique<shell::PointerTracker>(cu59::faceplateHitMap(calcu::machine::ModelId{"TI-59"}),
                                                           [scale] { return *scale; });
    }
    return self;
}

- (shell::Traits)traits {
    // The iPhone and the iPad: no menu bar to count on (the bar holds
    // Reset, Preset, Settings and More), no About, no Quit.
    shell::Traits t;
    t.menuBar = false;
    t.about = false;
    t.hasQuit = false;
    return t;
}

- (const shell::ShellInputs &)inputs {
    static const shell::ShellInputs none;
    TIAppSession *s = _session;
    return s ? [s shellInputs] : none;
}

- (BOOL)started { return self.inputs.started(); }

- (NSArray<TIMenu *> *)menus {
    NSMutableArray *out = [NSMutableArray array];
    for (const auto &m : shell::menus(self.inputs, product(), self.traits)) {
        TIMenu *t = [[TIMenu alloc] init];
        t.menuID = ns(m.id);
        t.label = ns(m.label);
        t.items = itemsFrom(m.items);
        [out addObject:t];
    }
    return out;
}

- (NSArray<TIMenuItem *> *)moreItems {
    return itemsFrom(shell::moreItems(shell::menus(self.inputs, product(), self.traits), product()));
}

- (NSArray<TIBarControl *> *)bar {
    NSMutableArray *out = [NSMutableArray array];
    if (!self.started) return out;   // no bar before the first control state
    for (const auto &g : shell::bar(self.inputs, product(), self.traits))
        for (const auto &c : g.controls) {
            TIBarControl *t = [[TIBarControl alloc] init];
            t.kind = static_cast<TIBarControlKind>(c.kind);
            t.group = static_cast<TIBarGroup>(g.group);
            t.controlID = ns(c.id);
            t.label = ns(c.label);
            t.plugged = c.plugged;
            t.choices = itemsFrom(c.choices);
            t.mediumKind = ns(c.mediumKind);
            if (c.kind == shell::BarControl::Kind::Action) t.item = itemFrom(c.item);
            [out addObject:t];
        }
    return out;
}

- (NSArray<TIDialogSection *> *)settingsDialog {
    const auto frontend = cu59::ShellFrontend::SwiftIOS;
    const auto &p = product();
    const auto spec = cu59::settingsSpec(frontend, shell::about(p).version);
    NSMutableArray *out = [NSMutableArray array];
    for (const auto &sec : shell::dialog(self.inputs.settings, self.inputs.presetList, spec)) {
        NSMutableArray *rows = [NSMutableArray array];
        for (const auto &r : sec.rows) {
            TIDialogRow *t = [[TIDialogRow alloc] init];
            t.widget = static_cast<TIDialogWidget>(r.row.widget);
            t.section = ns(r.row.section);
            t.key = ns(r.row.key);
            t.label = ns(r.row.label);
            t.value = ns(r.value);
            t.shown = ns(r.shown);
            NSMutableArray *values = [NSMutableArray array], *labels = [NSMutableArray array];
            bool truth = false, falsity = false;
            for (const auto &[v, l] : r.choices) {
                [values addObject:ns(v)];
                [labels addObject:ns(l)];
                truth = truth || v == "true";
                falsity = falsity || v == "false";
            }
            t.choiceValues = values;
            t.choiceLabels = labels;
            t.isToggle = r.row.widget == shell::DialogRow::Widget::Choice && r.choices.size() == 2 && truth && falsity;
            t.resettable = r.resettable;
            t.minimum = r.min ? @(*r.min) : nil;
            t.maximum = r.max ? @(*r.max) : nil;
            [rows addObject:t];
        }
        TIDialogSection *s = [[TIDialogSection alloc] init];
        s.title = ns(sec.title);
        s.rows = rows;
        [out addObject:s];
    }
    return out;
}

- (NSString *)modelID { return ns(shell::read::text(self.inputs.controlState, "model")); }

- (NSString *)paperOwner {
    for (const auto &p : shell::panels(self.inputs))
        if (p.kind == "paper") return ns(p.owner);
    return @"";
}

- (NSString *)settingValueInSection:(NSString *)section key:(NSString *)key {
    const std::string sec = cpp(section), k = cpp(key);
    for (const auto &r : shell::read::elements(self.inputs.settings, "rows"))
        if (shell::read::text(r, "section") == sec && shell::read::text(r, "key") == k)
            return ns(shell::read::text(r, "value"));
    return @"";
}

- (void)setSetting:(NSString *)section key:(NSString *)key value:(NSString *)value {
    std::vector<cs::Value::Member> args{{"section", cpp(section)}, {"key", cpp(key)}};
    args.push_back({"value", value ? cs::Value(cpp(value)) : cs::Value()});
    [self sendChecked:cs::Command{shell::commands::kSetSetting, cs::Value(std::move(args))}];
}

- (TIMenuItem *)itemWithID:(NSString *)itemID {
    for (TIMenu *m in self.menus)
        if (TIMenuItem *i = findIn(m.items, itemID)) return i;
    return findIn(self.moreItems, itemID);
}

// A command the shell sends; its refusal is shown like a notice.
- (void)sendChecked:(cs::Command)command {
    TIAppSession *s = _session;
    if (!s) return;
    __weak TIShell *weakSelf = self;
    cs::Command sent = command;
    [s sendCommand:std::move(command)
             reply:[weakSelf, sent](const cs::ReplyMessage &r) {
                 TIShell *me = weakSelf;
                 if (!me || r.done) return;
                 me->_notices.refusal(r.reason, sent);
                 [me notifyNotices];
             }];
}

- (void)notifyNotices {
    TIAppSession *s = _session;
    [s shellNoticesChanged];
}

- (NSString *)choose:(TIMenuItem *)item {
    if (!item.enabled) return nil;
    const shell::MenuItem &i = item->_item;
    if (!i.command.value.empty()) {
        [self sendChecked:cs::Command{i.command, i.args.isNull() ? cs::Value::object({}) : i.args}];
        return nil;
    }
    return ns(i.id);   // a shell action or a product item: the frontend's
}

- (BOOL)loadingShown { return _loading.shown(calcu::pacing::steadyWallTime()) ? YES : NO; }

- (double)loadingDueIn {
    const auto due = _loading.due();
    if (!due) return -1;
    return std::chrono::duration<double>(*due - calcu::pacing::steadyWallTime()).count();
}

- (NSString *)currentNotice {
    const auto *n = _notices.current();
    return n ? ns(n->text) : nil;
}
- (BOOL)currentNoticeIsQuestion {
    const auto *n = _notices.current();
    return n && n->question;
}
- (void)closeNotice {
    _notices.close();
    [self notifyNotices];
}
- (void)answerNotice:(BOOL)yes {
    if (auto again = _notices.answer(yes == YES)) {
        cs::Value args = again->args;
        [self sendChecked:cs::Command{again->name, std::move(args)}];
    }
    [self notifyNotices];
}

- (void)pushNotice:(NSString *)sentence { _notices.push(cpp(sentence)); }

// ── The pointer ──────────────────────────────────────────────────────────────

- (BOOL)apply:(shell::PointerTracker::Out)out pointer:(int64_t)pointer {
    BOOL pressed = NO;
    TIAppSession *s = _session;
    for (auto &c : out.commands) {
        if (c.name.value == shell::commands::kScreenKeyDown.value) {
            const NSInteger k = keyIndexOf(shell::read::text(c.args, "key"));
            _pointerKeys[pointer] = k;
            _lastKey = k;
            pressed = YES;
        } else if (c.name.value == shell::commands::kScreenKeyUp.value) {
            _pointerKeys.erase(pointer);
            _lastKey = -1;
            for (const auto &[p, k] : _pointerKeys) _lastKey = k;
        }
        if (s) [s sendCommand:std::move(c) reply:{}];
    }
    return pressed;
}

- (BOOL)pointerDown:(int64_t)pointer x:(double)x y:(double)y scale:(double)scale {
    *_scale = scale > 0 ? scale : 1;
    return [self apply:_tracker->down(pointer, x, y) pointer:pointer];
}
- (BOOL)pointerMove:(int64_t)pointer x:(double)x y:(double)y scale:(double)scale {
    *_scale = scale > 0 ? scale : 1;
    return [self apply:_tracker->move(pointer, x, y) pointer:pointer];
}
- (void)pointerUp:(int64_t)pointer { [self apply:_tracker->up(pointer) pointer:pointer]; }
- (void)releaseAllPointers {
    [self apply:_tracker->releaseAll() pointer:-1];
    _pointerKeys.clear();
    _lastKey = -1;
}
- (NSInteger)pointerKey { return _lastKey; }

+ (NSString *)versionText { return ns(product().version); }
+ (NSString *)buildId { return ns(product().build); }
+ (NSArray<NSString *> *)aboutLines {
    NSMutableArray *out = [NSMutableArray array];
    for (const auto &l : product().aboutLines) [out addObject:ns(l)];
    return out;
}
+ (NSArray<NSArray<NSString *> *> *)aboutLinks {
    NSMutableArray *out = [NSMutableArray array];
    for (const auto &[label, url] : product().aboutLinks) [out addObject:@[ ns(label), ns(url) ]];
    return out;
}
+ (NSString *)helpURL { return ns(product().helpUrl); }

- (void)noticeArrived:(const std::string &)sentence { _notices.push(sentence); }

- (void)busyChanged {
    const shell::Busy b = shell::busyOf(self.inputs.busy);
    _loading.busy(b.operationRuns, b.full, calcu::pacing::steadyWallTime());
}

- (void)modelChanged { [self releaseAllPointers]; }

@end
