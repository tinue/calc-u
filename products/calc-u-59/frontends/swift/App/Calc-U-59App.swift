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
                // LIB-12 AC-10, C59-8 AC-19: the lookup's text in place of the
                // faceplate; no bar, no Machine menu; Help still works.
                RomsMissingView(text: text) { viewModel.openHelp() }
                    .preferredColorScheme(.dark)
            } else {
                CalculatorView()
                    .environment(viewModel)
                    .preferredColorScheme(.dark)
                    // C59-8 AC-10: one notice after the other; over the
                    // settings sheet while it is open (EC-2: the sheet shows it).
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
        // The family's menus (C59-8 AC-1, AC-18; EC-6 on the iPad).
        .commands { ShellCommands(viewModel: viewModel) }
        #if os(macOS)
        // AC-7: the settings window of the app; Settings… in the application menu.
        Settings {
            SettingsView()
                .environment(viewModel)
        }
        #endif
    }
}

/// The ROM files are missing or bad (LIB-12 AC-5): what the lookup says, and
/// the help (C59-8 AC-19).
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
