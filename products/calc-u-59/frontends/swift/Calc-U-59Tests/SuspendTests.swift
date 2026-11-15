import XCTest

/// C59-5 AC-8 on the Swift side of the bridge: suspend is carried out, then
/// the session thread stops calling; continue makes it call again. (The app's
/// own scene phases are S4, `AttachTests.testBackgroundSuspendsAndContinues`.)
final class SuspendTests: XCTestCase {

    func testSuspendStopsCallsContinueCallsAgain() throws {
        let t = try TestSession()
        TestSession.run(0.5)
        let suspended = expectation(description: "suspend carried out")
        t.session.suspend { suspended.fulfill() }   // on the main thread
        wait(for: [suspended], timeout: 5)

        let calls = t.session.hostCallCount
        let ticks = t.ticks(self)
        TestSession.run(0.5)
        XCTAssertEqual(t.session.hostCallCount, calls, "the thread called while suspended")
        XCTAssertEqual(t.ticks(self), ticks, "emulated time passed while suspended")

        t.session.continueSession()
        TestSession.run(0.5)
        XCTAssertGreaterThan(t.session.hostCallCount, calls + 20, "no calls after continue")
        XCTAssertGreaterThan(t.ticks(self), ticks, "emulated time stands after continue")
    }
}
