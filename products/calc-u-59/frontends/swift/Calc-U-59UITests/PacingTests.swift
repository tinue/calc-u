import XCTest

/// Pacing in the Swift frontend: a TI-59 runs at regular speed for one
/// minute of wall time and its emulated time grows by one minute, within the
/// pacing bound (below). Emulated time is read over harness attach
/// (`ticks`), wall time is the test's own clock. Checkpoint: in the test
/// plan `PacingCheckpoint`, not in `UIRegressionTests`. On the iOS
/// simulator; the Mac app is checked by `tests/mac_attach_check.sh`, which
/// needs no XCUITest runner on macOS.
final class PacingTests: XCTestCase {
    static let port: UInt16 = 59161

    /// The TI-59's slice (20 ms) is the pacing bound at each end;
    /// `ticks` is answered between two host calls (1/60 s); each reading's
    /// wall time is the middle of its round trip, so half of each round trip
    /// is added too.
    private static let slice = 0.020
    private static let callPeriod = 1.0 / 60

    private struct Reading { let emulated: Double; let wall: Date; let roundTrip: Double }

    private func read(_ harness: HarnessClient) -> Reading {
        let sent = Date()
        let r = harness.command("ticks")
        let back = Date()
        XCTAssertEqual(r.first, "done 2", "ticks: \(r)")
        let seconds = r.count == 3 && r[2].hasPrefix("seconds: ") ? Double(r[2].dropFirst("seconds: ".count)) ?? 0 : 0
        let rtt = back.timeIntervalSince(sent)
        return Reading(emulated: seconds, wall: sent.addingTimeInterval(rtt / 2), roundTrip: rtt)
    }

    func testTI59KeepsOneMinute() throws {
        let app = XCUIApplication()
        app.launchArguments += ["--settings-file", writeSettingsFile([]), "--attach-port", String(Self.port)]
        app.launch()
        let harness = HarnessClient()
        XCTAssertTrue(harness.connect(port: Self.port), "no harness attach on 127.0.0.1:\(Self.port)")
        XCTAssertEqual(harness.command("wait 1s", timeout: 15).first, "done 2")   // the ROM is idle

        let a = read(harness)
        Thread.sleep(forTimeInterval: 60)
        let b = read(harness)
        let emulated = b.emulated - a.emulated
        let wall = b.wall.timeIntervalSince(a.wall)
        let bound = Self.slice + Self.callPeriod + (a.roundTrip + b.roundTrip) / 2
        let note = String(format: "wall %.4f s, emulated %.4f s, difference %+.4f s, bound %.4f s",
                          wall, emulated, emulated - wall, bound)
        print("[pacing] \(note)")
        add(XCTAttachment(string: note))
        XCTAssertLessThanOrEqual(abs(emulated - wall), bound, note)
        harness.send("quit")
        app.terminate()
    }
}
