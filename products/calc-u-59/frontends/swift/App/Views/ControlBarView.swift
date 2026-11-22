import SwiftUI

/// The control bar below the faceplate, drawn from the library's bar model
/// (`TIShell.bar`) over the control state: the model picker first, the
/// picker of `slot-1` (the library's module picker), the card controls on
/// the TI-59; More at the right edge (Load Preset…, Settings…, Reset and the
/// rest of the menus). The PC-100C's button is on the
/// paper panel (`TIShell.paperPlaces`). No control here
/// decides what the machine offers: a control is enabled exactly when its
/// entry is available now.
struct ControlBarView: View {
    @Environment(EmulatorViewModel.self) var viewModel
    /// The labels ("Crd") beside the icons; the column decides by
    /// the library's rule (TIBarFit).
    var labels: Bool = true

    /// More ▸ Cold boot asks before it is carried out.
    @State private var confirming: TIMenuItem?

    var body: some View {
        row(labels: labels)
        .buttonStyle(ChromeButtonStyle())
        .padding(.horizontal, 12)
        // Leading: without the TI-59's card controls the rest stays put.
        .frame(maxWidth: .infinity, alignment: .leading)
        .frame(height: 50)
        .background(Color(white: 0.15))
        .foregroundStyle(.white)
        .accessibilityElement(children: .contain)
        .accessibilityIdentifier("control-bar")
    }

    /// The bar model's groups; a spread bar (Calc-U-59): the model and the
    /// module left, the card controls in the middle, More right.
    private func row(labels: Bool) -> some View {
        let controls = viewModel.barControls
        return HStack(spacing: 12) {
            ForEach(Array(controls.enumerated()), id: \.offset) { index, control in
                // A group the bar model pushes (a spread bar) takes the room before it.
                if index > 0, control.group != controls[index - 1].group, control.groupPushed {
                    Spacer(minLength: 0)
                }
                view(for: control, labels: labels)
            }
        }
    }

    @ViewBuilder
    private func view(for control: TIBarControl, labels: Bool) -> some View {
        switch control.kind {
        case .modelPicker:
            picker(control, label: control.shown, id: "bar-model")
        case .slotPicker:
            slotPicker(control)
        case .placePicker:
            picker(control, label: control.shortShown ? control.shown : (control.choices.first(where: \.checked)?.label ?? "—"),
                   id: "bar-place-\(control.controlID)")
        case .placeButton:
            PlaceButton(control: control, showLabel: labels)
        case .media:
            CardControls(showLabels: labels)
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

    /// A module slot's picker: the `media` topic's entries in their order
    /// (the product's listing), the chosen one's short label ("ML") on the bar.
    private func slotPicker(_ control: TIBarControl) -> some View {
        let entries = viewModel.slotEntries[control.controlID] ?? []
        let chosen = entries.first(where: \.chosen)
        let shown = (control.shortShown ? chosen?.shortLabel : chosen?.label) ?? "—"
        return Menu {
            ForEach(Array(entries.enumerated()), id: \.offset) { _, entry in
                Button {
                    viewModel.choose(entry, place: control.controlID)
                } label: {
                    if entry.chosen { Label(entry.label, systemImage: "checkmark") } else { Text(entry.label) }
                }
                .disabled(!entry.canChoose)
                // "item-module:LE"; the empty slot "item-slot-1-none"
                .accessibilityIdentifier("item-" + (entry.entryID.isEmpty ? "\(control.controlID)-none" : entry.entryID))
            }
        } label: {
            Text(control.label.isEmpty ? shown : "\(control.label) \(shown)")
                .font(.callout.weight(.semibold))
                .fixedSize()
        }
        .menuStyle(ChromeMenuStyle())
        .menuOrder(.fixed)
        .fixedSize()
        .disabled(!(control.choices.first?.enabled ?? true))
        .accessibilityIdentifier("bar-slot-\(control.controlID)")
    }

    // MARK: - More

    private func moreMenu(_ control: TIBarControl) -> some View {
        Menu {
            MenuItems(items: control.choices, prefix: "more-item-") { item in
                if !item.confirm.isEmpty { confirming = item; return }
                if item.itemID == "reset" { viewModel.feedbackForReset() }
                viewModel.choose(item)
            }
        } label: {
            Image(systemName: "ellipsis.circle")
                .font(.system(size: 18))
        }
        .menuStyle(ChromeMenuStyle())
        // In the model's order, top to bottom: opened upwards from the bar,
        // iOS would otherwise put the first item next to the button.
        .menuOrder(.fixed)
        .fixedSize()
        .accessibilityLabel("More")
        .accessibilityIdentifier("bar-more")
        // On the More button, so the iPad's popover points at it.
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
}

/// A place's button (the library's place button), on the bar or the paper
/// panel: pressed (red) while plugged in; its label, or the printer where
/// the label would not fit or the name stands beside it.
struct PlaceButton: View {
    @Environment(EmulatorViewModel.self) var viewModel
    let control: TIBarControl
    var showLabel: Bool

    var body: some View {
        let item = control.choices.first
        Button {
            if let item { viewModel.choose(item) }
        } label: {
            HStack(spacing: 6) {
                if showLabel { Text(control.label) } else { Image(systemName: "printer") }
                Circle()
                    .fill(control.plugged ? Color.red : Color.gray.opacity(0.4))
                    .frame(width: 8, height: 8)
            }
            .font(.system(size: 11, weight: .bold))
            .foregroundStyle(.white)
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(Color(white: 0.25))
            .clipShape(RoundedRectangle(cornerRadius: 6))
        }
        .buttonStyle(.plain)
        .disabled(!(item?.enabled ?? false))
        .accessibilityLabel(control.label)
        .accessibilityValue(control.plugged ? "On" : "Off")
        .accessibilityIdentifier("bar-place-\(control.controlID)")
    }
}
