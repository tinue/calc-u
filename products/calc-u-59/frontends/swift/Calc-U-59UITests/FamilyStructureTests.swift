import XCTest

/// The family's scenarios in the Swift frontend (C59-8 AC-16, design §3.9,
/// T1 to T9; ADR 0032: under the IDs of the criteria that own them), on iOS
/// (UIRegressionTests) and on macOS (CalcU59UITestsMac). A row that is "not
/// there" on a system is a test that finds no such element. The app is
/// launched with a settings file of its own and harness attach (AC-21).
final class FamilyStructureTests: XCTestCase {

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    #if !os(macOS)
    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }
    #endif

    // MARK: - Helpers

    /// The app in its usual layout (iPhone portrait, iPad landscape), attached
    /// and idle.
    private func launch(settings: [String] = []) -> (XCUIApplication, HarnessClient) {
        #if os(macOS)
        let app = launchApp(settings: settings)
        #else
        let app = launchApp(orientation: UIDevice.current.userInterfaceIdiom == .pad ? .landscapeLeft : .portrait,
                            settings: settings)
        #endif
        let harness = attachHarness()
        waitIdle(harness)
        return (app, harness)
    }

    private func element(_ app: XCUIApplication, _ id: String) -> XCUIElement {
        app.descendants(matching: .any)[id].firstMatch
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

    #if !os(macOS)
    /// The More menu open (AC-5).
    private func openMore(_ app: XCUIApplication) {
        let more = app.buttons["bar-more"]
        XCTAssertTrue(more.waitForExistence(timeout: 5), "no More button")
        more.tap()
    }
    #endif

    // MARK: - The first check of M-58 (design §8)

    /// The runner writes a settings file and the app under test reads it by
    /// the path of `--settings-file` (C59-8 AC-21; design §8's open check).
    func test_c59_8_ac21_settings_file_by_path() throws {
        let (app, harness) = launch(settings: ["display.led-style=classic", "trace.max-file-size-mb=7"])
        let state = harness.command("state settings").joined(separator: "\n")
        XCTAssertTrue(state.contains("classic"), "the app did not read the file: \(state)")
        XCTAssertTrue(state.contains("\"7\""), "the app did not read the file: \(state)")
        harness.send("quit")
        app.terminate()
    }

    // MARK: - LIB-14 AC-1, AC-4, AC-6, AC-8 (T1, T2, T3)

    func test_lib14_ac1_the_menus() throws {
        let (app, _) = launch()
        #if os(macOS)
        for menu in ["File", "Machine", "Help"] {
            XCTAssertTrue(app.menuBars.menuBarItems[menu].exists, "no \(menu) menu")
        }
        app.menuBars.menuBarItems["File"].click()
        XCTAssertTrue(app.menuItems["Load Preset…"].exists)
        XCTAssertTrue(app.menuItems["Examples"].exists)
        XCTAssertTrue(app.menuItems["Arm Card…"].exists)
        app.typeKey(.escape, modifierFlags: [])
        app.menuBars.menuBarItems["Machine"].click()
        for item in ["Model", "Peripherals", "Reset", "Cold boot", "Resume"] {
            XCTAssertTrue(app.menuItems[item].exists, "Machine has no \(item)")
        }
        XCTAssertFalse(app.menuItems["Resume"].isEnabled, "Resume while running (disabled, not left out)")
        app.typeKey(.escape, modifierFlags: [])
        #else
        // Every item on the bar or behind More (AC-5).
        for id in ["btn-reset", "bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader",
                   "btn-preset", "btn-settings", "bar-more"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 5), "no \(id) on the bar")
        }
        openMore(app)
        for id in ["more-item-examples", "more-item-cold-boot", "more-item-resume", "more-item-help"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 3), "More has no \(id)")
        }
        XCTAssertFalse(element(app, "more-item-resume").isEnabled, "Resume while running")
        attachScreenshot(app, name: "LIB-14 AC-1 — the More menu")
        #endif
        app.terminate()
    }

    func test_lib14_ac4_command_r_resets() throws {
        let (app, harness) = launch()
        #if os(macOS)
        for key in ["1", "2"] { XCTAssertEqual(harness.command("key \(key)").first, "done") }
        XCTAssertTrue(lcd(harness).contains("12"))
        element(app, "faceplate").click()
        app.typeKey("r", modifierFlags: .command)
        waitForOperation(harness, timeout: 15)
        XCTAssertEqual(harness.command("wait 500ms").first, "done 2")
        XCTAssertFalse(lcd(harness).contains("12"), "Command+R did not reset")
        #else
        XCTAssertFalse(app.menuBars.firstMatch.exists, "a menu bar on iOS")   // not there
        _ = harness
        #endif
        app.terminate()
    }

    func test_lib14_ac6_application_menu() throws {
        let (app, _) = launch()
        #if os(macOS)
        app.menuBars.menuBarItems.element(boundBy: 1).click()   // the application menu
        XCTAssertTrue(app.menuItems["About Calc-U-59"].exists)
        XCTAssertTrue(app.menuItems["Settings…"].exists)
        XCTAssertTrue(app.menuItems["Quit Calc-U-59"].exists)
        app.typeKey(.escape, modifierFlags: [])
        #else
        openMore(app)
        XCTAssertFalse(element(app, "more-item-about").exists, "About on iOS")   // not there
        XCTAssertFalse(element(app, "more-item-quit").exists, "Quit on iOS")
        #endif
        app.terminate()
    }

    func test_lib14_ac8_the_bar() throws {
        let (app, harness) = launch()
        for id in ["bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader"] {
            XCTAssertTrue(element(app, id).waitForExistence(timeout: 5), "no \(id) on the bar")
        }
        #if os(macOS)
        XCTAssertFalse(app.buttons["btn-reset"].exists, "Reset on the bar on macOS (AC-4)")
        XCTAssertFalse(app.buttons["btn-preset"].exists, "Preset on the bar on macOS (AC-4)")
        #else
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
        #endif
        app.terminate()
    }

    #if !os(macOS)
    /// EC-3: no magnetic card controls on the TI-58 and TI-58C.
    func test_c59_8_ec3_no_card_controls_on_the_ti58() throws {
        let (app, harness) = launch()
        XCTAssertTrue(element(app, "bar-media-card-reader").waitForExistence(timeout: 5))
        XCTAssertEqual(harness.command("model TI-58", timeout: 20).first, "done")
        let gone = XCTNSPredicateExpectation(predicate: NSPredicate(format: "exists == false"),
                                             object: element(app, "bar-media-card-reader"))
        XCTAssertEqual(XCTWaiter.wait(for: [gone], timeout: 5), .completed, "card controls on the TI-58")
        XCTAssertEqual(element(app, "bar-model").label, "TI-58")
        app.terminate()
    }

    /// EC-4: on an iPhone in portrait every control of the bar can be reached.
    func test_c59_8_ec4_iphone_portrait_reaches_every_control() throws {
        let app = launchApp(orientation: .portrait)
        for id in ["btn-reset", "bar-model", "bar-slot-slot-1", "bar-place-printer", "bar-media-card-reader",
                   "btn-preset", "btn-settings", "bar-more"] {
            let e = element(app, id)
            XCTAssertTrue(e.waitForExistence(timeout: 5), "no \(id)")
            XCTAssertTrue(e.isHittable, "\(id) cannot be reached")
        }
        attachScreenshot(app, name: "C59-8 EC-4 — the bar in portrait")
        app.terminate()
    }

    /// AC-5: More's Cold boot is destructive and asks first.
    func test_c59_8_ac5_more_menu() throws {
        let (app, harness) = launch()
        for key in ["4", "2"] { XCTAssertEqual(harness.command("key \(key)").first, "done") }
        XCTAssertTrue(lcd(harness).contains("42"))
        openMore(app)
        element(app, "more-item-cold-boot").tap()
        let confirm = app.buttons["confirm-cold-boot"].firstMatch
        XCTAssertTrue(confirm.waitForExistence(timeout: 3), "Cold boot did not ask")
        attachScreenshot(app, name: "C59-8 AC-5 — Cold boot asks")
        confirm.tap()
        waitForOperation(harness, timeout: 15)
        XCTAssertFalse(lcd(harness).contains("42"), "the cold boot did not happen")
        app.terminate()
    }
    #endif

    // MARK: - LIB-14 AC-11 (T5)

    func test_lib14_ac11_the_settings() throws {
        let (app, harness) = launch()
        #if os(macOS)
        app.typeKey(",", modifierFlags: .command)
        #else
        app.buttons["btn-settings"].tap()
        #endif
        XCTAssertTrue(element(app, "settings-dialog").waitForExistence(timeout: 5), "no settings")
        var rows = ["setting-startup-model", "setting-preset-TI-59", "setting-preset-TI-58", "setting-preset-TI-58C",
                    "setting-display-led-style", "setting-trace-location", "setting-trace-max-file-size-mb"]
        #if !os(macOS)
        rows += ["setting-keyboard-feedback", "setting-window-portrait-debug-page"]
        #endif
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
        XCTAssertFalse(app.staticTexts["Solid State Module"].exists, "the module row is back (59-9)")
        #if !os(macOS)
        // A change is applied and written at once (AC-8).
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
        attachScreenshot(app, name: "LIB-14 AC-11 — the settings sheet")
        app.buttons["settings-close"].tap()
        XCTAssertTrue(element(app, "settings-dialog").waitForNonExistence(timeout: 5), "Done did not close it")
        #else
        _ = harness
        #endif
        app.terminate()
    }

    #if !os(macOS)
    /// EC-2: a notice that comes while the settings sheet is open shows over it.
    func test_c59_8_ec2_notice_over_the_settings_sheet() throws {
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
    #endif

    // MARK: - LIB-14 AC-15, AC-17, AC-20 (T6)

    func test_lib14_ac15_loading() throws {
        let (app, harness) = launch()
        let path = (NSTemporaryDirectory() as NSString).appendingPathComponent("loading.ti59")
        try "format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n    76 11 09 09 09 09 42 00 76 12 97 00 12 91\nkeys:\n  - full-speed:\n  - key: A\n  - wait: 3000s\n"
            .write(toFile: path, atomically: true, encoding: .utf8)
        let t0 = Date()
        harness.send("open \(path)")   // as a drop does: a preset by its extension (LIB-13)
        var started = false
        let until = Date().addingTimeInterval(10)
        while !started, Date() < until, let l = harness.line(timeout: until.timeIntervalSinceNow) {
            if !l.hasPrefix("message ") { print("[AC-15] reply: \(l)") }
            if l.hasPrefix("message busy ") { print("[AC-15] \(l)") }
            started = l.hasPrefix("message busy ") && l.contains("true")
        }
        XCTAssertTrue(started, "the preset did not start")
        let loading = element(app, "loading")
        let shown = loading.waitForExistence(timeout: 10)
        print("[AC-15] Loading… \(shown ? "shown" : "not shown") after \(Date().timeIntervalSince(t0)) s")
        XCTAssertTrue(shown, "no Loading… during a long load")
        // The keys and the bar under and around it still take touches.
        #if !os(macOS)
        openMore(app)
        XCTAssertTrue(element(app, "more-item-help").waitForExistence(timeout: 3), "the bar took no touch")
        attachScreenshot(app, name: "LIB-14 AC-15 — Loading…")
        app.coordinate(withNormalizedOffset: CGVector(dx: 0.5, dy: 0.1)).tap()   // closes the menu
        #endif
        XCTAssertTrue(loading.waitForNonExistence(timeout: 60), "Loading… stayed")
        app.terminate()
    }

    func test_lib14_ac17_a_notice() throws {
        let (app, harness) = launch()
        // Two notices of the session: an ASM overlay the debug panel's load
        // refuses gives one each (C59-4 EC-2).
        for name in ["first.asm", "second.asm"] {
            _ = harness.command("c59 asm load \(try badOverlay(name))")
        }
        let ok = app.alerts.buttons["OK"].firstMatch   // the alert's OK (iOS drops the identifier `notice-ok` there)
        XCTAssertTrue(ok.waitForExistence(timeout: 5), "no notice")
        XCTAssertTrue(app.alerts.firstMatch.label.contains("first.asm"), "the first notice is not first")
        attachScreenshot(app, name: "LIB-14 AC-17 — the first notice")
        ok.tap()
        let next = app.alerts.buttons["OK"].firstMatch
        if !next.waitForExistence(timeout: 5) { print("[AC-17] \(app.debugDescription)") }
        XCTAssertTrue(next.exists, "the second notice was lost")
        let second = XCTNSPredicateExpectation(predicate: NSPredicate(format: "label CONTAINS 'second.asm'"),
                                               object: app.alerts.firstMatch)
        XCTAssertEqual(XCTWaiter.wait(for: [second], timeout: 5), .completed, "the second notice is not second")
        app.alerts.buttons["OK"].firstMatch.tap()
        XCTAssertTrue(app.alerts.firstMatch.waitForNonExistence(timeout: 5))
        app.terminate()
    }

    func test_lib14_ac20_help() throws {
        let (app, _) = launch()
        #if os(macOS)
        app.menuBars.menuBarItems["Help"].click()
        XCTAssertTrue(app.menuItems["Calc-U-59 Help"].exists)
        XCTAssertTrue(app.menuItems["Calc-U-59 Help"].isEnabled)
        app.typeKey(.escape, modifierFlags: [])
        #else
        openMore(app)
        let help = element(app, "more-item-help")
        XCTAssertTrue(help.waitForExistence(timeout: 3))
        XCTAssertTrue(help.isEnabled)
        help.tap()   // the browser opens calc-u.org/59/
        XCTAssertTrue(app.wait(for: .runningBackground, timeout: 10) || app.state != .runningForeground,
                      "the help did not open")
        app.activate()
        #endif
        app.terminate()
    }

    // MARK: - LIB-14 AC-24, LIB-6 AC-42, LIB-8 AC-31 (T7), AC-20 (T8)

    func test_lib14_ac24_a_key_under_the_pointer() throws {
        let (app, harness) = launch()
        // 1 as a short tap: the typing engine holds it its minimum time (LIB-5 AC-24).
        tapKey(app, x: 0.2812, y: 0.8839, hold: 0)
        tapKey(app, x: 0.4998, y: 0.8831)   // 2
        XCTAssertEqual(harness.command("wait 300ms").first, "done 2")
        XCTAssertTrue(lcd(harness).contains("12"), "the keys under the pointer: \(lcd(harness))")
        app.terminate()
    }

    func test_lib6_ac42_an_example() throws {
        let (app, harness) = launch()
        #if os(macOS)
        app.menuBars.menuBarItems["File"].click()
        app.menuItems["Examples"].hover()
        app.menuItems["Programs"].hover()
        app.menuItems["Sum of N numbers"].click()
        #else
        openMore(app)
        element(app, "more-item-examples").tap()
        let group = app.buttons["Programs"]
        XCTAssertTrue(group.waitForExistence(timeout: 3), "no Programs group")
        group.tap()
        let sum = app.buttons["Sum of N numbers"]
        XCTAssertTrue(sum.waitForExistence(timeout: 3), "no Sum of N numbers")
        sum.tap()
        #endif
        waitForOperation(harness, timeout: 60)
        // C59-8 AC-20: its cue card above the keys, in its template.
        let card = element(app, "cue-card")
        XCTAssertTrue(card.waitForExistence(timeout: 5))
        XCTAssertEqual(card.label, "Sum 1..N")
        XCTAssertEqual(card.value as? String, "CueCard")
        // The display shows its result: 5 A is 15.
        XCTAssertEqual(harness.command("key 5 A", timeout: 20).first, "done")
        XCTAssertEqual(harness.command("wait 3s", timeout: 10).first, "done 2")
        XCTAssertTrue(lcd(harness).contains("15"), "5 A: \(lcd(harness))")
        attachScreenshot(app, name: "LIB-6 AC-42 — Sum of N numbers")
        app.terminate()
    }

    func test_lib8_ac31_the_printer() throws {
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
        #if !os(macOS)
        let right = app.buttons["btn-page-right"]
        if right.exists { right.tap() }   // the paged layout: the paper page
        #endif
        XCTAssertTrue(owner.waitForExistence(timeout: 5))
        XCTAssertEqual(owner.label, "PC-100C")
        element(app, "paper-view-mode").tap()   // the text view
        let text = element(app, "paper-text")
        XCTAssertTrue(text.waitForExistence(timeout: 3))
        XCTAssertTrue(text.label.contains("123"), "the text view: \(text.label)")
        attachScreenshot(app, name: "LIB-8 AC-31 — printed")
        element(app, "paper-cut").tap()
        let emptied = XCTNSPredicateExpectation(predicate: NSPredicate(format: "NOT (label CONTAINS '123')"), object: text)
        XCTAssertEqual(XCTWaiter.wait(for: [emptied], timeout: 5), .completed, "Cut did not empty the paper")
        app.terminate()
    }

    // MARK: - C59-8 AC-15 (T9)

    func test_c59_8_ac15_one_picture_per_layout() throws {
        let (app, _) = launch(settings: ["window.portrait-debug-page=true"])
        attachScreenshot(app, name: "C59-8 AC-15 — the window")
        #if !os(macOS)
        let right = app.buttons["btn-page-right"]
        if right.exists {   // the paged layout: calculator, paper, debugger
            right.tap()
            XCTAssertTrue(element(app, "paper-owner").waitForExistence(timeout: 3))
            attachScreenshot(app, name: "C59-8 AC-15 — the paper page")
            app.buttons["btn-page-right"].tap()
            XCTAssertTrue(app.buttons["btn-tab-calculator"].waitForExistence(timeout: 3), "no debug page")
            attachScreenshot(app, name: "C59-8 AC-15 — the debug page")
        } else {
            XCTAssertTrue(app.buttons["btn-tab-calculator"].exists, "the wide layout has no debug panel")
        }
        #endif
        app.terminate()
    }
}
