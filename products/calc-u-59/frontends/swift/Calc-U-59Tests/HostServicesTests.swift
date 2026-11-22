import XCTest

/// The host services the Swift frontend states to its
/// session, as the session sees them offered.
final class HostServicesTests: XCTestCase {

    private func offered() throws -> [String] {
        let t = try TestSession()
        let e = expectation(description: "offered")
        var names: [String] = []
        t.session.offeredHostServices { names = $0; e.fulfill() }
        wait(for: [e], timeout: 5)
        return names
    }

    func testStatedServices() throws {
        let expected = ["clipboard-text", "clipboard-image", "file-picker", "keep-file", "open-url", "haptics"]
        XCTAssertEqual(Set(TIAppSession.statedHostServices), Set(expected))
        XCTAssertEqual(Set(try offered()), Set(expected))
    }


    /// No audio sink on either: the key click is the frontend's own.
    func testNoAudioSink() throws {
        XCTAssertFalse(TIAppSession.statedHostServices.contains("audio-sink"))
        XCTAssertFalse(try offered().contains("audio-sink"))
    }
}
