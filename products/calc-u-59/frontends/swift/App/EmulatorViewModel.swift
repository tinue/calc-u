import Foundation
import os
import SwiftUI
import AudioToolbox

/// Which debug panel owns the current freeze: the level of the library's run
/// control (LIB-10 59): CPU is `cpu`, CALCULATOR `program`. RESUME and STEP
/// are only enabled in the owning panel (the run-control buttons' rule,
/// calcu::debugger::buttonUse, AC-18).
enum FreezePanel { case cpu, calculator }

/// Selected tab in the debug panel. Stored on EmulatorViewModel so the
/// selection survives navigation on iPhone (where DebugView is recreated).
enum DebugTab { case live, cpu, log }

/// cu59::DebugLevel of app/TIAppPart.hpp (cycled there).
enum DebugLevel: Int {
    case off   = 0
    case info  = 1
    case debug = 2
}

/// Where the running program's steps come from, decoded from the PRG SOURCE
/// flag (SCOM[0] nibble 3) the way the firmware tests it: the run loop (ROM
/// 07C9) only asks "non-zero?", the step fetch (08B7, 0B98) tests bit 0
/// (module) before bit 3 (constant-ROM keycode table). There is no fast-mode
/// flag value: fast mode is any non-zero flag that selects neither table
/// (2, 4, 6) — see rom/TI59-ROM-Guide.md § "Fast mode".
enum ProgramSource: Equatable {
    /// 0: RAM program or keyboard.
    case userProgram
    /// Bit 0 set (1 official; 3, 5, 7, 9 fetch from the module just the same).
    case solidState
    /// 2 inside RTN_POP → LIB_RETURN (ROM 1193–1194, 14F9–1506, same on all
    /// variants): RTN has written the popped library level into SCOM[0], so
    /// nibbles 4–7 hold the saved CROM return address (plain BCD, not a step
    /// number) until 1506 sets the flag back to 1. Lasts 16 instructions —
    /// visible only when the CPU is single-stepped; STEP stops at the step
    /// dispatch, outside it.
    case solidStateReturn
    /// 2, 4 or 6 anywhere else: a RAM program on the sub-program fast path —
    /// no display formatting, no R/S or GTO polls.
    case fastMode
    /// Bit 3 set, bit 0 clear (8 official): keycode sub-program in the
    /// constant ROM.
    case rom

    /// Decoded by app/ModuleImage.cpp (programSourceFrom).
    init(flag: UInt8, romPC: UInt16) {
        self = Self.cases[TIViewRules.programSource(forFlag: flag, romPC: romPC)]
    }

    /// TIViewRules / TILiveDebugSnapshot raw values follow the C++ enumerator order.
    init(raw: Int) { self = Self.cases[raw] }
    var raw: Int { Self.cases.firstIndex(of: self)! }
    private static let cases: [ProgramSource] = [.userProgram, .solidState, .solidStateReturn, .fastMode, .rom]
}
/// The SwiftUI face of Calc-U-59's app session (the Qt app's is
/// frontends/qt6/app/EmulatorController): the session runs on a thread of its
/// own behind Bridge/TIAppSession (C59-5 AC-4); this view model sends commands
/// and copies the messages of each batch into observable properties. It never
/// runs the machine. Everything that isn't UI state lives in app/ — see
/// CLAUDE.md § "Frontend Parity".
@Observable
class EmulatorViewModel {
    private static let logger = Logger(subsystem: Bundle.main.bundleIdentifier ?? "Calc-U-59", category: "EmulatorViewModel")

    var displaySegments: [UInt8] = Array(repeating: 0, count: 12)
    var displayLevels:   [UInt8] = Array(repeating: 0, count: 12)  // brightness 0…255 per position
    var dpLevels:        [UInt8] = Array(repeating: 0, count: 12)  // decimal point brightness per position
    var calcIndicatorOpacity: Double = 0.0

    // ── The family structure (C59-8): the library's shell models through
    // Bridge/TIShell, copied at each change of their inputs (TIChangeShell).
    // No view decides what the machine offers (frontend.md FE-2).
    /// The running model, the control state's ID ("TI-59"); the art is chosen by it.
    private(set) var modelID: String = ""
    /// A control state came (AC-19: without one, no bar and no Machine menu).
    private(set) var started: Bool = false
    private(set) var menus: [TIMenu] = []
    private(set) var moreItems: [TIMenuItem] = []
    private(set) var barControls: [TIBarControl] = []
    private(set) var settingsSections: [TIDialogSection] = []
    /// The paper panel's owner (LIB-8 AC-12, EC-5), "" while no printer is attached.
    private(set) var paperOwner: String = ""
    /// "Loading…" (AC-9).
    private(set) var loadingShown: Bool = false
    /// The notice shown now (AC-10), each in turn; a question has Yes and No.
    private(set) var currentNotice: String? = nil
    private(set) var noticeIsQuestion: Bool = false
    // The settings the views draw with, from the `settings` topic.
    private(set) var ledStyle: LEDFontStyle = .modernized
    private(set) var keyboardFeedback: KeyboardFeedbackType = .off
    private(set) var portraitDebugPage: Bool = false
    /// The shell asks the window for a file dialog (Load Preset…) or the
    /// settings (iOS sheet): each request moves the serial.
    private(set) var presetDialogSerial: Int = 0
    private(set) var settingsSerial: Int = 0
    /// The settings sheet is open (iOS): a notice shows over it (EC-2).
    var settingsSheetShown: Bool = false

