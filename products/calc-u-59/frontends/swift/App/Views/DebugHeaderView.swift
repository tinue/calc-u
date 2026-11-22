import SwiftUI

/// The header of a debug tab (CALCULATOR or CPU) as app/'s debugHeader
/// describes it for every GUI: the title, then RESUME, FREEZE, FREEZE ON
/// START, ARMED and STEP with their labels and colours, at the panel's base
/// font size; short forms below debugHeaderNarrow. The tab gives each
/// button's action and whether it can be used now.
struct DebugHeaderView: View {
    struct Use {
        var enabled: Bool
        var action: () -> Void
    }
    let cpu: Bool
    let baseFontSize: CGFloat
    /// By app/'s button ID: resume, freeze, freeze-on-start, armed, step.
    let buttons: [String: Use]

    var body: some View {
        let narrow = TIViewRules.debugHeaderNarrow(baseFontSize)
        HStack(spacing: 8) {
            Text(TIViewRules.debugHeaderTitle(cpu, narrow: narrow))
                .font(.caption.bold())
                .foregroundStyle(.white.opacity(0.6))
            Spacer()
            ForEach(TIViewRules.debugHeaderButtons(cpu), id: \.buttonID) { b in
                let use = buttons[b.buttonID] ?? Use(enabled: false, action: {})
                let colour = Color(rgb: b.colour)
                let id = (cpu ? "btn-cpu-" : "btn-") + b.buttonID
                if b.buttonID == "step" {
                    // Tap to single-step; hold to auto-step.
                    HoldRepeatButton(title: narrow ? b.narrowLabel : b.label, color: colour,
                                     baseFontSize: baseFontSize, enabled: use.enabled, action: use.action)
                        .accessibilityIdentifier(id)
                } else {
                    Button(narrow ? b.narrowLabel : b.label, action: use.action)
                        .font(.system(size: baseFontSize, weight: .bold, design: .monospaced))
                        .foregroundStyle(colour)
                        .opacity(use.enabled ? 1 : 0.4)
                        .disabled(!use.enabled)
                        .accessibilityIdentifier(id == "btn-armed" ? "btn-freeze-armed" : id)
                }
            }
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 6)
        .background(Chrome.debugHeader)
    }
}
