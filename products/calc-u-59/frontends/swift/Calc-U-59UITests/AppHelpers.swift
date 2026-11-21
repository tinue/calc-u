import XCTest

// MARK: - App launch helpers
//
// The app has no aid for its tests. A test hands
// the app a settings file of its own with `--settings-file` (Debug builds
// only), so it neither reads nor changes the simulator's settings, and
// reaches the session through harness attach (`--attach-port`): the module
// is set with `attach slot-1 <id>`, a key into a running program is
// `key-down`/`key-up`, and the end of a load started with a tap is the reply
// to the bare `wait`, after the busy topic said an operation runs.

/// The port of a launch's harness attach: one per simulator.
let uiTestAttachPort: UInt16 = 59162

extension XCTestCase {

    /// A settings file for one launch, with these values ("section.key=value"),
    /// in the runner's temporary folder; the app is given its full path. The
    /// simulator's app reads it there; the Mac runner is sandboxed, and the
    /// Mac Debug app may read its temporary folder
    /// (Calc-U-59-Debug-macOS.entitlements).
    func writeSettingsFile(_ values: [String]) -> String {
        var sections: [String: [String]] = [:]
        var order: [String] = []
        for v in values {
            guard let dot = v.firstIndex(of: "."), let eq = v.firstIndex(of: "="), dot < eq else { continue }
            let section = String(v[..<dot])
            if sections[section] == nil { order.append(section) }
            sections[section, default: []].append("\(v[v.index(after: dot)..<eq]) = \(v[v.index(after: eq)...])")
        }
        var text = "format-version = 1\n"
        for s in order { text += "\n[\(s)]\n" + sections[s]!.joined(separator: "\n") + "\n" }
        let path = (NSTemporaryDirectory() as NSString).appendingPathComponent("uitest-\(UUID().uuidString).ini")
        do {
            try text.write(toFile: path, atomically: true, encoding: .utf8)
        } catch {
            XCTFail("the settings file cannot be written: \(error)")
        }
        return path
    }

    /// Launch the app with a settings file of its own and harness attach.
    @discardableResult
    func launchApp(settings: [String] = [], arguments: [String] = []) -> XCUIApplication {
        let app = XCUIApplication()
        app.launchArguments += ["--settings-file", writeSettingsFile(settings),
                                "--attach-port", String(uiTestAttachPort)] + arguments
        #if os(macOS)
        // A window of one size, tall enough for the full bar (the card
        // buttons need a faceplate as wide as the bar): the window's saved
        // frame is the app's, shared with every earlier launch. The argument
        // domain overrides it for this launch only.
        app.launchArguments += ["-NSWindow Frame calculator", "100 100 1200 750 0 0 2560 1410 "]
        #endif
        app.launch()
        return app
    }

    /// The launched app's session through harness attach.
    func attachHarness(port: UInt16 = uiTestAttachPort) -> HarnessClient {
        let harness = HarnessClient()
        XCTAssertTrue(harness.connect(port: port), "no harness attach on 127.0.0.1:\(port)")
        return harness
    }

    /// The session is idle (the start and its startup preset are over), and
    /// the messages so far are read.
    func waitIdle(_ harness: HarnessClient, timeout: TimeInterval = 30) {
        // The bare `wait` is refused while an operation runs: asked again.
        let end = Date().addingTimeInterval(timeout)
        var reply: [String] = []
        repeat {
            reply = harness.command("wait", timeout: timeout)
            if reply.first?.hasPrefix("done") ?? false { break }
            Thread.sleep(forTimeInterval: 0.2)
        } while Date() < end
        XCTAssertTrue(reply.first?.hasPrefix("done") ?? false, "the session is not idle: \(reply)")
        harness.drain()
    }