    // ── Printer state ────────────────────────────────────────────────────────
    // The PC-100C's paper: the library's `paper PC-100C` (C59-8 AC-12, the
    // paper panel): its text lines and its dots, in the unit.
    var printerLines: [String] = []
    /// The dots by band of `paperBandUnits` down the paper (a dot near a band's
    /// edge is in both), so that the strip draws only the bands in view.
    var paperBands: [[TIPaperDot]] = []
    private var paperDotsTaken = 0
    static let paperBandUnits = 10.0
    var paperReachUnits: Double = 0 {
        didSet {
            let n = Int((paperReachUnits / Self.paperBandUnits).rounded(.up))
            if paperBands.count < n { paperBands.append(contentsOf: Array(repeating: [], count: n - paperBands.count)) }
        }
    }
    var printerClearID: Int = 0   // incremented on cut to reset Text identity and drop selection
    /// The outcome of the last Copy (LIB-8 AC-20): "Copied to clipboard" or "Copy failed".
    var paperCopyMessage: String? = nil
    var paperCopySerial: Int = 0
    var printerTrace: Bool = false

    // ── Binary trace (the "C" indicator toggle) ──────────────────────────────
    var cIndicatorDebug: Bool = false {
        didSet {
            guard session?.manualTrace != cIndicatorDebug else { return }
            session?.setManualTrace(cIndicatorDebug)   // the trace topic says whether it opened
        }
    }
    /// False if the trace location (e.g. iCloud) is unavailable at startup.
    private(set) var isTraceAvailable = true

    // ── Debug panel state ────────────────────────────────────────────────────
    var debugTab: DebugTab = .live   // persists tab selection across iPhone navigation
    var debugLevel: DebugLevel = .off
    var debugClearID: Int = 0              // incremented on clear to reset Text identity
    var debugDisplayText: String = ""      // LOG pane text: the bridge's view of LIB-3's `log` (C59-4 AC-9)
    var asmFileName: String = "No file selected"
    var asmWordCount: Int = 0
    var asmStatusMessage: String = "Load a hex opcode file and press Run."
    var asmOverlayActive: Bool = false
    // The product entries of the control state (C59-4 AC-25): Run, TRACE and
    // the CALCULATOR tab, each as the session says.
    private(set) var canRunASM: Bool = false
    private(set) var traceButtonAvailable: Bool = true
    private(set) var calculatorTabAvailable: Bool = true

    // ── Live debug panels ────────────────────────────────────────────────────
    // Both flags track panel visibility (LiveDebugView / CPUInspectorView
    // onAppear/onDisappear). While false the snapshots aren't built and the
    // core runs with tracing off — its zero-overhead fast path.
    var liveDebugEnabled: Bool = false {
        didSet { if liveDebugEnabled != oldValue { session?.setLiveDebugEnabled(liveDebugEnabled) } }
    }
    var cpuDebugEnabled: Bool = false {
        didSet { if cpuDebugEnabled != oldValue { session?.setCpuDebugEnabled(cpuDebugEnabled) } }
    }
    var liveDebugSnapshot: LiveDebugSnapshot = .empty
    private(set) var isFrozen: Bool = false
    private(set) var pausedLevel: FreezePanel? = nil
    /// CALCULATOR "FREEZE ON START": freeze before the next program step. Setting it to
    /// false (the ARMED button) disarms it.
    var pendingFreezeOnPCChange: Bool = false {
        didSet { if !pendingFreezeOnPCChange && session?.pcChangeArmed == true { session?.pause(onStart: .none) } }
    }
    /// CPU "FREEZE ON START": freeze when the keyboard scan loop exits.
    var pendingCPUScanLoopFreeze: Bool = false {
        didSet { if !pendingCPUScanLoopFreeze && session?.scanLoopExitArmed == true { session?.pause(onStart: .none) } }
    }
    /// FREEZE, STEP, RESUME of each tab, by the library's rule (LIB-10 AC-18, AC-34).
    private(set) var calculatorButtons = TIButtonUse(pause: false, step: false, resume: false)
    private(set) var cpuButtons = TIButtonUse(pause: false, step: false, resume: false)
    /// FREEZE and FREEZE ON START of each tab (C59-4 AC-5, AC-7: cu59::freezeButtons).
    private(set) var calculatorFreeze = TIFreezeButtons(freeze: false, freezeOnStart: false, armed: false)
    private(set) var cpuFreeze = TIFreezeButtons(freeze: false, freezeOnStart: false, armed: false)

    /// The whole program of the source last shown (frozen PROGRAM STEPS view).
    var programListing: [LiveDebugSnapshot.StepEntry] = []
    var programListingSource: ProgramSource = .userProgram
    private(set) var programListingCurrentIndex: Int = -1

    // ── CPU tab ──────────────────────────────────────────────────────────────
    struct InspectorSnapshot {
        var pc: UInt16
        var opcode: UInt16
        var disasm: String
        var frame: TICpuFrame
        var isHistory: Bool  // true = executed, false = the predicted next instruction
        var isCurrent: Bool  // the newest (running) or where it froze
    }
    /// Running: the newest 32 instructions; frozen: the trace ring plus the next.
    var cpuHistory: [InspectorSnapshot] = []
    var cpuHistoryUpdateID: Int = 0  // incremented on every cpuHistory change
    /// ROM heatmap (5 120 code addresses 0x0000–0x13FF), published at ~10 Hz.
    var romHitCount: [UInt32] = Array(repeating: 0, count: 0x1400)

