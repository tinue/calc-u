import Foundation
import XCTest

/// A second app session of the Swift frontend, made through the real bridge
/// (`TIAppSession` over `cu59::SessionThread`) in the app the tests are loaded
/// into: the ROM files of the app's bundle, a TI-59, a temporary save and
/// trace folder, a settings file of its own in that folder (nothing of the
/// user's is read or written). The test plan launches the host app with
/// `--settings-file`, so the host's own settings are a file of the run too.
final class TestSession {
    let session: TIAppSession
    let folder: URL
    private let delegate = Forward()

    /// `prepare`: files put into the save folder before the session starts.
    init(prepare: ((URL) throws -> Void)? = nil, file: StaticString = #filePath, line: UInt = #line) throws {
        folder = FileManager.default.temporaryDirectory
            .appendingPathComponent("Calc-U-59Tests-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        try prepare?(folder)
        var missing: NSString?
        let resources = Bundle.main.resourcePath ?? Bundle.main.bundlePath
        let roms = try XCTUnwrap(TIRomSet.lookUp(inFolder: resources, leavingOut: nil, missing: &missing),
                                 "no ROM set in the host app: \(String(describing: missing))", file: file, line: line)
        let start = TISessionStart()
        // A new settings file at its defaults: the library's start builds a
        // TI-59 with the Master Library module and no printer.
        start.settingsBase = folder.path
        start.settingsFile = folder.appendingPathComponent("Calc-U-59.ini").path
        start.saveFolder = folder.path
        start.examplesFolder = Bundle.main.url(forResource: "examples", withExtension: nil)?.path ?? ""
        session = TIAppSession(roms: roms, start: start, delegate: delegate)
        delegate.session = session
    }

    deinit { try? FileManager.default.removeItem(at: folder) }

    /// The notices the shell's queue held so far (each taken and closed in turn).
    var notices: [String] { delegate.notices }
    /// The batches' changes so far.
    var changes: TIChange { delegate.changes }

    /// The bridge holds its delegate weakly; it keeps the notices,
    /// the rest is not what these tests look at.
    private final class Forward: NSObject, TIAppSessionDelegate {
        weak var session: TIAppSession?
        var notices: [String] = []
        var changes: TIChange = []
        func showDebugPanel(_ panel: Int) {}
        func noticesChanged() {
            while let n = session?.shell.currentNotice {
                print("[TestSession] notice: \(n)")
                notices.append(n)
                session?.shell.closeNotice()
            }
        }
        func diagnostic(_ message: String) { print("[TestSession] log: \(message)") }
        func changed(_ changes: TIChange) { self.changes.formUnion(changes) }
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

    /// Turns the run loop until `cond` holds or `timeout` passes.
    @discardableResult
    static func until(_ timeout: TimeInterval = 10, _ cond: () -> Bool) -> Bool {
        let end = Date().addingTimeInterval(timeout)
        while !cond() && Date() < end { run(0.05) }
        return cond()
    }
}
