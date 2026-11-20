import SwiftUI

/// The two card dialogs (drawn natively):
/// Arm Card lists the saved cards of the save folder by their names (one
/// that cannot be chosen now is disabled), Arm Fresh Card asks a name and, when a
/// card of that name is saved, whether to replace it. Everything else is
/// the library's media part through the bridge.
struct CardPickerView: View {
    enum Mode { case arm, armFresh }

    let mode: Mode
    let cards: [TISavedCard]
    /// Arm Card: the chosen card.
    let onArm: (TISavedCard) -> Void
    /// Arm Fresh Card: the name and whether a replace was confirmed; the
    /// completion gets the question to ask, or nil when it was fitted.
    let onArmFresh: (String, Bool, @escaping (String?) -> Void) -> Void

    @State private var name = ""
    @State private var question: String?
    @Environment(\.dismiss) private var dismiss

    private var title: String { mode == .arm ? "Arm Card" : "Arm Fresh Card" }
    private var trimmed: String { name.trimmingCharacters(in: .whitespacesAndNewlines) }

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text(title).font(.headline)
                Spacer()
                Button("Cancel") { dismiss() }
                    .keyboardShortcut(.escape, modifiers: [])
                    .accessibilityIdentifier("card-cancel")
            }
            .padding()

            Divider()

            if mode == .armFresh {
                TextField("Card name", text: $name)
                    .accessibilityIdentifier("card-name")
                    .textFieldStyle(.roundedBorder)
                    .padding([.horizontal, .top])
            }

            if cards.isEmpty {
                Spacer()
                VStack(spacing: 8) {
                    Image(systemName: "doc.questionmark")
                        .font(.largeTitle)
                        .foregroundStyle(.secondary)
                    Text("No cards found")
                        .foregroundStyle(.secondary)
                }
                Spacer()
            } else {
                List(cards, id: \.entryID) { card in
                    Button {
                        if mode == .armFresh {
                            name = card.name
                        } else {
                            onArm(card)
                            dismiss()
                        }
                    } label: {
                        Label(card.name, systemImage: "doc")
                    }
                    .buttonStyle(.plain)
                    .disabled(mode == .arm && !card.canChoose)
                    .help(card.hint ?? "")
                }
            }

            Divider()

            HStack {
                Spacer()
                if mode == .armFresh {
                    Button("Arm") { armFresh(confirmed: false) }
                        .accessibilityIdentifier("card-arm")
                        .keyboardShortcut(.return, modifiers: [])
                        .disabled(trimmed.isEmpty)
                        .buttonStyle(.borderedProminent)
                }
            }
            .padding()
        }
        .frame(minWidth: 320, minHeight: 300)
        // The dialog is named; its buttons keep their own names.
        .accessibilityElement(children: .contain)
        .accessibilityLabel(title)
        .alert(question ?? "",
               isPresented: Binding(get: { question != nil }, set: { if !$0 { question = nil } })) {
            Button("Replace", role: .destructive) { armFresh(confirmed: true) }
            Button("Cancel", role: .cancel) {}
        }
    }

    private func armFresh(confirmed: Bool) {
        onArmFresh(trimmed, confirmed) { asked in
            if let asked {
                question = asked
            } else {
                dismiss()
            }
        }
    }
}