    // ── Card reader state (the library's `media` topic, C59-7, M-55) ─────────
    enum CardState: Equatable {
        case noCard      // No side in the reader
        case swiping     // A side is passing through; I/O in progress
    }
    var cardState: CardState = .noCard
    var cardPickerMode: CardPickerView.Mode? = nil
    /// The fitted card's name, nil when none.
    var fittedCardName: String?
    /// The saved cards, as Arm Card lists them (AC-14).
    var savedCards: [TISavedCard] = []
    #if os(macOS)
    /// The dropped card in the reader, whose security scope is still open.
    @ObservationIgnored private var droppedCardScope: URL?
    #endif

    // ── Cue card and module ──────────────────────────────────────────────────
    var cueCardContent: CueCardContent? = nil
    /// The fitted card's corner lit while that side is read: 0 left, 1 right.
    var litCardCorner: Int? = nil

    /// The key a physical-keyboard press is holding down (`row * 5 + col`), so
    /// `KeyboardView` shows it depressed as for a pointer press.
    var keyboardHeldKey: Int? = nil

    // ── The calculator (the app session via Bridge/TIAppSession)
    @ObservationIgnored private lazy var platform = SessionDelegate(self)
    /// The ROM files, looked up once at the start (LIB-12). nil: a file of the
    /// full set is missing or bad, and `romsMissingText` says which; then no
    /// controller is made and the window shows the text in place of the
    /// calculator (AC-10; C59-8 AC-19: with the Help item).
    @ObservationIgnored private let romSet: TIRomSet?
    private(set) var romsMissingText: String? = nil
    @ObservationIgnored private var session: TIAppSession?
    /// The shell's models over the session (C59-8 §3.1).
    @ObservationIgnored private(set) var shell: TIShell?
    @ObservationIgnored private var loadingCheck: DispatchWorkItem?
    /// The session, made once the save folder is resolved (start()).
    private var controller: TIAppSession { session! }
    /// Held for the app's lifetime so macOS never App-Naps the background emulation
    /// queue while the window is unfocused/occluded — App Nap can otherwise stall the
    /// emulation thread's step() loop entirely.
    private var appNapActivityToken: NSObjectProtocol?

    init() {
        switch ROMLoader.lookUp() {
        case .success(let set):
            romSet = set
        case .failure(let missing):
            romSet = nil
            romsMissingText = missing.text
            return
        }
        // .latencyCritical: without it macOS coalesces the session thread's
        // timers while the window is covered, the waits overshoot past the
        // pacer's lag limit and the machine falls behind (C59-5 AC-9, S6:
        // 17 s of 30 in M-33 before this option).
        appNapActivityToken = ProcessInfo.processInfo.beginActivity(
            options: [.userInitiated, .idleSystemSleepDisabled, .latencyCritical],
            reason: "TI-59 CPU emulation must keep running at real-time pace in the background"
        )
        Task { await self.start() }
    }

    /// The session, once the save folder is resolved off the main thread (iCloud);
    /// its LIB-18 `start` builds the startup model of the settings (with its
    /// startup preset, or the built-in default).
    func start() async {
        guard let romSet, session == nil else { return }
        await withCheckedContinuation { continuation in
            DispatchQueue.global(qos: .utility).async {
                CardStorage.warmUp()
                continuation.resume()
            }
        }
        let start = TISessionStart()
        start.settingsBase = AppSettings.applicationSupportBase.path
        start.settingsFile = AppSettings.launchSettingsFile
        start.examplesFolder = Bundle.main.url(forResource: "examples", withExtension: nil)?.path ?? ""
        start.saveFolder = CardStorage.directoryURL.path
        start.chosenTraceFolder = AppSettings.chosenTraceFolder()
        let made = TIAppSession(roms: romSet, start: start, delegate: platform)
        session = made
        shell = made.shell
        isTraceAvailable = made.traceAvailable
        if let problem = made.settingsProblem { print("[Settings] \(problem)") }
        #if DEBUG && (os(macOS) || targetEnvironment(simulator))
        // Harness attach (C59-5 AC-10): `calcu attach`, the port of --attach-port, else 59059.
        session?.openAttach(withArguments: ProcessInfo.processInfo.arguments)
        #endif
    }

    /// The app goes to the background (C59-5 AC-8): suspend is carried out —
    /// the TI-58C's memory written — and the session thread stops calling; a
    /// debug freeze survives. On iOS inside a background task until then.
    func suspendForBackground() {
        guard let session else { return }
        #if os(iOS)
        let task = UIApplication.shared.beginBackgroundTask(withName: "Suspend Calc-U-59")
        session.suspend { UIApplication.shared.endBackgroundTask(task) }
        #else
        session.suspend(completion: nil)
        #endif
    }

    func resumeFromBackground() {
        session?.continueSession()
    }

    // MARK: - Keys
    // Keys go through the session: the library's typing (LIB-5) holds them in
    // emulated time, waits for the machine, and the product's keyboard map
    // (app/TITyping) decides what a host key presses.

