import XCTest

/// In Swift: FREEZE, STEP and RESUME
/// of the CALCULATOR (`program`) and CPU (`cpu`) tabs follow the run-control
/// buttons' rule of the library (calcu::debugger::buttonUse), through
/// the bridge, over the rows of library/debugger/tests/button_cases.json.
final class RunControlTests: XCTestCase {

    private struct Case: Decodable {
        struct State: Decodable { let paused: Bool; let owner: String? }
        let level: String
        let state: State?
        let pause: Bool
        let step: Bool
        let resume: Bool
    }
    private struct Table: Decodable { let repeatMs: Int; let cases: [Case] }

    /// The table's rows as the library's file holds them. The
    /// test reads the file itself when the sandboxed test host can reach the
    /// source tree, and checks that this copy is still the same.
    private static let rows: [Case] = [
        Case(level: "cpu", state: nil, pause: false, step: false, resume: false),
        Case(level: "cpu", state: .init(paused: false, owner: nil), pause: true, step: false, resume: false),
        Case(level: "cpu", state: .init(paused: true, owner: "cpu"), pause: false, step: true, resume: true),
        Case(level: "cpu", state: .init(paused: true, owner: "program"), pause: false, step: false, resume: false),
        Case(level: "program", state: .init(paused: false, owner: nil), pause: true, step: false, resume: false),
        Case(level: "program", state: .init(paused: true, owner: "program"), pause: false, step: true, resume: true),
        Case(level: "program", state: .init(paused: true, owner: "cpu"), pause: false, step: false, resume: false),
    ]

    private func level(_ name: String?) -> TIRunLevel {
        switch name {
        case "cpu": .cpu
        case "program": .calculator
        default: .none
        }
    }

    private func check(_ rows: [Case]) {
        for c in rows {
            let use = TIAppSession.buttonUse(for: level(c.level), hasState: c.state != nil,
                                             paused: c.state?.paused ?? false, owner: level(c.state?.owner))
            XCTAssertEqual(use.pause, c.pause, "\(c.level) \(String(describing: c.state)) pause")
            XCTAssertEqual(use.step, c.step, "\(c.level) \(String(describing: c.state)) step")
            XCTAssertEqual(use.resume, c.resume, "\(c.level) \(String(describing: c.state)) resume")
        }
    }

    func testButtonsFollowTheRule() throws {
        check(Self.rows)
        // The library's table, where the host can read it.
        let file = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
            .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
            .appendingPathComponent("library/debugger/tests/button_cases.json")
        guard let data = try? Data(contentsOf: file) else { return }
        let table = try JSONDecoder().decode(Table.self, from: data)
        XCTAssertEqual(table.repeatMs, 250)   // HoldRepeatButton's interval
        XCTAssertEqual(table.cases.count, Self.rows.count)
        check(table.cases)
    }
}
