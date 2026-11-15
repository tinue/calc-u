import XCTest

/// Harness attach in the simulator (C59-5 design §6, S3 and S4): the app's
/// Debug build opens its session on 127.0.0.1:<port>; the test, which runs in
/// the same simulator, attaches as `calcu attach` would. Part of
/// UIRegressionTests. A fixed port of its own, so it does not meet an app
/// started by hand on 59059 (a parallel run on two simulators needs two).
final class AttachTests: XCTestCase {
    static let port: UInt16 = 59160

    override func setUp() {
        continueAfterFailure = false
    }

    private func launchAttached() -> (XCUIApplication, HarnessClient) {
        let app = XCUIApplication()
        app.launchArguments += ["--UITesting", "--attach-port", String(Self.port)]
        app.launch()
        let harness = HarnessClient()
        XCTAssertTrue(harness.connect(port: Self.port), "no harness attach on 127.0.0.1:\(Self.port)")
        return (app, harness)
    }

    private func tap(_ harness: HarnessClient, _ key: String) {
        XCTAssertEqual(harness.command("key-down \(key)").first, "done")
        XCTAssertEqual(harness.command("wait 150ms").first, "done 2")
        XCTAssertEqual(harness.command("key-up \(key)").first, "done")
        XCTAssertEqual(harness.command("wait 150ms").first, "done 2")
    }

    /// S3 (AC-10, AC-11, AC-12, AC-14): the harness drives the running app's
    /// session; what it changes shows in the window; `wait` passes on the app's
    /// clock; `quit` ends the attach, not the app.
    func testHarnessDrivesTheApp() throws {
        let (app, harness) = launchAttached()
        // The state on attach: every state topic once, before the first reply.
        harness.send("ticks")
        var messages: [String] = []
        var reply: String?
        while let l = harness.line() {
            if l.hasPrefix("message ") { messages.append(l); continue }
            reply = l
            break
        }
        XCTAssertEqual(reply, "done 2")
        XCTAssertTrue(messages.contains { $0.hasPrefix("message busy ") }, "no busy topic on attach")
        XCTAssertTrue(messages.contains { $0.hasPrefix("message c59.model ") }, "no c59.model topic on attach")
        _ = harness.line(); _ = harness.line()   // ticks:, seconds:

        XCTAssertEqual(harness.command("wait 1s", timeout: 15).first, "done 2")   // the ROM is idle
        for key in ["1", "2", "3"] { tap(harness, key) }
        let lcd = harness.command("lcd")
        XCTAssertEqual(lcd.first, "done 2")
        XCTAssertTrue(lcd.count > 1 && lcd[1].contains("123"), "lcd: \(lcd)")

        // The window shows what the harness typed.
        let shot = XCTAttachment(screenshot: app.screenshot())
        shot.name = "attached harness typed 123"
        shot.lifetime = .keepAlways
        add(shot)

        // AC-14: emulated time passes as the app paces it.
        let t0 = Date()
        let wait = harness.command("wait 1s", timeout: 15)
        let wall = Date().timeIntervalSince(t0)
        XCTAssertEqual(wait.first, "done 2")
        XCTAssertGreaterThanOrEqual(wall, 0.95)
        XCTAssertLessThan(wall, 1.5)

        harness.send("quit")
        XCTAssertTrue(harness.closedByPeer(), "quit did not end the attach")
        XCTAssertEqual(app.state, .runningForeground)   // the app goes on

        // The next harness attaches to the same app.
        let again = HarnessClient()
        XCTAssertTrue(again.connect(port: Self.port, timeout: 5))
        XCTAssertEqual(again.command("ticks").first, "done 2")
        again.send("quit")
        app.terminate()
    }
}
