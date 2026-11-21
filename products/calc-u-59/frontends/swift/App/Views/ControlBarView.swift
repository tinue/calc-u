import SwiftUI

/// The control bar below the faceplate, drawn from the library's bar model
/// (`TIShell.bar`) over the control state: the model picker first, the
/// picker of `slot-1` (the library's module picker), the button "PC-100C",
/// the card controls on the TI-59; on iOS and iPadOS, where no menu bar
/// can be counted on, Reset, Preset, Settings and More. No control here
/// decides what the machine offers: a control is enabled exactly when its
/// entry is available now; on macOS the reason is its help tag.
struct ControlBarView: View {
    @Environment(EmulatorViewModel.self) var viewModel
    /// Labels beside the icons; off where the bar is too narrow.
    var showLabels: Bool = true

    /// More ▸ Cold boot asks before it is carried out.
    @State private var confirming: TIMenuItem?

    var body: some View {
        HStack(spacing: 12) {
            ForEach(Array(viewModel.barControls.enumerated()), id: \.offset) { index, control in
                if index > 0, control.group != viewModel.barControls[index - 1].group,
                   control.group == .end || control.group == .media {
                    Spacer(minLength: 0)
                }
                view(for: control)
            }
        }
        .buttonStyle(ChromeButtonStyle())
        .padding(.horizontal, 12)
        // Leading: without the TI-59's card controls the rest stays put.
        .frame(maxWidth: .infinity, alignment: .leading)
        .frame(height: 50)
        .background(Color(white: 0.15))
        .foregroundStyle(.white)
        .accessibilityElement(children: .contain)
        .accessibilityIdentifier("control-bar")
        .confirmationDialog(confirming?.confirm ?? "", isPresented: .init(
            get: { confirming != nil },
            set: { if !$0 { confirming = nil } }
        ), titleVisibility: .visible) {
            if let item = confirming {
                Button(item.label, role: .destructive) { viewModel.choose(item) }
                    .accessibilityIdentifier("confirm-\(item.itemID)")
                Button("Cancel", role: .cancel) {}
            }
        }
    }

    @ViewBuilder
    private func view(for control: TIBarControl) -> some View {
        switch control.kind {
        case .modelPicker:
            picker(control, label: viewModel.modelID, id: "bar-model")
        case .slotPicker:
            picker(control, label: Self.shortLabel(control), id: "bar-slot-\(control.controlID)")
        case .placePicker:
            picker(control, label: Self.shortLabel(control), id: "bar-place-\(control.controlID)")
        case .placeButton:
            placeButton(control)
        case .media:
            CardControls(showLabels: showLabels)
        case .action:
            if let item = control.item { action(item) }
        case .more:
            moreMenu(control)
        @unknown default:
            EmptyView()
        }
    }

    // MARK: - Pickers (the model, slot-1)

    private func picker(_ control: TIBarControl, label: String, id: String) -> some View {
        Menu {
            ForEach(Array(control.choices.enumerated()), id: \.offset) { _, item in
                Button {
                    viewModel.choose(item)
                } label: {
                    if item.checked { Label(item.label, systemImage: "checkmark") } else { Text(item.label) }
                }
                .disabled(!item.enabled)
                .accessibilityIdentifier("item-\(item.itemID)")
            }
        } label: {
            Text(label.isEmpty ? control.label : label)
                .font(.callout.weight(.semibold))
                .fixedSize()
        }
        .menuStyle(ChromeMenuStyle())
        .fixedSize()
        .accessibilityIdentifier(id)
    }

    /// What a picker shows on the bar: the chosen entry's ID (ML, LE, …), "—" for none.
    private static func shortLabel(_ control: TIBarControl) -> String {
        guard let chosen = control.choices.first(where: { $0.checked }) else { return "—" }
        let prefix = "peripheral-\(control.controlID)-"
        let id = chosen.itemID.hasPrefix(prefix) ? String(chosen.itemID.dropFirst(prefix.count)) : chosen.label
        return id == "none" ? "—" : id
    }

    // MARK: - The place button (the PC-100C)

    private func placeButton(_ control: TIBarControl) -> some View {
        let item = control.choices.first
        return Button {
            if let item { viewModel.choose(item) }
        } label: {
            HStack(spacing: 6) {
                if showLabels { Text(control.label) } else { Image(systemName: "printer") }
                Circle()
                    .fill(control.plugged ? Color.red : Color.gray.opacity(0.4))
                    .frame(width: 8, height: 8)
            }
            .font(.system(size: 11, weight: .bold))
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(Color(white: 0.25))
            .clipShape(RoundedRectangle(cornerRadius: 6))
        }
        .disabled(!(item?.enabled ?? false))
        .reasonTag(item?.reason ?? "")
        .accessibilityLabel(control.label)
        .accessibilityValue(control.plugged ? "On" : "Off")
        .accessibilityIdentifier("bar-place-\(control.controlID)")
    }

    // MARK: - Actions (iOS: Reset, Preset, Settings)

