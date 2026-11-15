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

    /// The emulated seconds `ticks` reports.
    private func seconds(_ harness: HarnessClient) -> Double {
        let r = harness.command("ticks")
        XCTAssertEqual(r.first, "done 2", "ticks: \(r)")
        guard r.count == 3, r[2].hasPrefix("seconds: ") else { return 0 }
        return Double(r[2].dropFirst("seconds: ".count)) ?? 0
    }

    /// Reads what came on the connection for a second (the messages go on
    /// coming; a reply left from the time the app was away is read with them).
    private func drain(_ harness: HarnessClient) {
        let end = Date().addingTimeInterval(1)
        while Date() < end, harness.line(timeout: end.timeIntervalSinceNow) != nil {}
    }

    #if os(iOS)
    /// S4 (AC-8, EC-3): Home sends suspend and the session thread stops
    /// calling; back in front it calls again. iOS may stop the process once
    /// the suspend is carried out (its background task ends then); a reply
    /// that comes while the app is away is the refusal of EC-3.
    func testBackgroundSuspendsAndContinues() throws {
        let (app, harness) = launchAttached()
        XCTAssertEqual(harness.command("wait 1s", timeout: 15).first, "done 2")
        let before = seconds(harness)
        let w0 = Date()

        XCUIDevice.shared.press(.home)
        // Until the scene has gone to the background a command is still done.
        var away: String?
        let until = Date().addingTimeInterval(5)
        while Date() < until {
            away = harness.command("ticks", timeout: 2).first
            if away != "done 2" { break }
            Thread.sleep(forTimeInterval: 0.1)
        }
        let note = "a command while in the background: \(away ?? "no reply (the process is stopped)")"
        add(XCTAttachment(string: note))
        print("[S4] \(note)")
        if let away { XCTAssertEqual(away, "refused: the app is suspended") }
        Thread.sleep(forTimeInterval: 3)

        app.activate()
        XCTAssertTrue(app.wait(for: .runningForeground, timeout: 10))
        drain(harness)
        let after = seconds(harness)
        let wall = Date().timeIntervalSince(w0)
        print(String(format: "[S4] away %.2f s of wall time, %.2f s of emulated time", wall, after - before))
        XCTAssertLessThan(after - before, wall - 2, "the session ran while the app was in the background")
        // Calls again: emulated time passes on the app's clock.
        let t0 = Date()
        XCTAssertEqual(harness.command("wait 500ms", timeout: 10).first, "done 2")
        XCTAssertLessThan(Date().timeIntervalSince(t0), 2)
        harness.send("quit")
        app.terminate()
    }

    /// S5 (AC-5): a program runs; a press on the display (full speed) keeps
    /// the session thread busy for whole calls while the main thread takes the
    /// touch and its end; then a tap on a key reaches the session (R/S stops
    /// the program, `lcd` shows its display). The main thread's frame gaps
    /// are measured by `FrameGapTests` (Calc-U-59Tests).
    func testTapsReachSessionAtFullSpeed() throws {
        let (app, harness) = launchAttached()
        XCTAssertEqual(harness.command("wait 1s", timeout: 15).first, "done 2")
        // Lbl A, 1, SUM 00, GTO A; started with A (lcd blank while it runs).
        for key in ["LRN", "2nd", "SBR", "A", "1", "SUM", "0", "0", "GTO", "A", "LRN", "CLR", "A"] { tap(harness, key) }
        let running = harness.command("lcd")
        XCTAssertTrue(running.count > 1 && running[1].trimmingCharacters(in: .whitespaces).isEmpty, "not running: \(running)")

        let display = app.descendants(matching: .any)["led-display"].firstMatch
        XCTAssertTrue(display.waitForExistence(timeout: 5))
        let s0 = seconds(harness), w0 = Date()
        display.press(forDuration: 3)
        let fullRate = (seconds(harness) - s0) / Date().timeIntervalSince(w0)
        let s1 = seconds(harness), w1 = Date()
        Thread.sleep(forTimeInterval: 1.5)
        let regularRate = (seconds(harness) - s1) / Date().timeIntervalSince(w1)
        let note = String(format: "rate against wall time: pressed %.2f, released %.2f", fullRate, regularRate)
        add(XCTAttachment(string: note))
        print("[S5] \(note)")
        XCTAssertGreaterThan(fullRate, 1.5, "the press did not reach the session as full speed")
        XCTAssertEqual(regularRate, 1.0, accuracy: 0.15, "the end of the press did not reach the session")

        app.buttons["btn-key-rs"].press(forDuration: 0.3)   // held as long as a finger
        XCTAssertEqual(harness.command("wait 500ms", timeout: 10).first, "done 2")
        let stopped = harness.command("lcd")
        XCTAssertTrue(stopped.count > 1 && stopped[1].contains("1"), "R/S did not stop the program: \(stopped)")
        harness.send("quit")
        app.terminate()
    }
    #endif
}