    /// A pointer on the faceplate (LIB-14 AC-24, AC-43; C59-8 D-9): the
    /// library's rule finds the key (at least 100 ms of emulated time down,
    /// LIB-5 AC-24) or the display (full speed while it is held). (x, y) in
    /// points of the faceplate, `scale` its points per pixel of the base
    /// image. True when a key went down (the key feedback).
    @discardableResult
    func pointerDown(_ pointer: Int64, x: Double, y: Double, scale: Double) -> Bool {
        let pressed = shell?.pointerDown(pointer, x: x, y: y, scale: scale) ?? false
        syncPointerKey()
        return pressed
    }
    @discardableResult
    func pointerMove(_ pointer: Int64, x: Double, y: Double, scale: Double) -> Bool {
        let pressed = shell?.pointerMove(pointer, x: x, y: y, scale: scale) ?? false
        syncPointerKey()
        return pressed
    }
    func pointerUp(_ pointer: Int64) {
        shell?.pointerUp(pointer)
        syncPointerKey()
    }
    func releasePointers() {
        shell?.releaseAllPointers()
        syncPointerKey()
    }
    /// The feedback of a key press and of Reset on iOS ([keyboard] feedback).
    func keyFeedback() {
        #if os(iOS)
        switch keyboardFeedback {
        case .off: break
        case .haptic: haptic.impactOccurred()
        case .click: AudioServicesPlaySystemSound(1104)
        }
        #endif
    }
    func feedbackForReset() { keyFeedback() }
    #if os(iOS)
    @ObservationIgnored private let haptic = UIImpactFeedbackGenerator(style: .rigid)
    #endif

    /// A menu item of the shell by id (C59-7's card items, for the bar's card controls).
    func menuItem(_ id: String) -> TIMenuItem? {
        func find(_ items: [TIMenuItem]) -> TIMenuItem? {
            for i in items {
                if i.itemID == id && i.kind != .separator { return i }
                if let c = find(i.children) { return c }
            }
            return nil
        }
        return find(menus.flatMap(\.items))
    }

    /// The key a pointer holds, drawn pressed at once (row × 5 + col).
    private(set) var pointerKey: Int? = nil
    private func syncPointerKey() {
        let k = shell.map { $0.pointerKey >= 0 ? $0.pointerKey : nil } ?? nil
        if pointerKey != k { pointerKey = k }
    }

    /// A physical key as the host reports it (macOS): its identity, a name for
    /// Escape / Return / Enter, the character it produced, the letter it
    /// carries, the modifiers. The session's keyboard map decides.
    func hostKeyDown(physical: String, named: String, produced: UInt32, carried: UInt32,
                     shift: Bool, control: Bool, option: Bool, command: Bool) {
        session?.hostKeyDownPhysical(physical, named: named, produced: produced, carried: carried,
                                     shift: shift, control: control, alt: option, command: command)
    }
    func hostKeyUp(physical: String) { session?.hostKeyUpPhysical(physical) }
    /// The calculator view lost the focus or went away: every key held comes
    /// up, after its minimum hold (LIB-5 AC-10).
    func keyboardRelease() { session?.focusLost() }

    // MARK: - Printer

    // The PC-100C's controls through the library's `set-control` (LIB-8 P-E2).
    func pressPrinterPrint(_ pressed: Bool) { session?.setPrinterControl("PRINT", on: pressed) }
    func pressPrinterAdv(_ pressed: Bool) { session?.setPrinterControl("ADV", on: pressed) }
    func togglePrinterTrace() { session?.setPrinterControl("TRACE", on: !printerTrace) }
    /// Copy and Cut are the session's (LIB-8 AC-16, AC-18): the picture is the
    /// library's, the clipboard entry the bridge's.
    func copyPaper() { session?.copyPaper() }
    func cutPaper() { session?.cutPaper() }

    // MARK: - The shell (C59-8)

    /// Carries out a menu item, a More item or a bar control's item: its
    /// command goes to the session (a refusal is a notice); a shell action or
    /// a product item is the window's.
    func choose(_ item: TIMenuItem) {
        guard let action = shell?.choose(item) else { return }
        perform(action)
    }

    /// A shell action by id (`TIShellAction…`) or C59-7's card items.
    func perform(_ action: String) {
        switch action {
        case TIShellActionLoadPreset: presetDialogSerial &+= 1
        case TIShellActionSettings: settingsSerial &+= 1
        case TIShellActionHelp: openHelp()
        case TIShellActionAbout:
            #if os(macOS)
            NSApp.orderFrontStandardAboutPanel(options: AppInfo.aboutPanelOptions)
            #endif
        case TIShellActionQuit:
            #if os(macOS)
            NSApp.terminate(nil)
            #endif
        case "arm-card": cardPickerMode = .arm
        case "arm-fresh-card": cardPickerMode = .armFresh
        default: break
        }
    }

    /// Help ▸ "Calc-U-59 Help" (AC-2, LIB-14 AC-32): the help in the browser.
    func openHelp() {
        guard let url = URL(string: TIShell.helpURL) else { return }
        #if os(macOS)
        NSWorkspace.shared.open(url)
        #else
        UIApplication.shared.open(url)
        #endif
    }

