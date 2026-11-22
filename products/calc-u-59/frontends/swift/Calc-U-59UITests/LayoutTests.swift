import XCTest

/// Layout and visual regression tests for the card reader bar.
///
/// These tests check that toolbar buttons are present and hittable on every
/// device size (iPhone SE, iPhone 16 Pro, iPad). They also attach labelled
/// screenshots so visual issues — such as button labels wrapping onto two lines
/// — can be spotted during result review in Xcode or CI.
///
/// To check for label wrapping automatically, each button's height is compared
/// against a single-line threshold. The threshold is generous (56 pt) to
/// account for Dynamic Type and padding, but a wrapped two-line label would
/// push a .large control well past that.
final class LayoutTests: XCTestCase {

    private let singleLineLabelMaxHeight: CGFloat = 56

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    /// Leave the simulator in portrait after all tests complete so the next run
    /// starts in the expected orientation without a rotation dance.
    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }

    // MARK: - ROM files missing

    /// A file of the full set missing (the DEBUG-only launch variable
    /// CALCU59_ROM_LEAVE_OUT leaves it out of the lookup): the window shows the
    /// lookup's text naming it, in place of the calculator.
    @MainActor
    func testRomMissingShowsMessage() throws {
        XCUIDevice.shared.orientation = .portrait
        let app = XCUIApplication()
        app.launchArguments += ["--settings-file", writeSettingsFile([])]
        app.launchEnvironment["CALCU59_ROM_LEAVE_OUT"] = "TMC0583.txt"
        app.launch()
        let text = app.staticTexts["romsMissingText"]
        XCTAssertTrue(text.waitForExistence(timeout: 10), "no ROMs-missing text")
        XCTAssertTrue(text.label.contains("TMC0583.txt: missing"), "the text does not name the file: \(text.label)")
        XCTAssertTrue(text.label.contains("The full set of ROM files is needed"))
        XCTAssertFalse(app.buttons["bar-more"].exists, "the calculator is shown")
        // No bar; Help still works.
        XCTAssertFalse(app.descendants(matching: .any)["bar-model"].exists, "a control bar without the ROMs")
        XCTAssertTrue(app.buttons["item-help"].exists, "no help without the ROMs")
    }

    // MARK: - Button visibility

    /// The More button must be visible and hittable.
    @MainActor
    func testMoreButtonVisible() throws {
        let app = launchApp(orientation: .portrait)
        let button = app.buttons["bar-more"]
        XCTAssertTrue(button.waitForExistence(timeout: 5), "More button not found")
        XCTAssertTrue(button.isHittable, "More button exists but is not hittable")
    }

    /// Load Preset… and Settings… are in More, not on the bar.
    @MainActor
    func testPresetAndSettingsInMore() throws {
        let app = launchApp(orientation: .portrait)
        XCTAssertTrue(app.buttons["bar-more"].waitForExistence(timeout: 5), "More button not found")
        app.buttons["bar-more"].tap()
        for id in ["more-item-load-preset", "more-item-settings"] {
            XCTAssertTrue(app.descendants(matching: .any)[id].firstMatch.waitForExistence(timeout: 3), "More has no \(id)")
        }
    }

    // MARK: - Label wrapping

    /// The card button's label must not wrap onto two lines.
    ///
    /// A SwiftUI button with a single-line label should be at most ~56 pt
    /// tall. If the label wraps, the button grows taller than that.
    @MainActor
    func testCardButtonLabelSingleLine() throws {
        let app = launchApp(orientation: .portrait)
        let button = app.buttons["bar-media-card-reader"]
        XCTAssertTrue(button.waitForExistence(timeout: 5), "card button not found")
        let height = button.frame.height
        XCTAssertLessThanOrEqual(
            height, singleLineLabelMaxHeight,
            "card button is \(height) pt tall — label may be wrapping"
        )
    }

    /// Landscape: the card button's label must not wrap onto two lines.
    ///
    /// On iPad the landscape layout switches to side-by-side mode where the
    /// card reader bar is always visible — the most likely place for wrapping.
    @MainActor
    func testCardButtonLabelSingleLineLandscape() throws {
        let app = launchApp(orientation: .landscapeLeft)
        let button = app.buttons["bar-media-card-reader"]
        XCTAssertTrue(button.waitForExistence(timeout: 5), "card button not found in landscape")
        let height = button.frame.height
        XCTAssertLessThanOrEqual(
            height, singleLineLabelMaxHeight,
            "card button is \(height) pt tall in landscape — label may be wrapping"
        )
    }

    /// Landscape: a wide window shows the debugger beside the calculator
    /// whatever "Debug Page in Portrait" says — iPad, and iPhone Duo opened
    /// flat (the library's view rule on TIChrome: the room beside the
    /// faceplate takes the printer and the debugger, 256 + 220 pt, and the
    /// window is 560 pt high). Plain iPhones stay in portrait, so the test
    /// skips there.
    @MainActor
    func testWideWindowShowsDebugger() throws {
        let app = launchApp(orientation: .landscapeLeft)
        XCTAssertTrue(app.buttons["bar-more"].waitForExistence(timeout: 5))
        // The window frame includes the safe-area insets the app's layout excludes;
        // close enough on the devices this runs on (iPad, iPhone Duo).
        let frame = app.windows.firstMatch.frame
        let (w, h) = (max(frame.width, frame.height), min(frame.width, frame.height))
        // Page arrows mean the paged (portrait) layout: the device didn't rotate.
        guard !app.buttons["btn-page-right"].exists else {
            throw XCTSkip("App stays in portrait on this device")
        }
        let faceplate = (h - 66) * 2064 / 4075   // the column's bar and insets above and below
        let wide = h >= 560 && w - 18 - faceplate >= 256 + 220
        XCTAssertEqual(app.buttons["btn-tab-calculator"].waitForExistence(timeout: 3), wide,
                       "Debugger visibility wrong for a \(w) × \(h) pt window")
        guard wide else { return }
        // The tabs must take taps: on the open iPhone Duo they once sat in the
        // window's top band, which swallowed them (Shared mobileTopMargin).
        app.buttons["btn-tab-cpu"].tap()
        XCTAssertTrue(app.buttons["btn-cpu-step"].waitForExistence(timeout: 2), "CPU tab didn't open")
        app.buttons["btn-tab-calculator"].tap()
        XCTAssertTrue(app.buttons["btn-freeze"].waitForExistence(timeout: 2), "CALCULATOR tab didn't open")
    }

    // MARK: - Screenshots

    /// Portrait screenshot for visual review.
    @MainActor
    func testScreenshotPortrait() throws {
        let app = launchApp(orientation: .portrait)
        _ = app.descendants(matching: .any)["bar-model"].waitForExistence(timeout: 5)
        attachScreenshot(app, name: "Portrait — \(deviceDescription)")
    }

    /// Landscape screenshot — important for iPad where the layout switches to
    /// side-by-side mode and the card reader bar must fit without wrapping.
    @MainActor
    func testScreenshotLandscape() throws {
        let app = launchApp(orientation: .landscapeLeft)
        _ = app.buttons["bar-more"].waitForExistence(timeout: 5)
        attachScreenshot(app, name: "Landscape — \(deviceDescription)")
    }

    // MARK: - Helpers

    private var deviceDescription: String {
        let device = UIDevice.current
        return "\(device.model) \(device.systemVersion)"
    }
}