    @ViewBuilder
    private func action(_ item: TIMenuItem) -> some View {
        switch item.itemID {
        case "reset":
            Button("Reset", systemImage: "arrow.counterclockwise") {
                viewModel.feedbackForReset()
                viewModel.choose(item)
            }
            .foregroundStyle(.orange)
            .labelStyle(showLabel: showLabels)
            .disabled(!item.enabled)
            .accessibilityIdentifier("btn-reset")
        case TIShellActionLoadPreset:
            Button("Preset", systemImage: "doc.badge.arrow.up") { viewModel.choose(item) }
                .labelStyle(showLabel: showLabels)
                .controlSize(.large)
                .disabled(!item.enabled)
                .accessibilityIdentifier("btn-preset")
        case TIShellActionSettings:
            Button("Settings", systemImage: "gear") { viewModel.choose(item) }
                .labelStyle(.iconOnly)
                .controlSize(.large)
                .disabled(!item.enabled)
                .accessibilityIdentifier("btn-settings")
        default:
            Button(item.label) { viewModel.choose(item) }
                .disabled(!item.enabled)
                .accessibilityIdentifier("bar-action-\(item.itemID)")
        }
    }

    // MARK: - More

    private func moreMenu(_ control: TIBarControl) -> some View {
        Menu {
            MenuItems(items: control.choices, prefix: "more-item-") { item in
                if !item.confirm.isEmpty { confirming = item } else { viewModel.choose(item) }
            }
        } label: {
            Image(systemName: "ellipsis.circle")
                .font(.system(size: 18))
        }
        .menuStyle(ChromeMenuStyle())
        .fixedSize()
        .accessibilityLabel("More")
        .accessibilityIdentifier("bar-more")
    }
}

/// Menu items from the shell's model, submenus nested (the More menu and,
/// through ShellCommands, the menu bar's): disabled, not left out.
struct MenuItems: View {
    let items: [TIMenuItem]
    var prefix = "item-"
    let choose: (TIMenuItem) -> Void

    var body: some View {
        ForEach(Array(items.enumerated()), id: \.offset) { _, item in
            switch item.kind {
            case .separator:
                Divider()
            case .submenu:
                Menu(item.label) {
                    MenuItems(items: item.children, prefix: prefix, choose: choose)
                }
                .disabled(!item.enabled)
                .accessibilityIdentifier("\(prefix)\(item.itemID)")
            default:
                Button(role: item.destructive ? .destructive : nil) {
                    choose(item)
                } label: {
                    if item.checked { Label(item.label, systemImage: "checkmark") } else { Text(item.label) }
                }
                .disabled(!item.enabled)
                .reasonTag(item.reason)
                .accessibilityIdentifier("\(prefix)\(item.itemID)")
            }
        }
    }
}

/// The card controls on the bar: Arm Card and Arm Fresh Card
/// while no side passes, Eject Card while one does; they follow the card
/// reader's `insert` entry through the File menu's card items.
struct CardControls: View {
    @Environment(EmulatorViewModel.self) var viewModel
    var showLabels: Bool = true

    var body: some View {
        let arm = viewModel.menuItem("arm-card")
        let fresh = viewModel.menuItem("arm-fresh-card")
        if viewModel.cardState == .noCard {
            // Both read "Crd"; icon and screen-reader name differ.
            Button("Crd", systemImage: "square.and.arrow.down") {
                viewModel.cardPickerMode = .arm
            }
            .labelStyle(showLabel: showLabels)
            .controlSize(.regular)
            .disabled(!(arm?.enabled ?? true))
            .accessibilityLabel("Arm Card")
            .accessibilityIdentifier("bar-media-card-reader")
            .help("Arm Card: fit a saved magnetic card, to feed by a corner")
            Button("Crd", systemImage: "plus.rectangle") {
                viewModel.cardPickerMode = .armFresh
            }
            .labelStyle(showLabel: showLabels)
            .controlSize(.regular)
            .disabled(!(fresh?.enabled ?? true))
            .accessibilityLabel("Arm Fresh Card")
            .accessibilityIdentifier("bar-media-card-reader-fresh")
            .help("Arm Fresh Card: fit a fresh magnetic card under a name")
        } else {
            Button("Eject Card", systemImage: "eject") {
                viewModel.ejectIfSwiping()
            }
            .labelStyle(showLabel: showLabels)
            .accessibilityIdentifier("bar-media-card-reader")
        }
    }
}

extension View {
    /// Applies `.titleAndIcon` when `showLabel` is true, `.iconOnly` otherwise.
    @ViewBuilder
    func labelStyle(showLabel: Bool) -> some View {
        if showLabel { self.labelStyle(.titleAndIcon) }
        else { self.labelStyle(.iconOnly) }
    }

    /// Why a control cannot be used, as its help tag on macOS (which
    /// XCUITest reads as the element's accessibility help); not shown on iOS.
    @ViewBuilder
    func reasonTag(_ reason: String) -> some View {
        #if os(macOS)
        if reason.isEmpty { self } else { self.help(reason) }
        #else
        self
        #endif
    }
}