    /// A setting changed, applied and written at once (AC-8); nil: its default.
    func setSetting(_ section: String, _ key: String, _ value: String?) {
        shell?.setSetting(section, key, value: value)
    }

    /// The notice alert's buttons (AC-10): OK, or Yes and No for a question.
    func closeNotice() {
        shell?.closeNotice()
        syncNotices()
    }
    func answerNotice(_ yes: Bool) {
        shell?.answerNotice(yes)
        syncNotices()
    }

    fileprivate func syncNotices() {
        let n = shell?.currentNotice
        if currentNotice != n { currentNotice = n }
        let q = shell?.currentNoticeIsQuestion ?? false
        if noticeIsQuestion != q { noticeIsQuestion = q }
    }

    /// Copies the shell's models (TIChangeShell); they change at most with
    /// the control state, not at 60 Hz (design §8).
    private func syncShell() {
        guard let shell else { return }
        if started != shell.started { started = shell.started }
        if modelID != shell.modelID {
            modelID = shell.modelID
            releaseDroppedCardScope()
        }
        menus = shell.menus
        moreItems = shell.moreItems
        barControls = shell.bar
        settingsSections = shell.settingsDialog
        if paperOwner != shell.paperOwner { paperOwner = shell.paperOwner }
        let led = LEDFontStyle(setting: shell.setting("display", "led-style"))
        if ledStyle != led { ledStyle = led }
        let feedback = KeyboardFeedbackType(setting: shell.setting("keyboard", "feedback"))
        if keyboardFeedback != feedback { keyboardFeedback = feedback }
        let page = shell.setting("window", "portrait-debug-page") == "true"
        if portraitDebugPage != page { portraitDebugPage = page }
        syncLoading()
    }

    /// "Loading…" after 500 ms of an operation at full speed (LIB-14 L1): a
    /// look again when the rule says it is due.
    private func syncLoading() {
        guard let shell else { return }
        if loadingShown != shell.loadingShown { loadingShown = shell.loadingShown }
        loadingCheck?.cancel()
        let due = shell.loadingDueIn
        guard due >= 0, !shell.loadingShown else { return }
        let check = DispatchWorkItem { [weak self] in self?.syncLoading() }
        loadingCheck = check
        DispatchQueue.main.asyncAfter(deadline: .now() + due + 0.01, execute: check)
    }

    /// The trace folder the user chose (macOS): `[trace] folder` and its bookmark (LIB-9 AC-34).
    func chooseTraceFolder(_ url: URL) {
        AppSettings.keepTraceFolderBookmark(url)
        setSetting("trace", "folder", url.path)
    }

    // MARK: - Magnetic card reader

    func ejectIfSwiping() {
        if cardState == .swiping { ejectCard() }
    }

    /// Arm Card (AC-14): a saved card fitted for the user to feed.
    func armCard(_ card: TISavedCard) {
        releaseDroppedCardScope()
        session?.armCard(card)
    }

    /// Arm Fresh Card (AC-15): `asked` gets the replace question, or nil once it is fitted.
    func armFreshCard(name: String, confirmed: Bool, asked: @escaping (String?) -> Void) {
        session?.armFreshCard(name, confirmed: confirmed) { [weak self] question, refusal in
            if let question { asked(question); return }
            if let refusal { self?.shell?.pushNotice(refusal); self?.syncNotices() }
            else { self?.releaseDroppedCardScope() }
            asked(nil)
        }
    }

    /// A press at canvas position (x, y), each 0…1: true if it hit a corner of the fitted card.
    func tapCard(x: Double, y: Double) -> Bool { session?.tapCardAt(x: x, y: y) ?? false }

    private func releaseDroppedCardScope() {
        #if os(macOS)
        droppedCardScope?.stopAccessingSecurityScopedResource()
        droppedCardScope = nil
        #endif
    }

    func ejectCard() { session?.ejectCard() }

    // MARK: - Debugger

    // The run control (LIB-10 59): FREEZE is `pause`, STEP `step`, RESUME
    // `resume` of the tab's level; FREEZE ON START `pause-on-start`.
    func freeze(from panel: FreezePanel) { session?.pause(panel == .cpu ? .cpu : .calculator) }
    func resume() { session?.resume() }
    func freezeOnNextPCChange() { session?.pause(onStart: .calculator) }
    func freezeOnScanLoopExit() { session?.pause(onStart: .cpu) }
    func disarmCPUScanLoopFreeze() { session?.pause(onStart: .none) }
    /// STEP in the CALCULATOR panel: one program step (`program`).
    func stepKeycode() { session?.step(.calculator) }
    /// STEP in the CPU panel: one instruction, through a HOLD (`cpu`).
    func stepFrozen() { session?.step(.cpu) }
    func buttons(for panel: FreezePanel) -> TIButtonUse { panel == .cpu ? cpuButtons : calculatorButtons }
    func freezeButtons(for panel: FreezePanel) -> TIFreezeButtons { panel == .cpu ? cpuFreeze : calculatorFreeze }

    func clearRomHeatmap() { session?.clearHeatmap() }

    func toggleDebug() { session?.cycleDebugLevel() }

    /// Empties the LOG tab's view, not the log (C59-4 AC-9).
    func clearDebug() {
        session?.clearLog()
        if !debugDisplayText.isEmpty { debugDisplayText = "" }
        debugClearID &+= 1
    }

