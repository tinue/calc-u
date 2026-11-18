import SwiftUI
#if os(macOS)
import AppKit
#endif

@main
struct CalcU59App: App {
    @State private var viewModel = EmulatorViewModel()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            if let text = viewModel.romsMissingText {
                // The lookup's text in place of the
                // faceplate; no bar, no Machine menu; Help still works.
                RomsMissingView(text: text) { viewModel.openHelp() }
                    .preferredColorScheme(.dark)
            } else {
                CalculatorView()
                    .environment(viewModel)
                    .preferredColorScheme(.dark)
                    // One notice after the other; over the settings
                    // sheet while it is open, the sheet shows it.
                    .noticeAlert(viewModel, when: !viewModel.settingsSheetShown)
            }
        }
        .onChange(of: scenePhase) { _, newPhase in
            switch newPhase {
            case .background:
                viewModel.suspendForBackground()
            case .active:
                viewModel.resumeFromBackground()
            default:
                break
            }
        }
        // The shell's menus (also in the iPad's system menu bar).
        .commands { ShellCommands(viewModel: viewModel) }
        #if os(macOS)
        // The settings window of the app; Settings… in the application menu.
        Settings {
            SettingsView()
                .environment(viewModel)
        }
        #endif
    }
}

/// The ROM files are missing or bad: what the lookup says, and
/// the help.
struct RomsMissingView: View {
    let text: String
    var help: () -> Void = {}

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                Text(text)
                    .font(.body.monospaced())
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .accessibilityIdentifier("romsMissingText")
                Button("Calc-U-59 Help", action: help)
                    .accessibilityIdentifier("item-help")
            }
            .padding()
        }
        .accessibilityIdentifier("roms-missing")
    }
}
