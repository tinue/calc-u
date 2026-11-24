import XCTest

/// The shell models through the real bridge (Longer,
/// CalcU59SwiftUnitTests): TIShell computes the library's shell models over
/// a second session of the app's (TestSession) as the window draws them.
final class ShellBridgeTests: XCTestCase {

    private func started() throws -> TestSession {
        let t = try TestSession()
        XCTAssertTrue(TestSession.until(15) { t.session.shell.started && t.session.shell.item("examples") != nil },
                      "no control state or no preset list came")
        return t
    }

    /// File, Machine and Help, in that order, with their items.
    func test_menus_through_the_bridge() throws {
        let t = try started()
        let menus = t.session.shell.menus
        XCTAssertEqual(menus.map(\.label), ["File", "Machine", "Help"])
        XCTAssertEqual(menus.first?.items.first?.itemID, "load-preset")
        XCTAssertEqual(menus.dropFirst().first?.items.filter { $0.kind != .separator }.map(\.itemID),
                       ["model", "peripherals", "reset", "cold-boot", "resume"])
        XCTAssertEqual(menus.last?.items.first?.label, "Calc-U-59 Help")
        // The TI-59 has the card items.
        XCTAssertNotNil(t.session.shell.item("arm-card"))
        // The model picker first, More last with every menu item under
        // its heading; the PC-100C on the paper panel.
        let bar = t.session.shell.bar
        XCTAssertEqual(bar.first?.kind, .modelPicker)
        XCTAssertEqual(bar.first?.shown, "59")
        XCTAssertEqual(bar.last?.kind, .more)
        XCTAssertFalse(bar.contains { $0.kind == .placeButton })
        XCTAssertEqual(t.session.shell.paperPlaces.map(\.controlID), ["printer"])
        let more = bar.first { $0.kind == .more }
        XCTAssertEqual(more?.choices.filter { $0.kind != .separator }.map(\.itemID),
                       ["heading-file", "load-preset", "examples", "arm-card", "arm-fresh-card", "settings",
                        "heading-machine", "model", "peripherals", "reset", "cold-boot", "resume", "heading-help", "help", "show-app-log"])
    }

    /// An entry that is not available now is disabled, with its reason.
    func test_disabled_with_reason() throws {
        let t = try started()
        let resume = try XCTUnwrap(t.session.shell.item("resume"))
        XCTAssertFalse(resume.enabled, "Resume while running")
        XCTAssertFalse(resume.reason.isEmpty, "Resume has no reason")
        XCTAssertNil(t.session.shell.choose(resume), "a disabled item is not carried out")
        let reset = try XCTUnwrap(t.session.shell.item("reset"))
        XCTAssertTrue(reset.enabled)
    }

    /// The key under the pointer is the
    /// library's rule on the faceplate's hit map: each key's centre presses
    /// that key; the display holds no key.
    func test_hit_rule_matches_the_library() throws {
        let t = try started()
        let shell = t.session.shell
        let width: Double = 400, scale = width / Double(TISharedTables.canvasWidthPixels)
        let height = width / Double(TISharedTables.canvasAspect)
        for row in 0..<9 {
            for col in 0..<5 {
                let r = TISharedTables.keyRect(row: row, col: col)
                let x = Double(r.midX) * width
                let y = (Double(TISharedTables.keyboardYStart) + Double(r.midY) * Double(TISharedTables.keyboardYScale)) * height
                XCTAssertTrue(shell.pointerDown(1, x: x, y: y, scale: scale), "no key at row \(row) col \(col)")
                XCTAssertEqual(shell.pointerKey, row * 5 + col)
                shell.pointerUp(1)
                XCTAssertEqual(shell.pointerKey, -1)
            }
        }
        let d = TISharedTables.displayRect
        XCTAssertFalse(shell.pointerDown(1, x: Double(d.midX) * width, y: Double(d.midY) * height, scale: scale))
        XCTAssertEqual(shell.pointerKey, -1)
        shell.pointerUp(1)
    }

    /// The About texts and links are the product's, as in every frontend.
    func test_about_texts_are_the_products() {
        XCTAssertEqual(TIShell.aboutLines.first, "Calc-U-59 Calculator Emulation")
        XCTAssertEqual(TIShell.aboutLinks.map { $0.first ?? "" }, ["GitHub", "Support", "Privacy"])
        XCTAssertEqual(TIShell.helpURL, "https://calc-u.org/59/")
    }

