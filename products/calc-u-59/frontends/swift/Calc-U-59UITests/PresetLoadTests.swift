import XCTest

/// Regression tests for loading a preset via the Preset button.
///
/// This test drives the iOS system document picker, which is inherently brittle:
/// navigation element labels ("Durchsuchen", "Auf meinem iPhone") are
/// locale-dependent and Apple can restructure the picker between iOS releases.
/// The filename itself ("test_diag.ti59") and the wait through harness attach
/// are stable. If this test breaks after an Xcode/iOS upgrade, first inspect
/// the picker's element tree with Xcode's Accessibility Inspector or a
/// print(app.debugDescription) dump.
///
/// Prerequisite: diag.ti59 must be present on "On My iPhone" in the simulator.
/// The setup-simulator-state-files script installs it.
final class PresetLoadTests: XCTestCase {

    override func setUpWithError() throws {
        continueAfterFailure = false
    }

    override class func tearDown() {
        XCUIDevice.shared.orientation = .portrait
        super.tearDown()
    }

    // MARK: - Tests

    /// A preset loaded through the file picker runs. The load's
    /// end is the reply to the bare `wait` through attach; the
    /// diagnostic shows its result on the display.
    @MainActor
    func testLoadDiagPreset() throws {
        let orientation: UIDeviceOrientation = UIDevice.current.userInterfaceIdiom == .pad
            ? .landscapeLeft : .portrait
        let app = launchApp(orientation: orientation)
        let harness = attachHarness()
        waitIdle(harness)
        loadPreset(app, file: "test_diag.ti59", idleTimeout: 60, harness: harness)
        attachScreenshot(app, name: "After loading test_diag.ti59")
        XCTAssertEqual(harness.command("wait 2s", timeout: 10).first, "done 2")
        let lcd = harness.command("display-text")
        XCTAssertTrue(lcd.count > 1 && !lcd[1].trimmingCharacters(in: .whitespaces).isEmpty, "lcd: \(lcd)")
        app.terminate()
    }
}
