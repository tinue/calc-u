import XCTest

/// C59-5 S1 (AC-4, AC-6) through the real bridge: commands the Swift side
/// sends in an order reach the session in that order, and what the session
/// sends back arrives on the main thread in that order.
final class MessageOrderTests: XCTestCase {

    /// 500 key commands (the 1 key down and up, in turn): each one's reply
    /// comes on the main thread, in the order they were sent.
    func testMessagesArriveOnMainInOrder() throws {
        let t = try TestSession()
        let code = TISharedTables.matrixCode(row: 7, col: 1)   // the 1 key
        var order: [Int] = []
        var offMain = 0
        var refused = 0
        let all = expectation(description: "500 replies")
        all.expectedFulfillmentCount = 500
        for i in 0..<500 {
            let done: (Bool) -> Void = { ok in
                if !Thread.isMainThread { offMain += 1 }
                if !ok { refused += 1 }
                order.append(i)
                all.fulfill()
            }
            if i % 2 == 0 { t.session.rawKeyDown(code, completion: done) } else { t.session.rawKeyUp(code, completion: done) }
        }
        wait(for: [all], timeout: 30)
        XCTAssertEqual(order, Array(0..<500), "replies out of order")
        XCTAssertEqual(offMain, 0, "a reply came off the main thread")
        XCTAssertEqual(refused, 0)
    }

    /// AC-4: the session thread calls the session about 60 times a second;
    /// the main thread runs no host call.
    func testSessionThreadCallsAbout60TimesASecond() throws {
        let t = try TestSession()
        TestSession.run(0.5)   // the thread has built the session
        let c0 = t.session.hostCallCount
        let t0 = Date()
        TestSession.run(2)
        let rate = Double(t.session.hostCallCount - c0) / Date().timeIntervalSince(t0)
        XCTAssertGreaterThan(rate, 50, "\(rate) calls a second")
        XCTAssertLessThan(rate, 70, "\(rate) calls a second")
    }
}
