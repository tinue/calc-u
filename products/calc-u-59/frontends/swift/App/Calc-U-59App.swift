import SwiftUI
#if os(macOS)
import AppKit
#endif

#if os(macOS)
/// The Finder's Open With hands its files to the application, which
/// passes them to the one machine; no file opens a window of its own.
final class AppDelegate: NSObject, NSApplicationDelegate {
    @MainActor static weak var viewModel: EmulatorViewModel?

    @MainActor func application(_ application: NSApplication, open urls: [URL]) {
        Self.viewModel?.openWith(urls)
    }
}
#endif

@main
struct CalcU59App: App {
    @State private var viewModel: EmulatorViewModel
    @Environment(\.scenePhase) private var scenePhase
    #if os(macOS)
    @NSApplicationDelegateAdaptor private var appDelegate: AppDelegate
    #endif

    init() {
        let made = EmulatorViewModel()
        _viewModel = State(initialValue: made)
        #if os(macOS)
        AppDelegate.viewModel = made
        #endif
    }

    @ViewBuilder
    private var content: some View {
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

    var body: some Scene {
        // A fixed id keeps the window's saved frame under one name; the
        // default name comes from the view's type, which changes per launch.
        // The Mac has one machine and so one window.
        #if os(macOS)
        Window("Calc-U-59", id: "calculator") { content }
            .onChange(of: scenePhase) { _, newPhase in phaseChanged(newPhase) }
            .commands { ShellCommands(viewModel: viewModel) }
        // The settings window of the app; Settings… in the application menu.
        Settings {
            SettingsView()
                .environment(viewModel)
        }
        #else
        WindowGroup(id: "calculator") { content }
            .onChange(of: scenePhase) { _, newPhase in phaseChanged(newPhase) }
            // The shell's menus (also in the iPad's system menu bar).
            .commands { ShellCommands(viewModel: viewModel) }
        #endif
    }

    private func phaseChanged(_ newPhase: ScenePhase) {
        switch newPhase {
        case .background:
            viewModel.suspendForBackground()
        case .active:
            viewModel.resumeFromBackground()
        default:
            break
        }
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
