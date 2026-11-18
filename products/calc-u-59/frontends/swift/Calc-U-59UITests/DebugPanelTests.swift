import XCTest

/// The debug panel's GUI scenarios in the Swift frontend. The panel is drawn
/// from the session's product topics; these drive it as a user does.
/// The overlay's scenario needs an `.asm` file on the device and
/// is ScreenshotTests.testAsmOverlay.
#if !os(macOS)
final class DebugPanelTests: XCTestCase {

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }

    /// The app with its debug panel in view: side by side where the window is
    /// wide enough, else the paged layout's debugger page.
    private var harness: HarnessClient!

    private func launchWithDebugPanel() -> XCUIApplication {
        let landscape = UIDevice.current.userInterfaceIdiom == .pad
        let app = launchApp(orientation: landscape ? .landscapeLeft : .portrait,
                            settings: ["window.portrait-debug-page=true"])
        harness = attachHarness()
        waitIdle(harness)
        XCTAssertTrue(app.buttons["btn-preset"].waitForExistence(timeout: 10), "the calculator did not start")
        let left = app.buttons["btn-page-left"]
        if left.exists { left.tap() }   // the paged layout: the debugger is left of the calculator
        XCTAssertTrue(app.buttons["btn-tab-calculator"].waitForExistence(timeout: 5), "the debug panel is not shown")
        return app
    }

    private func waitEnabled(_ e: XCUIElement, _ enabled: Bool, _ what: String, timeout: TimeInterval = 5) {
        let p = XCTNSPredicateExpectation(predicate: NSPredicate(format: "isEnabled == %@", NSNumber(value: enabled)),
                                          object: e)
        XCTAssertEqual(XCTWaiter.wait(for: [p], timeout: timeout), .completed,
                       "\(what) is \(enabled ? "not " : "")enabled")
    }

    /// FREEZE, STEP and RESUME on the
    /// CALCULATOR tab, the run-control buttons of the level `program`.
    @MainActor
    func testCalculatorFreezeStepResume() throws {
        let app = launchWithDebugPanel()
        app.buttons["btn-tab-calculator"].tap()

        let freeze = app.buttons["btn-freeze"], step = app.buttons["btn-step"], resume = app.buttons["btn-resume"]
        XCTAssertTrue(freeze.waitForExistence(timeout: 3), "btn-freeze not found")
        XCTAssertTrue(step.exists, "btn-step not found")
        waitEnabled(freeze, true, "FREEZE while running")
        waitEnabled(step, false, "STEP while running")

        // R/S runs the (empty) program memory from step 000, so program steps
        // pass and the pause and STEP land on one (as the Web scenario runs A).
        // R/S into the machine through attach (key-down, key-up).
        XCTAssertEqual(harness.command("key-down R/S").first, "done")
        XCTAssertEqual(harness.command("wait 150ms").first, "done 2")
        XCTAssertEqual(harness.command("key-up R/S").first, "done")
        freeze.tap()
        waitEnabled(resume, true, "RESUME once paused", timeout: 20)   // the pause landed at a program step boundary
        waitEnabled(step, true, "STEP while paused")
        XCTAssertFalse(freeze.isEnabled, "FREEZE while paused")

        step.press(forDuration: 0.1)   // one program step; the machine stays paused
        waitEnabled(resume, true, "RESUME after STEP")
        attachScreenshot(app, name: "CALCULATOR tab — paused after STEP")

        resume.tap()
        waitEnabled(freeze, true, "FREEZE after RESUME")
        waitEnabled(step, false, "STEP after RESUME")
    }

    /// The three tabs, their labels, and the short forms where
    /// the panel is narrow ("F.START", "DEBUG").
    @MainActor
    func testTabsAndLabels() throws {
        let app = launchWithDebugPanel()
        for (id, label) in [("btn-tab-calculator", "CALCULATOR"), ("btn-tab-cpu", "CPU"), ("btn-tab-log", "LOG")] {
            let tab = app.buttons[id]
            XCTAssertTrue(tab.waitForExistence(timeout: 3), "\(id) not found")
            XCTAssertEqual(tab.label, label)
        }
        app.buttons["btn-tab-calculator"].tap()
        let start = app.buttons["btn-freeze-on-start"]
        XCTAssertTrue(start.waitForExistence(timeout: 3), "btn-freeze-on-start not found")
        XCTAssertTrue(["F.START", "FREEZE ON START"].contains(start.label), "FREEZE ON START reads \(start.label)")
        let header = start.label == "F.START" ? "DEBUG" : "CALCULATOR DEBUG"
        XCTAssertTrue(app.staticTexts[header].exists, "the tab's header is not \(header)")
        app.buttons["btn-tab-cpu"].tap()
        XCTAssertTrue(app.buttons["btn-cpu-step"].waitForExistence(timeout: 3), "the CPU tab did not open")
        XCTAssertTrue(app.buttons["btn-asm-run"].exists, "the ASM OVERLAY box is not on the CPU tab")
        XCTAssertFalse(app.buttons["btn-asm-run"].isEnabled, "Run without an overlay loaded")
        app.buttons["btn-tab-log"].tap()
        for label in ["SCOM", "Memory", "Prog", "Vars"] {
            XCTAssertTrue(app.buttons[label].waitForExistence(timeout: 3), "the LOG tab has no \(label)")
        }
        XCTAssertTrue(app.buttons["btn-trace"].exists, "the LOG tab has no TRACE")
        attachScreenshot(app, name: "Debug panel — LOG tab")
    }
}
#endif
