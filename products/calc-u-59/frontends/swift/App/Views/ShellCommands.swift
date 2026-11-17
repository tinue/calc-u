import SwiftUI

/// The family's menus in the menu bar (C59-8 AC-1, AC-2, AC-18, EC-6;
/// design §3.2, D-4): File's items take the place of the `.newItem` group,
/// Machine is a menu of its own, Help takes the place of the `.help` group.
/// The items come from the library's menu model (`TIShell.menus`); an item
/// that cannot be used now is disabled, not left out. Settings…, About and
/// Quit are the application menu's own (the Settings scene, the About panel,
/// macOS's Quit), so the items with those roles are left to it. The
/// shortcuts are key equivalents, taken before the keyboard map sees the key
/// (LIB-14 AC-4). The same commands serve the iPad's menu bar.
struct ShellCommands: Commands {
    let viewModel: EmulatorViewModel

    private func menu(_ id: String) -> TIMenu? { viewModel.menus.first { $0.menuID == id } }

    var body: some Commands {
        CommandGroup(replacing: .newItem) {
            if let file = menu("file") { ShellMenuItems(items: file.items, viewModel: viewModel) }
        }
        CommandMenu("Machine") {
            if let machine = menu("machine") { ShellMenuItems(items: machine.items, viewModel: viewModel) }
        }
        CommandGroup(replacing: .help) {
            if let help = menu("help") { ShellMenuItems(items: help.items, viewModel: viewModel) }
            else {
                // AC-19: Help still works when the product could not start.
                Button("Calc-U-59 Help") { viewModel.openHelp() }
            }
        }
        #if os(macOS)
        CommandGroup(replacing: .appInfo) {
            Button("About Calc-U-59") { viewModel.perform(TIShellActionAbout) }   // AC-11: the system's panel
        }
        #endif
    }
}

/// The items of one menu, with their key equivalents (AC-1).
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
                .reasonTag(item.reason)
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
