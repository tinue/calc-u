import XCTest

// MARK: - App launch helpers
//
// C59-8 AC-21 (design §3.9): the app has no aid for its tests. A test hands
// the app a settings file of its own with `--settings-file` (Debug builds
// only), so it neither reads nor changes the simulator's settings, and
// reaches the session through harness attach (`--attach-port`): the module
// is set with `attach slot-1 <id>`, a key into a running program is
// `key-down`/`key-up`, and the end of a load started with a tap is the reply
// to the bare `wait`, after the busy topic said an operation runs.

/// The port of a launch's harness attach: one per simulator (first step 4).
let uiTestAttachPort: UInt16 = 59162

extension XCTestCase {

    /// A settings file for one launch, with these values ("section.key=value"),
    /// at a path the app under test reads (first check of M-58: the runner
    /// writes it, the simulator's app reads it by path). On macOS the app is
    /// sandboxed: the file goes into its container's temporary folder, and
    /// the app is given its name, which it takes in that folder.
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
        let name = "uitest-\(UUID().uuidString).ini"
        #if os(macOS)
        let folder = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Containers/ch.erzberger.calcu59/Data/tmp", isDirectory: true)
        try? FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        try? text.write(to: folder.appendingPathComponent(name), atomically: true, encoding: .utf8)
        return name
        #else
        let path = (NSTemporaryDirectory() as NSString).appendingPathComponent(name)
        try? text.write(toFile: path, atomically: true, encoding: .utf8)
        return path
        #endif
    }

    /// Launch the app with a settings file of its own and harness attach.
    @discardableResult
    func launchApp(settings: [String] = [], arguments: [String] = []) -> XCUIApplication {
        let app = XCUIApplication()
        app.launchArguments += ["--settings-file", writeSettingsFile(settings),
                                "--attach-port", String(uiTestAttachPort)] + arguments
        app.launch()
        return app
    }

    /// The launched app's session through harness attach (C59-5 S3).
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
    /// topic received as a stream (C59-8 AC-21): it says that one runs, then
    /// that none does; then the reply to the bare `wait` (until idle). The
    /// bare `wait` alone is refused while an operation runs ("Another
    /// operation is running."), so the stream comes first.
    func waitForOperation(_ harness: HarnessClient, timeout: TimeInterval) {
        XCTAssertTrue(harness.waitForMessage(timeout: 15) { $0.hasPrefix("message busy ") && $0.contains("\"operationRuns\":true") },
                      "no operation started")
        XCTAssertTrue(harness.waitForMessage(timeout: timeout) { $0.hasPrefix("message busy ") && $0.contains("\"operationRuns\":false") },
                      "the operation did not end")
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
    /// the load to end within `idleTimeout`.
    func loadPreset(_ app: XCUIApplication, file: String, idleTimeout: TimeInterval, harness: HarnessClient) {
        let presetButton = app.buttons["btn-preset"]
        XCTAssertTrue(presetButton.waitForExistence(timeout: 5), "Preset button not found")
        presetButton.tap()

        navigateToOnMyIPhone(app, targetFile: file)
        let cell = app.cells.containing(.staticText, identifier: file).firstMatch
        XCTAssertTrue(cell.waitForExistence(timeout: 5), "\(file) not found in file picker")
        harness.drain()   // the busy messages before the tap
        cell.tap()
        waitForOperation(harness, timeout: idleTimeout)
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
