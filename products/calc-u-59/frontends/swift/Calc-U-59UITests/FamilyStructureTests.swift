import XCTest

/// The shell's scenarios in the Swift frontend, on iOS
/// (UIRegressionTests) and on macOS (CalcU59UITestsMac). A row that is "not
/// there" on a system is a test that finds no such element. The app is
/// launched with a settings file of its own and harness attach.
final class FamilyStructureTests: XCTestCase {

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }

    // MARK: - Helpers

    /// The app in its usual layout (iPhone portrait, iPad landscape), attached
    /// and idle.
    private func launch(settings: [String] = []) -> (XCUIApplication, HarnessClient) {
        let app = launchApp(orientation: UIDevice.current.userInterfaceIdiom == .pad ? .landscapeLeft : .portrait,
                            settings: settings)
        let harness = attachHarness()
        waitIdle(harness)
        return (app, harness)
    }

    private func element(_ app: XCUIApplication, _ id: String) -> XCUIElement {
        app.descendants(matching: .any)[id].firstMatch
    }

    /// The session's notice: an alert on iOS, a sheet on the Mac window.
    /// Its OK is found by its label (iOS drops the identifier `notice-ok`
    /// on an alert's button).
    private func notice(_ app: XCUIApplication) -> XCUIElement {
        app.alerts.firstMatch
    }

    /// OK of the notice: a tap on iOS; Return on the Mac, where OK is the
    /// sheet's default button and a scripted click does not reach it.
    private func closeNotice(_ app: XCUIApplication) {
        notice(app).buttons["OK"].firstMatch.tap()
    }

    /// The notice's text that contains `text` (the alert's label on iOS, a
    /// text of the sheet on the Mac).
    private func noticeText(_ app: XCUIApplication, contains text: String) -> XCUIElement {
        app.alerts.matching(NSPredicate(format: "label CONTAINS %@", text)).firstMatch
    }

    /// A key of the faceplate by its place on the base image (canvas fractions).
    private func tapKey(_ app: XCUIApplication, x: Double, y: Double, hold: TimeInterval = 0.2) {
        let face = element(app, "faceplate")
        XCTAssertTrue(face.waitForExistence(timeout: 5), "no faceplate")
        let at = face.coordinate(withNormalizedOffset: CGVector(dx: x, dy: y))
        if hold > 0 { at.press(forDuration: hold) } else { at.tap() }
    }

    /// An ASM overlay file without a HEX: line, which the session refuses with a notice naming it.
    private func badOverlay(_ name: String) throws -> String {
        let file = (NSTemporaryDirectory() as NSString).appendingPathComponent(name)
        try "Calc-U-59 ASM 2.0\n".write(toFile: file, atomically: true, encoding: .utf8)
        return file
    }

    private func lcd(_ harness: HarnessClient) -> String {
        let r = harness.command("display-text")
        return r.count > 1 ? r[1] : ""
    }

    /// The More menu open.
    private func openMore(_ app: XCUIApplication) {
        let more = app.buttons["bar-more"]
        XCTAssertTrue(more.waitForExistence(timeout: 5), "no More button")
        more.tap()
    }

    // MARK: - The settings file of a launch

    /// The runner writes a settings file and the app under test reads it by
    /// the path of `--settings-file`.
    func test_settings_file_by_path() throws {
        let (app, harness) = launch(settings: ["display.led-style=classic", "trace.max-file-size-mb=7"])
        let state = harness.command("state settings").joined(separator: "\n")
        XCTAssertTrue(state.contains("classic"), "the app did not read the file: \(state)")
        XCTAssertTrue(state.contains("\"7\""), "the app did not read the file: \(state)")
        harness.send("quit")
        app.terminate()
    }

    // MARK: - Menus and the bar

    func test_the_menus() throws {
        let (app, _) = launch()
        // Every item on the bar or behind More.
        for id in ["btn-reset", "bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader",
                   "btn-preset", "btn-settings", "bar-more"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 5), "no \(id) on the bar")
        }
        openMore(app)
        for id in ["more-item-examples", "more-item-cold-boot", "more-item-resume", "more-item-help"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 3), "More has no \(id)")
        }
        XCTAssertFalse(element(app, "more-item-resume").isEnabled, "Resume while running")
        attachScreenshot(app, name: "The More menu")
        app.terminate()
    }

    func test_command_r_resets() throws {
        let (app, harness) = launch()
        XCTAssertFalse(app.menuBars.firstMatch.exists, "a menu bar on iOS")   // not there
        _ = harness
        app.terminate()
    }

    func test_application_menu() throws {
        let (app, _) = launch()
        openMore(app)
        XCTAssertFalse(element(app, "more-item-about").exists, "About on iOS")   // not there
        XCTAssertFalse(element(app, "more-item-quit").exists, "Quit on iOS")
        app.terminate()
    }

    func test_the_bar() throws {
        let (app, harness) = launch()
        for id in ["bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 5), "no \(id) on the bar")
        }
        // The model picker first, before the module and the printer.
        let model = element(app, "bar-model").frame, slot = element(app, "bar-slot-slot-1").frame
        XCTAssertLessThan(model.minX, slot.minX, "the model picker is not before the module picker")
        XCTAssertEqual(element(app, "bar-model").label, "TI-59")
        element(app, "bar-slot-slot-1").tap()
        XCTAssertTrue(element(app, "item-peripheral-slot-1-none").waitForExistence(timeout: 3), "no None in the module picker")
        XCTAssertTrue(element(app, "item-peripheral-slot-1-LE").exists, "no LE in the module picker")
        element(app, "item-peripheral-slot-1-LE").tap()
        waitForOperation(harness, timeout: 15)
        let state = harness.command("state control-state").joined(separator: "\n")
        XCTAssertTrue(state.contains("LE"), "the module did not change: \(state)")
        app.terminate()
    }

    /// No magnetic card controls on the TI-58 and TI-58C.
    func test_no_card_controls_on_the_ti58() throws {
        let (app, harness) = launch()
        XCTAssertTrue(element(app, "bar-media-card-reader").waitForExistence(timeout: 5))
        XCTAssertEqual(harness.command("model TI-58", timeout: 20).first, "done")
        let gone = XCTNSPredicateExpectation(predicate: NSPredicate(format: "exists == false"),
                                             object: element(app, "bar-media-card-reader"))
        XCTAssertEqual(XCTWaiter.wait(for: [gone], timeout: 5), .completed, "card controls on the TI-58")
        XCTAssertEqual(element(app, "bar-model").label, "TI-58")
        app.terminate()
    }

    /// On an iPhone in portrait every control of the bar can be reached.
    func test_iphone_portrait_reaches_every_control() throws {
        let app = launchApp(orientation: .portrait)
        for id in ["btn-reset", "bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader",
                   "btn-preset", "btn-settings", "bar-more"] {
            let e = element(app, id)
            XCTAssertTrue(e.waitForExistence(timeout: 5), "no \(id)")
            XCTAssertTrue(e.isHittable, "\(id) cannot be reached")
        }
        attachScreenshot(app, name: "The bar in portrait")
        app.terminate()
    }

    /// More's Cold boot is destructive and asks first.
    func test_more_cold_boot_asks_first() throws {
        let (app, harness) = launch()
        for key in ["4", "2"] { XCTAssertEqual(harness.command("key \(key)").first, "done") }
        XCTAssertTrue(lcd(harness).contains("42"))
        openMore(app)
        element(app, "more-item-cold-boot").tap()
        let confirm = app.buttons["confirm-cold-boot"].firstMatch
        XCTAssertTrue(confirm.waitForExistence(timeout: 3), "Cold boot did not ask")
        attachScreenshot(app, name: "Cold boot asks")
        confirm.tap()
        waitForOperation(harness, timeout: 15)
        XCTAssertFalse(lcd(harness).contains("42"), "the cold boot did not happen")
        app.terminate()
    }

    // MARK: - The settings

    func test_the_settings() throws {
        let (app, harness) = launch()
        app.buttons["btn-settings"].tap()
        XCTAssertTrue(element(app, "settings-dialog").waitForExistence(timeout: 5), "no settings")
        var rows = ["setting-startup-model", "setting-preset-TI-59", "setting-preset-TI-58", "setting-preset-TI-58C",
                    "setting-display-led-style", "setting-trace-location", "setting-trace-max-file-size-mb"]
        rows += ["setting-keyboard-feedback", "setting-window-portrait-debug-page"]
        for id in rows {
            // The form is a list: a row further down is there once scrolled to.
            var found = element(app, id).waitForExistence(timeout: 2)
            for _ in 0..<4 where !found {
                element(app, "settings-dialog").swipeUp()
                found = element(app, id).waitForExistence(timeout: 1)
            }
            XCTAssertTrue(found, "no row \(id)")
        }
        element(app, "settings-dialog").swipeDown()
        XCTAssertFalse(app.staticTexts["Solid State Module"].exists, "a module row in the settings (the module is chosen on the bar)")
        // A change is applied and written at once.
        let toggle = element(app, "setting-window-portrait-debug-page")
        toggle.switches.firstMatch.exists ? toggle.switches.firstMatch.tap() : toggle.tap()
        let deadline = Date().addingTimeInterval(5)
        var state = ""
        while Date() < deadline {
            state = harness.command("state settings").joined(separator: "\n")
            if state.contains("\"portrait-debug-page\"") && state.contains("\"true\"") { break }
            Thread.sleep(forTimeInterval: 0.2)
        }
        XCTAssertTrue(state.contains("\"true\""), "the change was not written: \(state)")
        attachScreenshot(app, name: "The settings sheet")
        app.buttons["settings-close"].tap()
        XCTAssertTrue(element(app, "settings-dialog").waitForNonExistence(timeout: 5), "Done did not close it")
        app.terminate()
    }

    /// A notice that comes while the settings sheet is open shows over it.
    func test_notice_over_the_settings_sheet() throws {
        let (app, harness) = launch()
        app.buttons["btn-settings"].tap()
        XCTAssertTrue(element(app, "settings-dialog").waitForExistence(timeout: 5))
        _ = harness.command("c59 asm load \(try badOverlay("notes.asm"))")
        let ok = app.alerts.buttons["OK"].firstMatch   // the alert's OK (iOS drops the identifier `notice-ok` there)
        XCTAssertTrue(ok.waitForExistence(timeout: 5), "no notice over the sheet")
        ok.tap()
        XCTAssertTrue(element(app, "settings-dialog").exists, "the sheet went with the notice")
        app.terminate()
    }

    // MARK: - Loading…, notices, Help

    func test_loading_sign_during_a_long_load() throws {
        let (app, harness) = launch()
        let path = (NSTemporaryDirectory() as NSString).appendingPathComponent("loading.ti59")
        try "format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n    76 11 09 09 09 09 42 00 76 12 97 00 12 91\nkeys:\n  - full-speed:\n  - key: A\n  - wait: 3000s\n"
            .write(toFile: path, atomically: true, encoding: .utf8)
        let t0 = Date()
        harness.send("open \(path)")   // as a drop does: a preset by its extension
        var started = false
        let until = Date().addingTimeInterval(10)
        while !started, Date() < until, let l = harness.line(timeout: until.timeIntervalSinceNow) {
            if !l.hasPrefix("message ") { print("[loading] reply: \(l)") }
            if l.hasPrefix("message busy ") { print("[loading] \(l)") }
            started = l.hasPrefix("message busy ") && l.contains("true")
        }
        XCTAssertTrue(started, "the preset did not start")
        let loading = element(app, "loading")
        let shown = loading.waitForExistence(timeout: 10)
        print("[loading] Loading… \(shown ? "shown" : "not shown") after \(Date().timeIntervalSince(t0)) s")
        XCTAssertTrue(shown, "no Loading… during a long load")
        // The keys and the bar under and around it still take touches.
        openMore(app)
        XCTAssertTrue(element(app, "more-item-help").waitForExistence(timeout: 3), "the bar took no touch")
        attachScreenshot(app, name: "Loading…")
        app.coordinate(withNormalizedOffset: CGVector(dx: 0.5, dy: 0.1)).tap()   // closes the menu
        XCTAssertTrue(loading.waitForNonExistence(timeout: 60), "Loading… stayed")
        app.terminate()
    }

    func test_notices_one_after_the_other() throws {
        let (app, harness) = launch()
        // Two notices of the session: an ASM overlay the debug panel's load
        // refuses gives one each.
        for name in ["first.asm", "second.asm"] {
            _ = harness.command("c59 asm load \(try badOverlay(name))")
        }
        let ok = notice(app).buttons["OK"].firstMatch
        XCTAssertTrue(ok.waitForExistence(timeout: 5), "no notice")
        XCTAssertTrue(noticeText(app, contains: "first.asm").waitForExistence(timeout: 2), "the first notice is not first")
        attachScreenshot(app, name: "The first notice")
        closeNotice(app)
        let next = notice(app).buttons["OK"].firstMatch
        if !next.waitForExistence(timeout: 5) { print("[notices] \(app.debugDescription)") }
        XCTAssertTrue(next.exists, "the second notice was lost")
        XCTAssertTrue(noticeText(app, contains: "second.asm").waitForExistence(timeout: 5), "the second notice is not second")
        closeNotice(app)
        XCTAssertTrue(notice(app).waitForNonExistence(timeout: 5))
        app.terminate()
    }

    func test_help() throws {
        let (app, _) = launch()
        openMore(app)
        let help = element(app, "more-item-help")
        XCTAssertTrue(help.waitForExistence(timeout: 3))
        XCTAssertTrue(help.isEnabled)
        help.tap()   // the browser opens calc-u.org/59/
        XCTAssertTrue(app.wait(for: .runningBackground, timeout: 10) || app.state != .runningForeground,
                      "the help did not open")
        app.activate()
        app.terminate()
    }

    // MARK: - Keys, examples, the printer

    func test_a_key_under_the_pointer() throws {
        let (app, harness) = launch()
        // 1 as a short tap: the typing engine holds it its minimum time.
        tapKey(app, x: 0.2812, y: 0.8839, hold: 0)
        tapKey(app, x: 0.4998, y: 0.8831)   // 2
        XCTAssertEqual(harness.command("wait 300ms").first, "done 2")
        XCTAssertTrue(lcd(harness).contains("12"), "the keys under the pointer: \(lcd(harness))")
        app.terminate()
    }

    func test_an_example() throws {
        let (app, harness) = launch()
        openMore(app)
        element(app, "more-item-examples").tap()
        let group = app.buttons["Programs"]
        XCTAssertTrue(group.waitForExistence(timeout: 3), "no Programs group")
        group.tap()
        let sum = app.buttons["Sum of N numbers"]
        XCTAssertTrue(sum.waitForExistence(timeout: 3), "no Sum of N numbers")
        sum.tap()
        waitForOperation(harness, timeout: 60)
        // Its cue card above the keys, in its template.
        let card = element(app, "cue-card")
        XCTAssertTrue(card.waitForExistence(timeout: 5))
        XCTAssertEqual(card.label, "Sum 1..N")
        XCTAssertEqual(card.value as? String, "CueCard")
        // The display shows its result: 5 A is 15.
        XCTAssertEqual(harness.command("key 5 A", timeout: 20).first, "done")
        XCTAssertEqual(harness.command("wait 3s", timeout: 10).first, "done 2")
        XCTAssertTrue(lcd(harness).contains("15"), "5 A: \(lcd(harness))")
        attachScreenshot(app, name: "Example — Sum of N numbers")
        app.terminate()
    }

    func test_the_printer() throws {
        let (app, harness) = launch()
        let owner = element(app, "paper-owner")
        let plug = element(app, "bar-place-printer")
        XCTAssertTrue(plug.waitForExistence(timeout: 5))
        XCTAssertEqual(plug.value as? String, "Off")
        plug.tap()
        waitForOperation(harness, timeout: 20)
        XCTAssertEqual(element(app, "bar-place-printer").value as? String, "On")
        XCTAssertEqual(harness.command("key 1 2 3", timeout: 10).first, "done")
        XCTAssertEqual(harness.command("key 2nd Prt", timeout: 10).first, "done")
        XCTAssertEqual(harness.command("wait 2s", timeout: 10).first, "done 2")
        let right = app.buttons["btn-page-right"]
        if right.exists { right.tap() }   // the paged layout: the paper page
        XCTAssertTrue(owner.waitForExistence(timeout: 5))
        XCTAssertEqual(owner.label, "PC-100C")
        element(app, "paper-view-mode").tap()   // the text view
        let text = element(app, "paper-text")
        XCTAssertTrue(text.waitForExistence(timeout: 3))
        XCTAssertTrue(text.label.contains("123"), "the text view: \(text.label)")
        attachScreenshot(app, name: "The printer — printed")
        element(app, "paper-cut").tap()
        let emptied = XCTNSPredicateExpectation(predicate: NSPredicate(format: "NOT (label CONTAINS '123')"), object: text)
        XCTAssertEqual(XCTWaiter.wait(for: [emptied], timeout: 5), .completed, "Cut did not empty the paper")
        app.terminate()
    }

    /// A cut empties the dot strip while its bands are in view; the app
    /// keeps running and prints on the new paper.
    func test_paper_cut_in_dot_mode() throws {
        let (app, harness) = launch()
        let plug = element(app, "bar-place-printer")
        XCTAssertTrue(plug.waitForExistence(timeout: 5))
        plug.tap()
        waitForOperation(harness, timeout: 20)
        XCTAssertEqual(element(app, "bar-place-printer").value as? String, "On")
        let right = app.buttons["btn-page-right"]
        if right.exists { right.tap() }   // the paged layout: the paper page
        let owner = element(app, "paper-owner")
        XCTAssertTrue(owner.waitForExistence(timeout: 5))
        for round in 1...3 {
            // Several lines, so that more than one band is in view.
            for n in ["1", "2 2", "3 3 3", "4 4 4 4"] {
                XCTAssertEqual(harness.command("key \(n) 2nd Prt", timeout: 10).first, "done")
            }
            XCTAssertEqual(harness.command("wait 2s", timeout: 10).first, "done 2")
            element(app, "paper-cut").tap()
            XCTAssertEqual(harness.command("key CLR 2nd Prt", timeout: 10).first, "done", "round \(round)")
        }
        XCTAssertEqual(app.state, .runningForeground, "the app ended after a cut in dot mode")
        element(app, "paper-view-mode").tap()   // the text view: the last print only
        let text = element(app, "paper-text")
        XCTAssertTrue(text.waitForExistence(timeout: 3))
        XCTAssertFalse(text.label.contains("4444"), "the cut kept the paper: \(text.label)")
        app.terminate()
    }

    // MARK: - The layouts

    func test_one_picture_per_layout() throws {
        let (app, _) = launch(settings: ["window.portrait-debug-page=true"])
        attachScreenshot(app, name: "Layout — the window")
        let right = app.buttons["btn-page-right"]
        if right.exists {   // the paged layout: calculator, paper, debugger
            right.tap()
            XCTAssertTrue(element(app, "paper-owner").waitForExistence(timeout: 3))
            attachScreenshot(app, name: "Layout — the paper page")
            app.buttons["btn-page-right"].tap()
            XCTAssertTrue(app.buttons["btn-tab-calculator"].waitForExistence(timeout: 3), "no debug page")
            attachScreenshot(app, name: "Layout — the debug page")
        } else {
            XCTAssertTrue(app.buttons["btn-tab-calculator"].exists, "the wide layout has no debug panel")
        }
        app.terminate()
    }
}
