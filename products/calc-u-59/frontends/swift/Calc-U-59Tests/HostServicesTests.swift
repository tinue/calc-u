import XCTest

/// C59-5 S2 (AC-7): the host services the Swift frontend states to its
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

    #if os(iOS)
    func testStatedServicesIOS() throws {
        let expected = ["clipboard-text", "clipboard-image", "file-picker", "keep-file", "open-url", "haptics"]
        XCTAssertEqual(Set(TIAppSession.statedHostServices), Set(expected))
        XCTAssertEqual(Set(try offered()), Set(expected))
    }
    #endif

    #if os(macOS)
    func testStatedServicesMacOS() throws {
        let expected = ["clipboard-text", "clipboard-image", "file-picker", "folder-picker", "keep-file", "open-url"]
        XCTAssertEqual(Set(TIAppSession.statedHostServices), Set(expected))
        XCTAssertEqual(Set(try offered()), Set(expected))
    }
    #endif

    /// No audio sink on either: the key click is the frontend's own (LIB-3 AC-33).
    func testNoAudioSink() throws {
        XCTAssertFalse(TIAppSession.statedHostServices.contains("audio-sink"))
        XCTAssertFalse(try offered().contains("audio-sink"))
    }
}