    /// The rows of this system's settings; a change is
    /// written at once and comes back in the settings topic.
    func test_settings_rows_and_a_change() throws {
        let t = try started()
        XCTAssertTrue(TestSession.until(5) { !t.session.shell.settingsDialog.isEmpty })
        let titles = t.session.shell.settingsDialog.map(\.title)
        XCTAssertEqual(titles, ["General", "Startup presets", "Display", "Keyboard", "Layout", "Tracing", "App log"])
        // Each startup preset row ends with "Choose…" (the file importer).
        let presets = t.session.shell.settingsDialog.first { $0.title == "Startup presets" }?.rows ?? []
        XCTAssertFalse(presets.isEmpty)
        XCTAssertTrue(presets.allSatisfy { $0.chooseLabel == "Choose…" })
        XCTAssertEqual(t.session.shell.setting("display", "led-style"), "modernized")
        t.session.shell.setSetting("display", "led-style", value: "classic")
        XCTAssertTrue(TestSession.until(5) { t.session.shell.setting("display", "led-style") == "classic" })
        let text = try String(contentsOf: t.folder.appendingPathComponent("Calc-U-59.ini"), encoding: .utf8)
        XCTAssertTrue(text.contains("led-style = classic"), text)
    }

    /// Copy of the paper: one pasteboard entry with the picture and the text.
    func test_paper_copy_forms() throws {
        let t = try started()
        // The PC-100C into its place (its button on the paper panel), then a feed.
        let printer = try XCTUnwrap(t.session.shell.paperPlaces.first { $0.kind == .placeButton && $0.controlID == "printer" })
        let attach = try XCTUnwrap(printer.choices.first)
        XCTAssertNil(t.session.shell.choose(attach))
        XCTAssertTrue(TestSession.until(15) { t.session.shell.paperOwner == "PC-100C" }, "the PC-100C was not attached")
        XCTAssertTrue(TestSession.until(10) { t.session.shell.started && !(t.session.shell.item("reset")?.enabled == false) })
        TestSession.run(1)
        t.session.setPrinterControl("ADV", on: true)
        TestSession.run(0.5)
        t.session.setPrinterControl("ADV", on: false)
        XCTAssertTrue(TestSession.until(10) { t.session.paperLineCount > 0 }, "the feed did not reach the paper")
        UIPasteboard.general.items = []
        t.session.copyPaper()
        XCTAssertTrue(TestSession.until(10) { UIPasteboard.general.items.count > 0 }, "nothing copied")
        XCTAssertEqual(UIPasteboard.general.items.count, 1)
        XCTAssertNotNil(UIPasteboard.general.items.first?["public.png"], "no picture in the entry")
    }

    /// "Loading…" while an operation runs at full
    /// speed for more than half a second, gone when it ends.
    func test_loading_during_a_long_load() throws {
        let t = try started()
        t.session.loadPresetContent(Data("format-version: 2\nmodel: TI-59\nprogram:\n  keycodes: |\n    76 11 09 09 09 09 42 00 76 12 97 00 12 91\nkeys:\n  - full-speed:\n  - key: A\n  - wait: 3000s\n".utf8),
                                    name: "long.ti59")
        XCTAssertTrue(TestSession.until(10) { t.session.shell.loadingShown }, "no Loading…")
        XCTAssertTrue(TestSession.until(120) { !t.session.shell.loadingShown }, "Loading… stayed")
    }


    /// An on-screen key goes through the typing engine,
    /// its minimum hold included: a press let go at once still holds the key.
    func test_screen_key_minimum_hold() throws {
        let t = try started()
        TestSession.run(0.5)
        let width = 400.0, height = width / Double(TISharedTables.canvasAspect)
        let r = TISharedTables.keyRect(row: 7, col: 1)
        let x = Double(r.midX) * width
        let y = (Double(TISharedTables.keyboardYStart) + Double(r.midY) * Double(TISharedTables.keyboardYScale)) * height
        func digits() -> [UInt8] { withUnsafeBytes(of: t.session.display.segments) { Array($0) } }
        let before = digits()
        XCTAssertTrue(t.session.shell.pointerDown(3, x: x, y: y, scale: width / Double(TISharedTables.canvasWidthPixels)))
        t.session.shell.pointerUp(3)
        // Let go at once, the key was held long enough for the ROM: the display shows 1.
        XCTAssertTrue(TestSession.until(5) { digits() != before }, "the short press did not reach the machine")
    }
}
