import SwiftUI
#if os(macOS)
import AppKit
#endif

struct SettingsView: View {
    #if !os(macOS)
    @Environment(\.dismiss) private var dismiss
    #endif

    var viewModel: EmulatorViewModel

    /// The settings file (Calc-U-59.ini, LIB-9); every change is written through.
    @Bindable private var settings = SettingsModel.shared

    @State private var allModules: [ModuleMetadata] = []
    @State private var pendingModuleID: String = ""

    // Trigger refresh of warning after re-authorization
    @State private var authRefreshTrigger: UUID = UUID()

    var body: some View {
        #if os(macOS)
        settingsForm
            .frame(minWidth: 460, minHeight: 540)
            .padding()
            .onAppear {
                allModules = viewModel.allModules()
                pendingModuleID = viewModel.selectedModuleID
            }
            .onDisappear { viewModel.changeModule(id: pendingModuleID) }
        #else
        NavigationStack {
            settingsForm
                .navigationTitle("Settings")
                .toolbar {
                    // Icon too: bars on the iPhone Duo's outer display are vertical
                    // and show only items that have one.
                    ToolbarItem(placement: .confirmationAction) {
                        Button("Done", systemImage: "checkmark") { dismiss() }
                    }
                }
        }
        .frame(maxHeight: .infinity)
        .onAppear {
            allModules = viewModel.allModules()
            pendingModuleID = viewModel.selectedModuleID
        }
        .onDisappear { viewModel.changeModule(id: pendingModuleID) }
        #endif
    }

    private var settingsForm: some View {
        Form {
            Section("General") {
                Picker("Startup Calculator", selection: $settings.startupModelRaw) {
                    ForEach(MachineModel.allCases) { m in
                        Text(m.displayName).tag(m.rawValue)
                    }
                    Divider()
                    Text("Last Used").tag(-1)
                }

                Picker("Solid State Module", selection: $pendingModuleID) {
                    ForEach(allModules, id: \.id) { module in
                        Text(module.menuTitle).tag(module.id)
                    }
                }

                // Keyboard feedback and portrait layout only on iOS
                #if os(iOS)
                Picker("Keyboard Feedback", selection: $settings.keyboardFeedbackRaw) {
                    ForEach(KeyboardFeedbackType.allCases) { feedback in
                        Text(feedback.displayName).tag(feedback.rawValue)
                    }
                }
                Toggle("Debug Page in Portrait", isOn: $settings.portraitDebugPage)
                #endif

                HStack {
                    Text("Version")
                    Spacer()
                    Text(AppInfo.versionNumberString)
                        .foregroundStyle(.secondary)
                }
            }

            Section("Display") {
                Picker("Segment Style", selection: $settings.ledFontStyleRaw) {
                    ForEach(LEDFontStyle.allCases) { style in
                        Text(style.displayName).tag(style.rawValue)
                    }
                }
            }

            Section("Trace File") {
                #if os(macOS)
                Picker("Save Location", selection: $settings.traceLocationRaw) {
                    Text(TraceLocation.local.displayName).tag(TraceLocation.local.rawValue)
                    Text(TraceLocation.iCloud.displayName).tag(TraceLocation.iCloud.rawValue)
                    Text(TraceLocation.custom.displayName).tag(TraceLocation.custom.rawValue)
                }
                if settings.traceLocationRaw == TraceLocation.custom.rawValue {
                    HStack {
                        Text(settings.traceCustomPath.isEmpty ? "No folder chosen" : settings.traceCustomPath)
                            .foregroundStyle(.secondary)
                            .lineLimit(1)
                            .truncationMode(.head)
                        Spacer()
                        Button("Choose…") { chooseCustomFolder() }
                    }

                    // Show warning if path exists but no bookmark (old settings without re-authorization)
                    Group {
                        if AppSettings.customTracePathNeedsReauthorization() {
                            Text("⚠️ This folder needs to be re-authorized. Click 'Choose…' again to allow trace file writing.")
                                .foregroundStyle(.orange)
                                .font(.caption)
                        }
                    }
                    .id(authRefreshTrigger)  // Re-evaluate when trigger changes
                }
                #endif

                HStack {
                    Text("Maximum File Size")
                    Spacer()
                    HStack(spacing: 4) {
                        TextField("", value: $settings.traceMaxFileSizeMB, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 90)
                        Text("MB")
                            .foregroundStyle(.secondary)
                    }
                }
            }
        }
        .formStyle(.grouped)
    }

    private func chooseCustomFolder() {
        #if os(macOS)
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose a folder for CALCU59_TRACE.bin"
        if panel.runModal() == .OK, let url = panel.url {
            print("[SettingsView] User selected folder: \(url.path)")
            // [trace] folder, and the security-scoped bookmark (AC-34)
            AppSettings.setCustomTraceDirectory(url)
            print("[SettingsView] Folder saved with bookmark")
            // Trigger refresh of warning so it disappears if bookmark was successful
            authRefreshTrigger = UUID()
            print("[SettingsView] Triggered UI refresh")
        }
        #endif
    }
}

#Preview {
    SettingsView(viewModel: EmulatorViewModel())
}