    // Dumps for the LOG tab (formats: app/DebugLog.cpp).
    func debugDumpVars() { session?.dumpVars() }
    func debugDumpSCOM() { session?.dumpScom() }
    func debugDumpProg() { session?.dumpProg() }
    func debugDumpMemory() { session?.dumpMemory() }

    func loadASMOverlayFile(_ url: URL) {
        let scoped = url.startAccessingSecurityScopedResource()
        defer { if scoped { url.stopAccessingSecurityScopedResource() } }
        session?.loadAsm(try? String(contentsOf: url, encoding: .utf8), fileName: url.lastPathComponent)
    }

    func runASMOverlay() { session?.runAsm() }
    func clearASMOverlay() { session?.clearAsm() }

    // MARK: - Opening files (LIB-13: the session's `open`, M-56)

    /// The Preset button's file dialog (LIB-13 AC-5, D-21): any file, opened
    /// as what it is (a preset, an ASM overlay, a keystroke program, a card);
    /// its content goes over (D-41: the picked file is readable only inside
    /// its security scope). A refusal is the session's notice (AC-15).
    func loadPreset(_ url: URL) { open(url, way: "dialog") }

    #if os(macOS)
    /// A file dropped on the window (LIB-13 AC-1): `open {way: drop}`; the
    /// session finds out what it is. SwiftUI's drop tells the file's name only
    /// at the drop, so any one file is taken and a name the session does not
    /// take gets its notice there (LIB-13 design §10.9; the bridge's
    /// `wouldOpenFile`, `open-would-take`, is there for a drag that can tell).
    /// A card can be anywhere on disk and is written back while it is
    /// fitted, so its security scope stays open until it is replaced or gone.
    @discardableResult
    func openDroppedFile(_ url: URL) -> Bool {
        open(url, way: "drop")
        return true
    }
    #endif

    private func open(_ url: URL, way: String) {
        let scoped = url.startAccessingSecurityScopedResource()
        // The security scope of an opened card stays open until the reply and
        // the card state say it is fitted (C59-5 design §3.4).
        session?.openFile(try? Data(contentsOf: url), path: url.path, fileName: url.lastPathComponent, way: way) { [weak self] opened in
            guard let self else { return }
            #if os(macOS)
            if opened && self.controller.cardFitted && url.lastPathComponent.lowercased().hasSuffix(".magnetcard.yaml") {
                self.releaseDroppedCardScope()
                if scoped { self.droppedCardScope = url }
                return
            }
            #endif
            if scoped { url.stopAccessingSecurityScopedResource() }
        }
    }

    // MARK: - Changes from the controller

