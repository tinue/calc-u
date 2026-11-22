import SwiftUI

/// The shell's menus in the iPad's menu bar: File's items take the place of the
/// `.newItem` group, Machine is a menu of its own, Help takes the place of the
/// `.help` group. The items come from the library's menu model
/// (`TIShell.menus`); an item that cannot be used now is disabled, not left
/// out. Items with a role of the platform's own menu are left out (Settings is
/// the bar's). The shortcuts are key equivalents of a hardware keyboard.
struct ShellCommands: Commands {
    let viewModel: EmulatorViewModel

    private func menu(_ id: String) -> TIMenu? { viewModel.menus.first { $0.menuID == id } }

    var body: some Commands {
        CommandGroup(replacing: .newItem) {
            if let file = menu("file") { ShellMenuItems(items: file.items, viewModel: viewModel) }
            else {
                // The session starts after the menu bar is built: a group
                // empty at launch takes the File menu away for good.
                Button("Load Preset…") {}.disabled(true)
            }
        }
        CommandMenu("Machine") {
            if let machine = menu("machine") { ShellMenuItems(items: machine.items, viewModel: viewModel) }
        }
        CommandGroup(replacing: .help) {
            if let help = menu("help") { ShellMenuItems(items: help.items, viewModel: viewModel) }
            else {
                // Help still works when the product could not start.
                Button("Calc-U-59 Help") { viewModel.openHelp() }
            }
        }
    }
}

/// The items of one menu, with their key equivalents.
struct ShellMenuItems: View {
    let items: [TIMenuItem]
    let viewModel: EmulatorViewModel

    /// The items the application menu holds itself (Settings…, About, Quit).
    private var shown: [TIMenuItem] {
        let kept = items.filter { $0.role == .none }
        // No separator first, last or twice once those are gone.
        var out: [TIMenuItem] = []
        for i in kept {
            if i.kind == .separator && (out.isEmpty || out.last?.kind == .separator) { continue }
            out.append(i)
        }
        while out.last?.kind == .separator { out.removeLast() }
        return out
    }

    var body: some View {
        ForEach(Array(shown.enumerated()), id: \.offset) { _, item in
            switch item.kind {
            case .separator:
                Divider()
            case .submenu:
                Menu(item.label) {
                    ShellMenuItems(items: item.children, viewModel: viewModel)
                }
                .disabled(!item.enabled)
            default:
                Button {
                    viewModel.choose(item)
                } label: {
                    if item.checked { Label(item.label, systemImage: "checkmark") } else { Text(item.label) }
                }
                .disabled(!item.enabled)
                .keyboardShortcut(Self.shortcut(item.shortcut))
            }
        }
    }

    private static func shortcut(_ s: TIMenuShortcut) -> KeyboardShortcut? {
        switch s {
        case .loadPreset: KeyboardShortcut("o", modifiers: .command)
        case .reset: KeyboardShortcut("r", modifiers: .command)
        case .coldBoot: KeyboardShortcut("r", modifiers: [.command, .shift])
        default: nil
        }
    }
}
