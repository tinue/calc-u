import XCTest
#if os(iOS)
import UIKit
#endif

/// C59-5 AC-5 (design S5, D-9): while the session thread is busy for whole
/// calls (full speed, a program running), the main thread keeps its frames,
/// and a key command sent then is answered when the call is over. The probe
/// is in the test bundle, not the app (D-9: no probe in the app): a display
/// link on iOS, a 60 Hz timer on macOS, on the host app's main thread, as
/// first step 3 measured (FS 3). The numbers go into qa-report.md.
final class FrameGapTests: XCTestCase {

    /// An endless loop, started with the A key (the spike's busy-loop.ti59).
    private static let busyLoop = """
        MODEL: TI-59
        PROGRAM:
        000 76 LBL
        001 11 A
        002 01 1
        003 44 SUM
        004 00 00
        005 61 GTO
        006 11 A
        """

    private final class Probe: NSObject {
        private(set) var gaps: [Double] = []
        private var last: CFTimeInterval = 0
        #if os(iOS)
        private var link: CADisplayLink?
        func start() {
            link = CADisplayLink(target: self, selector: #selector(tick(_:)))
            link?.add(to: .main, forMode: .common)
        }
        @objc private func tick(_ l: CADisplayLink) { record(l.timestamp) }
        func stop() { link?.invalidate() }
        #else
        private var timer: Timer?
        func start() {
            let t = Timer(timeInterval: 1.0 / 60, repeats: true) { [weak self] _ in self?.record(CACurrentMediaTime()) }
            RunLoop.main.add(t, forMode: .common)
            timer = t
        }
        func stop() { timer?.invalidate() }
        #endif
        private func record(_ t: CFTimeInterval) {
            if last > 0 { gaps.append((t - last) * 1000) }
            last = t
        }
        func quantile(_ q: Double) -> Double {
            let g = gaps.sorted()
            return g.isEmpty ? 0 : g[min(g.count - 1, Int(Double(g.count) * q))]
        }
    }

    private func measure(_ name: String, seconds: TimeInterval) -> Probe {
        let p = Probe()
        p.start()
        TestSession.run(seconds)
        p.stop()
        let line = String(format: "%@: %d frames, gap p50 %.1f ms, p95 %.1f ms, max %.1f ms, over 25 ms: %d",
                          name, p.gaps.count, p.quantile(0.5), p.quantile(0.95), p.gaps.max() ?? 0,
                          p.gaps.filter { $0 > 25 }.count)
        print("[FrameGap] \(line)")
        add(XCTAttachment(string: line))
        return p
    }

    func testMainThreadKeepsFramesWhileSessionRunsAtFullSpeed() throws {
        let t = try TestSession()
        TestSession.run(1)
        let idle = measure("idle", seconds: 2)

        t.session.loadStateFile(Data(Self.busyLoop.utf8))
        TestSession.run(1)
        let a = TISharedTables.matrixCode(row: 0, col: 0)   // A: starts the loop
        t.session.pressKey(a)
        TestSession.run(0.15)
        t.session.releaseKey(a)
        TestSession.run(0.5)
        let w0 = Date(), e0 = t.time(self).seconds
        let regular = measure("program running, regular speed", seconds: 2)
        let regularRate = (t.time(self).seconds - e0) / Date().timeIntervalSince(w0)

        t.session.fullSpeed = true
        TestSession.run(0.2)
        let w1 = Date(), e1 = t.time(self).seconds
        let full = measure("program running, full speed", seconds: 3)

        // A key command sent while the calls are busy: its reply comes when
        // the call it arrived in is over.
        var latencies: [Double] = []
        let code = TISharedTables.matrixCode(row: 4, col: 0)   // BST: harmless while running
        for i in 0..<20 {
            let e = expectation(description: "reply \(i)")
            let sent = Date()
            let done: (Bool) -> Void = { _ in latencies.append(Date().timeIntervalSince(sent) * 1000); e.fulfill() }
            if i % 2 == 0 { t.session.pressKey(code, completion: done) } else { t.session.releaseKey(code, completion: done) }
            wait(for: [e], timeout: 5)
        }
        let fullRate = (t.time(self).seconds - e1) / Date().timeIntervalSince(w1)
        t.session.fullSpeed = false
        let summary = String(format: "rate against wall time: regular %.2f, full speed %.2f; key reply latency max %.1f ms",
                             regularRate, fullRate, latencies.max() ?? 0)
        print("[FrameGap] \(summary)")
        add(XCTAttachment(string: summary))

        XCTAssertGreaterThan(fullRate, 1.5, "full speed did not keep the session thread busy")
        for p in [idle, regular, full] {
            XCTAssertLessThan(p.quantile(0.95), 25, "main thread p95 frame gap")
            XCTAssertLessThan(p.gaps.max() ?? 0, 100, "main thread frame gap")
        }
        XCTAssertLessThan(latencies.max() ?? 0, 100, "a key command waited more than a few calls")
    }
}
