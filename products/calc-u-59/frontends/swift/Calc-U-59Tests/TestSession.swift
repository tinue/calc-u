import Foundation
import XCTest

/// A second app session of the Swift frontend, made through the real bridge
/// (`TIAppSession` over `cu59::SessionThread`) in the app the tests are loaded
/// into: the ROM files of the app's bundle, a TI-59, a temporary save and
/// trace folder, the settings' defaults in memory (nothing of the user's is read or written). The test plan
/// launches the host app with `--UITesting`, so its own settings stay in memory.
final class TestSession {
    let session: TIAppSession
    let folder: URL
    private let delegate = Forward()

    init(file: StaticString = #filePath, line: UInt = #line) throws {
        folder = FileManager.default.temporaryDirectory
            .appendingPathComponent("Calc-U-59Tests-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        var missing: NSString?
        let resources = Bundle.main.resourcePath ?? Bundle.main.bundlePath
        let roms = try XCTUnwrap(TIRomSet.lookUp(inFolder: resources, leavingOut: nil, missing: &missing),
                                 "no ROM set in the host app: \(String(describing: missing))", file: file, line: line)
        let start = TISessionStart()
        start.cuecards = ""
        // The defaults in memory: LIB-18's start builds a TI-59 with the
        // Master Library module and no printer (LIB-18 AC-17, AC-18).
        start.settings = TISettings(defaultsOnlyWithLaunchSettings: [])
        start.saveFolder = folder.path
        start.traceDirectory = folder.path
        start.traceMaxBytes = 1_000_000
        start.traceAvailable = false
        session = TIAppSession(roms: roms, start: start, delegate: delegate)
    }

    deinit { try? FileManager.default.removeItem(at: folder) }

    /// The notices the bridge showed so far, as "<title>: <message>".
    var notices: [String] { delegate.notices }

    /// The bridge holds its delegate weakly; it keeps the notices (LIB-13
    /// AC-15), the rest is not what these tests look at.
    private final class Forward: NSObject, TIAppSessionDelegate {
        var notices: [String] = []
        func showDebugPanel(_ panel: Int) {}
        func showErrorTitle(_ title: String, message: String) {
            print("[TestSession] notice: \(title): \(message)")
            notices.append("\(title): \(message)")
        }
        func diagnostic(_ message: String) { print("[TestSession] log: \(message)") }
        func changed(_ changes: TIChange) {}
        func cardWritten() {}
    }

    /// The session's emulated time in ticks, waited for on the main thread.
    func ticks(_ test: XCTestCase) -> UInt64 { time(test).ticks }

    /// The session's emulated time, waited for on the main thread.
    func time(_ test: XCTestCase) -> (ticks: UInt64, seconds: Double) {
        let e = test.expectation(description: "emulated time")
        var value: (UInt64, Double) = (0, 0)
        session.emulatedTime { value = ($0, $1); e.fulfill() }
        test.wait(for: [e], timeout: 5)
        return value
    }

    /// Lets the main run loop turn for `seconds` (the session's messages arrive).
    static func run(_ seconds: TimeInterval) {
        RunLoop.main.run(until: Date().addingTimeInterval(seconds))
    }
}