    /// Copy what changed into the observable properties. Each assignment is
    /// guarded: @Observable notifies on every write, even of an identical value.
    fileprivate func apply(_ c: TIChange) {
        if c.contains(.shell) { syncShell() }
        if c.contains(.display) {
            let snap = controller.display
            let g = withUnsafeBytes(of: snap.segments) { Array($0) }
            let lv = withUnsafeBytes(of: snap.digitLevel) { Array($0) }
            let dp = withUnsafeBytes(of: snap.dpLevel) { Array($0) }
            if displaySegments != g { displaySegments = g }
            if displayLevels != lv { displayLevels = lv }
            if dpLevels != dp { dpLevels = dp }
            if calcIndicatorOpacity != controller.calcIndicatorOpacity { calcIndicatorOpacity = controller.calcIndicatorOpacity }
        }
        if c.contains(.cueCard) { cueCardContent = controller.cueCard.swiftValue }
        if c.contains(.cueCard) || c.contains(.card) {
            let lit = controller.litCardCorner
            let corner = lit >= 0 ? lit : nil
            if litCardCorner != corner { litCardCorner = corner }
        }
        if c.contains(.paperCut) {
            printerLines = []
            paperBands = []
            paperDotsTaken = 0
            paperReachUnits = 0
            printerClearID &+= 1
        }
        if c.contains(.printer) {
            // Append what's new (the bridge keeps the paper; .paperCut cleared above).
            if controller.paperLineCount > printerLines.count {
                printerLines.append(contentsOf: controller.paperLines(from: printerLines.count))
            }
            if paperReachUnits != controller.paperReachUnits { paperReachUnits = controller.paperReachUnits }
            if controller.paperDotCount > paperDotsTaken {
                let data = controller.paperDots(from: paperDotsTaken)
                let dots = data.withUnsafeBytes { Array($0.bindMemory(to: TIPaperDot.self)) }
                paperDotsTaken += dots.count
                var bands = paperBands
                let r = TIViewRules.printerDotRadiusFraction
                for d in dots {
                    for y in [d.y - r, d.y + r] {
                        let b = max(0, Int(y / Self.paperBandUnits))
                        if bands.count <= b { bands.append(contentsOf: Array(repeating: [], count: b + 1 - bands.count)) }
                        if bands[b].last.map({ $0.x == d.x && $0.y == d.y }) != true { bands[b].append(d) }
                    }
                }
                paperBands = bands
            }
            if printerTrace != controller.printerTrace { printerTrace = controller.printerTrace }
        }
        if c.contains(.card) {
            let state: CardState = controller.cardPassing ? .swiping : .noCard
            if cardState != state { cardState = state }
            if !controller.cardFitted { releaseDroppedCardScope() }
            if fittedCardName != controller.fittedCardName { fittedCardName = controller.fittedCardName }
            savedCards = controller.savedCards
        }
        if c.contains(.runState) {
            if isFrozen != controller.frozen { isFrozen = controller.frozen }
            let owner: FreezePanel? = switch controller.pausedLevel {
            case .cpu: .cpu
            case .calculator: .calculator
            default: nil
            }
            if pausedLevel != owner { pausedLevel = owner }
            if pendingFreezeOnPCChange != controller.pcChangeArmed { pendingFreezeOnPCChange = controller.pcChangeArmed }
            if pendingCPUScanLoopFreeze != controller.scanLoopExitArmed { pendingCPUScanLoopFreeze = controller.scanLoopExitArmed }
            let calc = controller.buttonUse(for: .calculator), cpu = controller.buttonUse(for: .cpu)
            if !Self.same(calculatorButtons, calc) { calculatorButtons = calc }
            if !Self.same(cpuButtons, cpu) { cpuButtons = cpu }
            let calcFreeze = controller.freezeButtons(for: .calculator), cpuFreezeNow = controller.freezeButtons(for: .cpu)
            if !Self.same(calculatorFreeze, calcFreeze) { calculatorFreeze = calcFreeze }
            if !Self.same(cpuFreeze, cpuFreezeNow) { cpuFreeze = cpuFreezeNow }
            let held: Int? = controller.heldKey >= 0 ? controller.heldKey : nil
            if keyboardHeldKey != held { keyboardHeldKey = held }
        }
        if c.contains(.live) {
            let snap = LiveDebugSnapshot(controller.liveSnapshot)
            if snap != liveDebugSnapshot { liveDebugSnapshot = snap }
        }
        if c.contains(.listing) {
            programListing = controller.programListing.map { LiveDebugSnapshot.StepEntry($0) }
            programListingSource = ProgramSource(raw: controller.programListingSource)
            programListingCurrentIndex = controller.programListingCurrentIndex
        }
        if c.contains(.cpu) {
            cpuHistory = controller.cpuHistory.map { InspectorSnapshot($0) }
            cpuHistoryUpdateID &+= 1
        }
        if c.contains(.heatmap) {
            romHitCount = controller.heatmapCounts.withUnsafeBytes { Array($0.bindMemory(to: UInt32.self)) }
        }
        if c.contains(.log) {
            if debugDisplayText != controller.logText { debugDisplayText = controller.logText }
            let level = DebugLevel(rawValue: controller.debugLevel) ?? .off
            if debugLevel != level { debugLevel = level }
        }
        if c.contains(.asm) {
            if asmFileName != controller.asmFileName { asmFileName = controller.asmFileName }
            if asmWordCount != controller.asmWordCount { asmWordCount = controller.asmWordCount }
            if asmStatusMessage != controller.asmStatus { asmStatusMessage = controller.asmStatus }
            if asmOverlayActive != controller.asmActive { asmOverlayActive = controller.asmActive }
        }
        if c.contains(.trace), cIndicatorDebug != controller.manualTrace { cIndicatorDebug = controller.manualTrace }
        if c.contains(.control) || c.contains(.asm) {
            if canRunASM != controller.asmRunAvailable { canRunASM = controller.asmRunAvailable }
            if traceButtonAvailable != controller.traceButtonAvailable { traceButtonAvailable = controller.traceButtonAvailable }
            if calculatorTabAvailable != controller.calculatorTabAvailable { calculatorTabAvailable = controller.calculatorTabAvailable }
        }
    }


    fileprivate func cardWritten() { if let controller = session { savedCards = controller.savedCards } }

    fileprivate func paperCopied(_ outcome: Int) {
        paperCopyMessage = outcome == 2 ? "Copy failed" : "Copied to clipboard"
        paperCopySerial &+= 1
    }

    private static func same(_ a: TIButtonUse, _ b: TIButtonUse) -> Bool {
        a.pause == b.pause && a.step == b.step && a.resume == b.resume
    }
    private static func same(_ a: TIFreezeButtons, _ b: TIFreezeButtons) -> Bool {
        a.freeze == b.freeze && a.freezeOnStart == b.freezeOnStart && a.armed == b.armed
    }

    fileprivate static func diagnostic(_ message: String) {
        logger.debug("\(message, privacy: .public)")
    }
}

// MARK: - Session delegate (Bridge/TIAppSession ↔ app)

/// TIAppSessionDelegate needs an NSObject; @Observable classes can't be one.
/// All calls arrive on the main thread. The card and constant-memory files are
/// the session's own (LIB-3 AC-30), read and written by the bridge through
/// NSFileCoordinator in the save folder (C59-5 D-6).
final class SessionDelegate: NSObject, TIAppSessionDelegate {
    weak var target: EmulatorViewModel?
    init(_ target: EmulatorViewModel) { self.target = target }

    func showDebugPanel(_ panel: Int) {
        guard let vm = target else { return }
        switch panel {
        case 1: vm.debugTab = .cpu
        case 2: vm.debugTab = .log
        default: vm.debugTab = .live
        }
    }
    func noticesChanged() { target?.syncNotices() }
    func diagnostic(_ message: String) { EmulatorViewModel.diagnostic(message) }
    func changed(_ changes: TIChange) { target?.apply(changes) }
    func cardWritten() { target?.cardWritten() }
    func paperCopied(_ outcome: Int) { target?.paperCopied(outcome) }
}