    /// Waits for the end of an operation started with a tap, on the busy
    /// topic received as a stream: it says that one runs, then
    /// that none does; then the reply to the bare `wait` (until idle). The
    /// bare `wait` alone is refused while an operation runs ("Another
    /// operation is running."), so the stream comes first. A preset that
    /// ends paused (`pause:`, or `pause-on-start:` fired by what its last key
    /// starts) stays paused (docs/formats/preset.md §5.6); a paused machine is never idle,
    /// so the bare `wait` would wait until it is resumed (§6.2): for
    /// `endsPaused` the run-state topic is read until it says paused.
    ///
    /// For a preset (`preset`: its file name) the start is that preset's own
    /// end line in the log: a busy pair of another operation, such as one the
    /// closing file picker causes, would end the wait before the preset ran.
    func waitForOperation(_ harness: HarnessClient, timeout: TimeInterval, endsPaused: Bool = false,
                          preset: String? = nil) {
        if let preset {
            var ending = ""
            XCTAssertTrue(harness.waitForMessage(timeout: timeout + 15) {
                guard $0.hasPrefix("message log "), $0.contains("Preset "), $0.contains(preset),
                      $0.contains(" carried out to its end.") || $0.contains(" stopped: ") else { return false }
                ending = $0
                return true
            }, "the preset \(preset) did not run to its end")
            XCTAssertTrue(ending.contains(" carried out to its end."), "the preset \(preset) stopped: \(ending)")
            // The busy topic's end may come before the log's last lines.
            if !endsPaused {
                waitIdle(harness)
                return
            }
        } else {
            XCTAssertTrue(harness.waitForMessage(timeout: 15) { $0.hasPrefix("message busy ") && $0.contains("\"operationRuns\":true") },
                          "no operation started")
            XCTAssertTrue(harness.waitForMessage(timeout: timeout) { $0.hasPrefix("message busy ") && $0.contains("\"operationRuns\":false") },
                          "the operation did not end")
        }
        if endsPaused {
            let end = Date().addingTimeInterval(10)
            var state: [String] = []
            repeat {
                state = harness.command("state run-state", timeout: 5)
                if state.contains(where: { $0.contains("\"paused\":true") }) { break }
                Thread.sleep(forTimeInterval: 0.2)
            } while Date() < end
            XCTAssertTrue(state.contains(where: { $0.contains("\"paused\":true") }), "the preset did not end paused: \(state)")
            return
        }
        let reply = harness.command("wait", timeout: 30)
        XCTAssertTrue(reply.first?.hasPrefix("done") ?? false, "the session is not idle: \(reply)")
    }

    #if !os(macOS)
    /// Launch the app after forcing the simulator into a known orientation.
    ///
    /// Setting orientation before launch avoids an iPad simulator quirk where
    /// the simulator retains its previous session orientation, causing XCUITest
    /// to rotate several times before tests can run.
    @discardableResult
    func launchApp(orientation: UIDeviceOrientation, settings: [String] = [], arguments: [String] = []) -> XCUIApplication {
        XCUIDevice.shared.orientation = orientation
        return launchApp(settings: settings, arguments: arguments)
    }

    /// Open the document picker's folder that holds `targetFile`
    /// ("On My iPhone/iPad" → 1-Testfiles), unless the picker already shows it.
    func navigateToOnMyIPhone(_ app: XCUIApplication, targetFile: String) {
        let target = app.cells.containing(.staticText, identifier: targetFile).firstMatch
        // The picker's first opening on a fresh simulator takes many seconds.
        let search = app.searchFields.firstMatch
        let pickerUp = XCTNSPredicateExpectation(
            predicate: NSPredicate { _, _ in search.exists || target.exists }, object: nil)
        _ = XCTWaiter.wait(for: [pickerUp], timeout: 30)

        if target.waitForExistence(timeout: 2) { return }

        for label in ["Durchsuchen", "Browse"] {
            let tab = app.tabBars.buttons[label]
            if tab.waitForExistence(timeout: 2) {
                if !tab.isSelected { tab.tap() }
                break
            }
        }

        for label in ["Auf meinem iPhone", "Auf meinem iPad", "On My iPhone", "On My iPad"] {
            let item = app.cells.containing(.staticText, identifier: label).firstMatch
            if item.waitForExistence(timeout: 2) { item.tap(); break }
        }

        let folder = app.cells.containing(.staticText, identifier: "1-Testfiles").firstMatch
        if folder.waitForExistence(timeout: 2) { folder.tap() }
    }

    /// Load `file` through the Preset button and wait, through attach, for
    /// the load to end within `idleTimeout` (paused, for `endsPaused`).
    func loadPreset(_ app: XCUIApplication, file: String, idleTimeout: TimeInterval, harness: HarnessClient,
                    endsPaused: Bool = false) {
        let presetButton = app.buttons["btn-preset"]
        XCTAssertTrue(presetButton.waitForExistence(timeout: 5), "Preset button not found")
        presetButton.tap()

        navigateToOnMyIPhone(app, targetFile: file)
        let cell = app.cells.containing(.staticText, identifier: file).firstMatch
        XCTAssertTrue(cell.waitForExistence(timeout: 5), "\(file) not found in file picker")
        harness.drain()   // the busy messages before the tap
        cell.tap()
        waitForOperation(harness, timeout: idleTimeout, endsPaused: endsPaused, preset: file)
    }
    #endif

    /// Attach a labelled screenshot to the current test result (always kept).
    func attachScreenshot(_ app: XCUIApplication, name: String) {
        let attachment = XCTAttachment(screenshot: app.screenshot())
        attachment.name = name
        attachment.lifetime = .keepAlways
        add(attachment)
    }
}
