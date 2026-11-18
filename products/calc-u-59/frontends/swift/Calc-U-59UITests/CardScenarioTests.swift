import XCTest

/// The magnetic card scenarios in the Swift frontend (the same as the Web
/// client's): a fresh card armed
/// through the bar, written by a program and listed by Arm Card; a card
/// armed and read when its corner is pressed. The presets go in through
/// attach (`open`), the keys of a program too; the card controls, the
/// dialogs and the corner are touched as a user does.
final class CardScenarioTests: XCTestCase {

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    #if !os(macOS)
    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }
    #endif

    private static let head = "format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\nprinter: PC-100C\n"
    /// Lbl A 1 2 3 R/S: A shows 123.
    private static let program123 = head + "program:\n  keycodes: |\n    76 11 01 02 03 91\n"
    /// The same, then CLR 1 2nd Write at regular speed: the machine waits for a card.
    private static let program123Write = program123 + "keys:\n  - regular-speed:\n  - key: CLR 1 Write\n"
    /// A fresh machine: no program.
    private static let fresh = head + "partition: 479\n"

    private func element(_ app: XCUIApplication, _ id: String) -> XCUIElement {
        app.descendants(matching: .any)[id].firstMatch
    }

    private func launch() -> (XCUIApplication, HarnessClient) {
        #if os(macOS)
        let app = launchApp()
        #else
        let app = launchApp(orientation: .portrait)
        #endif
        let harness = attachHarness()
        waitIdle(harness)
        return (app, harness)
    }

    /// A preset through attach (`open`, as a drop), waited for.
    private func load(_ harness: HarnessClient, _ text: String, _ name: String) throws {
        let path = (NSTemporaryDirectory() as NSString).appendingPathComponent(name)
        try text.write(toFile: path, atomically: true, encoding: .utf8)
        XCTAssertEqual(harness.command("open \(path)", timeout: 60).first, "done", "\(name) did not load")
        waitIdle(harness)
    }

    /// The fitted card's top right corner: side 1 (the card's
    /// place and corners of app/KeyboardLayout.hpp).
    private func pressCorner(_ app: XCUIApplication) {
        element(app, "faceplate").coordinate(withNormalizedOffset: CGVector(dx: 0.9, dy: 0.121)).press(forDuration: 0.1)
    }

    /// Arm Fresh Card with `name`, fed while the program waits in `1 2nd Write`.
    private func writeFreshCard(_ app: XCUIApplication, _ harness: HarnessClient, name: String) throws {
        try load(harness, Self.program123Write, "write.ti59")
        let fresh = element(app, "bar-media-card-reader-fresh")
        XCTAssertTrue(fresh.waitForExistence(timeout: 5), "no Arm Fresh Card")
        fresh.tap()
        let field = app.textFields["card-name"]
        XCTAssertTrue(field.waitForExistence(timeout: 5), "no name field")
        field.tap()
        field.typeText(name)
        let arm = app.buttons["card-arm"]
        if !arm.waitForExistence(timeout: 3) { print("[cards] \(app.debugDescription)") }
        arm.tap()
        XCTAssertTrue(field.waitForNonExistence(timeout: 5), "the dialog stayed")
        XCTAssertEqual(element(app, "bar-media-card-reader").label, "Arm Card", "fed before the corner was pressed")
        pressCorner(app)
        // In the reader until the write is done, then given back (and saved).
        let back = XCTNSPredicateExpectation(predicate: NSPredicate(format: "label == 'Arm Card'"),
                                             object: element(app, "bar-media-card-reader"))
        XCTAssertEqual(harness.command("wait 4s", timeout: 20).first, "done 2")
        XCTAssertEqual(XCTWaiter.wait(for: [back], timeout: 15), .completed, "the card was not given back")
    }

    /// The names Arm Card lists.
    private func listed(_ app: XCUIApplication, _ name: String) -> Bool {
        let arm = element(app, "bar-media-card-reader")
        let ready = XCTNSPredicateExpectation(predicate: NSPredicate(format: "label == 'Arm Card' AND isEnabled == true"), object: arm)
        XCTAssertEqual(XCTWaiter.wait(for: [ready], timeout: 10), .completed, "Arm Card cannot be used")
        arm.tap()
        let cancel = app.buttons["card-cancel"]
        if !cancel.waitForExistence(timeout: 5) { print("[cards] \(app.debugDescription)") }
        let found = app.buttons[name].exists || app.staticTexts[name].exists
        cancel.tap()
        return found
    }

    /// A fresh card armed, written by a program,
    /// listed by Arm Card; its cue card has its name.
    func test_fresh_card_written_and_listed() throws {
        let (app, harness) = launch()
        let name = "prog\(Int.random(in: 1000...9999))"
        try writeFreshCard(app, harness, name: name)
        XCTAssertTrue(listed(app, name), "Arm Card does not list \(name)")
        attachScreenshot(app, name: "Fresh card — written and listed")
        app.terminate()
    }

    /// A card armed and read when its corner is pressed.
    func test_card_read_by_its_corner() throws {
        let (app, harness) = launch()
        let name = "read\(Int.random(in: 1000...9999))"
        try writeFreshCard(app, harness, name: name)
        try load(harness, Self.fresh, "fresh.ti59")   // a fresh machine: no program, nothing fitted
        element(app, "bar-media-card-reader").tap()
        let card = app.buttons[name]
        XCTAssertTrue(card.waitForExistence(timeout: 5), "\(name) not listed")
        card.tap()
        XCTAssertTrue(card.waitForNonExistence(timeout: 5), "the dialog stayed")
        pressCorner(app)   // the ROM reads side 1 into program memory
        XCTAssertEqual(harness.command("wait 4s", timeout: 20).first, "done 2")
        XCTAssertEqual(harness.command("key CLR A", timeout: 20).first, "done")
        XCTAssertEqual(harness.command("wait 2s", timeout: 10).first, "done 2")
        let lcd = harness.command("display-text")
        XCTAssertTrue(lcd.count > 1 && lcd[1].contains("123"), "the program was not read: \(lcd)")
        app.terminate()
    }
}
