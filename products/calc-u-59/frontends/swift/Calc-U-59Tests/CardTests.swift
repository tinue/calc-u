import XCTest

/// C59-7 AC-26, LIB-7 AC-33 on the Swift side of the bridge (M-55): the cards
/// are the library's media of the save folder the app passes (its iCloud
/// Documents folder, here a temporary one), read and written through the
/// bridge's coordinated file access (C59-5 D-6, LIB-7 D-30). Arm Card lists
/// the saved cards by the names inside them; Arm Fresh Card asks before it
/// replaces one.
final class CardTests: XCTestCase {

    private static let savedCard = """
        format: ti59-magnetic-card
        format-version: 3
        card-name: "Saved Program"
        sides: {}

        """

    private func waitFor(_ what: String, _ cond: () -> Bool) {
        for _ in 0..<50 where !cond() { TestSession.run(0.1) }
        XCTAssertTrue(cond(), what)
    }

    func test_c59_7_ac26_swift_cards_in_the_icloud_save_folder() throws {
        let t = try TestSession(prepare: { folder in
            try Self.savedCard.write(to: folder.appendingPathComponent("saved.magnetcard.yaml"), atomically: true, encoding: .utf8)
            // 2.3.0's card files are no cards (C59-7 AC-4): not listed.
            try "Calc-U-59 Card 2.0\nEND-CARD\n".write(to: folder.appendingPathComponent("old.u59"), atomically: true, encoding: .utf8)
        })
        waitFor("the saved card is listed by its name") { t.session.savedCards.map(\.name) == ["Saved Program"] }

        // Arm Fresh Card under a saved card's name: the question first, nothing written.
        var asked: String?
        let first = expectation(description: "asked")
        t.session.armFreshCard("Saved Program", confirmed: false) { question, _ in asked = question; first.fulfill() }
        wait(for: [first], timeout: 5)
        XCTAssertTrue(asked?.hasPrefix("Replace card") == true, "no replace question: \(String(describing: asked))")
        XCTAssertFalse(t.session.cardFitted)

        // A new name: fitted at once, its file written at the first write only.
        let fresh = expectation(description: "fitted")
        t.session.armFreshCard("Fresh", confirmed: false) { question, refusal in
            XCTAssertNil(question)
            XCTAssertNil(refusal)
            fresh.fulfill()
        }
        wait(for: [fresh], timeout: 5)
        waitFor("the fresh card is fitted") { t.session.fittedCardName == "Fresh" }
        XCTAssertFalse(FileManager.default.fileExists(atPath: t.folder.appendingPathComponent("Fresh.magnetcard.yaml").path))

        // Arm Card: the saved card in its place.
        let saved = try XCTUnwrap(t.session.savedCards.first)
        t.session.armCard(saved)
        waitFor("the saved card is fitted") { t.session.fittedCardName == "Saved Program" }
        XCTAssertFalse(t.session.cardPassing)
    }
}
