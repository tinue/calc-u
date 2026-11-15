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
                // LIB-12 AC-10 (design D-8, until C59-8): the lookup's text in
                // place of the calculator; nothing else is offered.
                RomsMissingView(text: text)
                    .preferredColorScheme(.dark)
            } else {
                CalculatorView()
                    .environment(viewModel)
                    .preferredColorScheme(.dark)
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
        #if os(macOS)
        Settings {
            SettingsView(viewModel: viewModel)
        }
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About Calc-U-59") {
                    NSApp.orderFrontStandardAboutPanel(options: AppInfo.aboutPanelOptions)
                }
            }
        }
        #endif
    }
}

/// The ROM files are missing or bad (LIB-12 AC-5): what the lookup says.
struct RomsMissingView: View {
    let text: String

    var body: some View {
        ScrollView {
            Text(text)
                .font(.body.monospaced())
                .textSelection(.enabled)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding()
                .accessibilityIdentifier("romsMissingText")
        }
    }
}