// ── Live Debug Snapshot ───────────────────────────────────────────────────────
//
// Real-time (60 Hz) view of calculator state: registers, flags, SCOM, program steps.
// Stores decoded/formatted values to minimize work on render thread.

struct LiveDebugSnapshot: Equatable {
    // Data registers — only non-zero values
    struct RegEntry: Equatable {
        var num: Int      // register number (0–99 for R, 0–3 for H)
        var value: Double
        var isHidden: Bool = false  // true for H##, false for R##
    }
    var nonZeroRegs: [RegEntry] = []
    var dataRegCount: Int = 0
    var programRegCount: Int = 0

    // Program steps window — ±5 around current step
    struct StepEntry: Equatable {
        var stepNum: Int       // 000–479
        var keycode: UInt8     // raw 2-digit code
        var mnemonic: String   // e.g. "STO 00", "GTO 27"
        var isCurrent: Bool
    }
    var programWindow: [StepEntry] = []
    var currentStep: Int = -1   // -1 = unknown (SCOM location TBD)

    // HIR registers (stored in SCOM[1..8]; decoded as Double)
    // Each HIR is 16 BCD nibbles: bits 15–3 = mantissa, 2–1 = exponent, 0 = sign
    var hir: [Double] = Array(repeating: 0, count: 8)  // HIR 1–8

    // T register (SCOM[11]; decoded as Double; stack top / last-X equivalent)
    var tRegister: Double = 0
    var xValue: Double = 0
    var xRaw: String = ""              // X's register as "C=…"/"A=…" when it holds no number

    // Calculator flags 0–9 (stored in SCOM; bit mapping TBD experimentally)
    var calcFlags: [Bool] = Array(repeating: false, count: 10)

    // SCOM[0] fields
    var fixIndicator: String = "0"      // Nibble 15 (FIX = value - 2 mod 10)
    var statusRegister: String = "0000000000000000"  // Whole register (HIR 0), nibble 15 first
    var fA: UInt16 = 0                  // FA register (debug display)
    var fB: UInt16 = 0                  // FB register (debug display)
    var engIndicator: String = ""       // "Eng" if FB bit 3 (of 3rd digit) is set, else empty
    var secondIndicator: String = ""    // "2nd" if FA bit 1 is set, else empty
    var invIndicator: String = ""       // "INV" if FA bit 0 is set, else empty

    // Calculator-level status (from SCOM)
    var angleMode: Int = -1  // TILiveDebugSnapshot.angleMode: -1 unknown, 0 DEG, 1 RAD, 2 GRAD

    // SCOM-derived control state
    var prSourceFlag: UInt8 = 0   // Program Source Flag (SCOM[0] nibble 3)
    var programSource: ProgramSource = .userProgram  // prSourceFlag decoded (see ProgramSource)
    var pendingOpsCount: Int = 0  // Number of pending operations in hierarchy stack (SCOM 13)

    // All 16 SCOM rows
    var scomRows: [String] = []

    // Return address stack (from SCOM[14:15]) — 6 levels of subroutine return addresses
    var returnAddressSourceFlags: [UInt8] = Array(repeating: 0, count: 6)  // PRG SOURCE for each level (L1-L6)
    var returnAddresses: [Int] = Array(repeating: 0, count: 6)  // Decoded address values for each level (L1-L6)

    static let empty = LiveDebugSnapshot()
}

// MARK: - Bridge conversion (app/DebugSnapshots.hpp → SwiftUI view model)

extension EmulatorViewModel.InspectorSnapshot {
    init(_ e: TIInspectorEntry) {
        self.init(pc: e.pc, opcode: e.opcode, disasm: e.disasm, frame: e.frame, isHistory: e.isHistory, isCurrent: e.isCurrent)
    }
}

extension LiveDebugSnapshot.StepEntry {
    init(_ e: TIStepEntry) {
        self.init(stepNum: e.stepNum, keycode: e.keycode, mnemonic: e.mnemonic, isCurrent: e.isCurrent)
    }
}

extension LiveDebugSnapshot {
    init(_ t: TILiveDebugSnapshot) {
        self.init()
        nonZeroRegs = t.nonZeroRegs.map { RegEntry(num: $0.num, value: $0.value, isHidden: $0.isHidden) }
        dataRegCount = t.dataRegCount
        programRegCount = t.programRegCount
        programWindow = t.programWindow.map { StepEntry($0) }
        currentStep = t.currentStep
        hir = t.hir.map(\.doubleValue)
        tRegister = t.tRegister
        xValue = t.xValue
        xRaw = t.xRaw
        calcFlags = t.calcFlags.map(\.boolValue)
        fixIndicator = t.fixIndicator
        statusRegister = t.statusRegister
        fA = t.fA
        fB = t.fB
        engIndicator = t.engIndicator
        secondIndicator = t.secondIndicator
        invIndicator = t.invIndicator
        angleMode = t.angleMode
        prSourceFlag = t.prSourceFlag
        programSource = ProgramSource(raw: t.programSource)
        pendingOpsCount = t.pendingOpsCount
        scomRows = t.scomRows
        returnAddressSourceFlags = [UInt8](t.returnAddressSourceFlags)
        returnAddresses = t.returnAddresses.map(\.intValue)
    }
}

