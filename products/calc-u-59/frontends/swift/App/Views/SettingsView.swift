import SwiftUI
#if os(macOS)
import AppKit
#endif

/// The settings: macOS's settings window of
/// the app, iOS's sheet whose one button is Done. The sections and rows are
/// the library's dialog model (`TIShell.settingsDialog` over
/// `cu59::settingsSpec`, the values of the `settings` topic, the startup
/// presets of the preset list); a change is `set-setting`, applied and
/// written at once. A notice that comes while the sheet is open shows over
/// it.
struct SettingsView: View {
    @Environment(EmulatorViewModel.self) var viewModel
    #if !os(macOS)
    @Environment(\.dismiss) private var dismiss
    #endif
    @State private var authRefresh = UUID()

    var body: some View {
        #if os(macOS)
        form
            .frame(minWidth: 460, minHeight: 540)
            .padding()
        #else
        NavigationStack {
            form
                .navigationTitle("Settings")
                .toolbar {
                    // Icon too: bars on the iPhone Duo's outer display are vertical
                    // and show only items that have one.
                    ToolbarItem(placement: .confirmationAction) {
                        Button("Done", systemImage: "checkmark") { dismiss() }
                            .accessibilityIdentifier("settings-close")
                    }
                }
        }
        .frame(maxHeight: .infinity)
        .noticeAlert(viewModel)
        #endif
    }

    private var traceLocation: String {
        viewModel.settingsSections.flatMap(\.rows).first { $0.section == "trace" && $0.key == "location" }?.value ?? ""
    }

    private var form: some View {
        Form {
            ForEach(Array(viewModel.settingsSections.enumerated()), id: \.offset) { _, section in
                Section(section.title) {
                    ForEach(Array(section.rows.enumerated()), id: \.offset) { _, row in
                        rowView(row)
                    }
                }
            }
        }
        .formStyle(.grouped)
        .accessibilityIdentifier("settings-dialog")
    }

    @ViewBuilder
    private func rowView(_ row: TIDialogRow) -> some View {
        let id = "setting-\(row.section)-\(row.key)"
        switch row.widget {
        case .note:
            HStack {
                Text(row.label)
                Spacer()
            }
            .foregroundStyle(.secondary)
            .accessibilityIdentifier(id)
        case .integer:
            HStack {
                Text(row.label)
                Spacer()
                TextField("", value: Binding(
                    get: { Int(row.value) ?? 0 },
                    set: { viewModel.setSetting(row.section, row.key, String($0)) }
                ), format: .number)
                .textFieldStyle(.roundedBorder)
                .frame(width: 90)
                .accessibilityIdentifier(id)
            }
        case .folder:
            // The trace folder of `[trace] location = folder` (macOS).
            if row.section != "trace" || traceLocation == "folder" {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text(row.shown.isEmpty ? "No folder chosen" : row.shown)
                            .foregroundStyle(.secondary)
                            .lineLimit(1)
                            .truncationMode(.head)
                        Spacer()
                        Button("Choose…") { chooseFolder() }
                            .accessibilityIdentifier(id)
                    }
                    if AppSettings.traceFolderNeedsReauthorization(path: row.value) {
                        Text("This folder needs to be authorized again. Click Choose… to allow trace files there.")
                            .foregroundStyle(.orange)
                            .font(.caption)
                            .id(authRefresh)
                    }
                }
            }
        case .text:
            HStack {
                Text(row.label)
                Spacer()
                Text(row.value).foregroundStyle(.secondary)
            }
        default:
            if row.isToggle {
                Toggle(row.label, isOn: Binding(
                    get: { row.value == "true" },
                    set: { viewModel.setSetting(row.section, row.key, $0 ? "true" : "false") }
                ))
                .accessibilityIdentifier(id)
            } else {
                Picker(row.label, selection: Binding(
                    get: { row.value },
                    set: { viewModel.setSetting(row.section, row.key, $0) }
                )) {
                    ForEach(Array(row.choiceValues.enumerated()), id: \.offset) { i, value in
                        Text(row.choiceLabels[i]).tag(value)
                    }
                }
                .accessibilityIdentifier(id)
            }
        }
    }

    private func chooseFolder() {
        #if os(macOS)
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose a folder for CALCU59_TRACE.bin"
        if panel.runModal() == .OK, let url = panel.url {
            viewModel.chooseTraceFolder(url)   // [trace] folder, and the security-scoped bookmark
            authRefresh = UUID()
        }
        #endif
    }
}

extension View {
    /// A notice is an alert whose only text is the notice, with OK
    /// (a question: Yes and No); the next follows when it is closed.
    func noticeAlert(_ viewModel: EmulatorViewModel, when shown: Bool = true) -> some View {
        modifier(NoticeAlert(viewModel: viewModel, shown: shown))
    }
}

/// The notice read in the modifier's body, so the view follows it
/// (Observation); the alert's own state follows the notice shown, so a
/// window that redraws at 60 Hz keeps it up.
private struct NoticeAlert: ViewModifier {
    let viewModel: EmulatorViewModel
    let shown: Bool
    @State private var presented = false
    @State private var text = ""
    @State private var question = false

    private var wanted: String? { shown ? viewModel.currentNotice : nil }

    /// Shows the notice waiting, a moment after the last alert went: an
    /// alert asked for while the one before is still going away is never
    /// shown (the button's action already moved the queue on).
    private func later() {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.35) {
            guard !presented, let next = wanted else { return }
            text = next
            question = viewModel.noticeIsQuestion
            presented = true
        }
    }

    func body(content: Content) -> some View {
        content
            .onChange(of: wanted, initial: true) { _, _ in later() }
            .onChange(of: presented) { _, up in if !up { later() } }
            .alert(text, isPresented: $presented) {
                if question {
                    Button("Yes") { viewModel.answerNotice(true) }
                        .accessibilityIdentifier("notice-yes")
                    Button("No", role: .cancel) { viewModel.answerNotice(false) }
                        .accessibilityIdentifier("notice-no")
                } else {
                    Button("OK") { viewModel.closeNotice() }
                        .accessibilityIdentifier("notice-ok")
                }
            }
    }
}
