import XCTest

/// LIB-13 AC-15 (design §10.6, E13-6): a file the session refuses after the
/// Swift frontend handed it over (a drop, the Preset button's dialog) is shown
/// as the session's notice, which names the file.
final class OpeningTests: XCTestCase {

    func testLib13AC15RefusalIsANotice() throws {
        let t = try TestSession()
        _ = t.ticks(self)   // the session runs
        let e = expectation(description: "open replied")
        var opened = true
        t.session.openFile(Data("just some text\n".utf8), path: nil, fileName: "readme.txt", way: "drop") {
            opened = $0
            e.fulfill()
        }
        wait(for: [e], timeout: 10)
        // Refused at once, or kept while the start still runs (LIB-13 AC-7)
        // and refused when it ends: either way the refusal is the notice.
        _ = opened
        let deadline = Date().addingTimeInterval(5)
        while !t.notices.contains(where: { $0.contains("readme.txt") }) && Date() < deadline { TestSession.run(0.05) }
        XCTAssertTrue(t.notices.contains(where: { $0.contains("readme.txt") }), "notices: \(t.notices)")
    }

    /// The session says whether it would take a name (LIB-13 AC-6).
    func testLib13AC6WouldTakeByName() throws {
        let t = try TestSession()
        _ = t.ticks(self)
        for (name, takes) in [("p.ti59", true), ("c.asm", true), ("two.keystrokes.yaml", true), ("readme.txt", false)] {
            let e = expectation(description: name)
            var got = !takes
            t.session.wouldOpenFile(name) { got = $0; e.fulfill() }
            wait(for: [e], timeout: 5)
            XCTAssertEqual(got, takes, name)
        }
    }
}
