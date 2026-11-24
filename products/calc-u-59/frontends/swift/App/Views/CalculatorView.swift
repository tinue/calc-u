import SwiftUI
import UniformTypeIdentifiers
import AudioToolbox

struct CalculatorView: View {
    @Environment(EmulatorViewModel.self) var viewModel

    enum FilePickerMode { case asm, preset }
    // Two-variable design: filePickerMode is the source of truth for routing;
    // filePickerPresented is the Bool that drives isPresented. SwiftUI may
    // reset filePickerPresented to false on tap-outside dismiss (iPad popover)
    // without calling the result handler, leaving filePickerMode non-nil.
    // filePickerModeBinding always sets filePickerPresented = true on any write,
    // so re-tapping "Select File" reopens the picker even if mode is unchanged.
    @State private var filePickerMode: FilePickerMode?
    @State private var filePickerPresented = false

    private var filePickerModeBinding: Binding<FilePickerMode?> {
        Binding(
            get: { filePickerMode },
            set: { newMode in
                filePickerMode = newMode
                if newMode != nil { filePickerPresented = true }
            }
        )
    }

    private static let asmTypes = [UTType(filenameExtension: "asm") ?? .plainText]
    /// The presets' types (Load Preset…, Settings ▸ Choose…).
    static let presetTypes = [
        UTType(filenameExtension: "ti59") ?? .data,
        UTType(filenameExtension: "ti58") ?? .data,
        UTType(filenameExtension: "ti58c") ?? .data
    ]

    enum PortraitPage { case calc, printer, debug }
    @State private var portraitPage: PortraitPage = .calc
    /// The faceplate's drawn width and the bar's width without and with its
    /// labels: the library's rule makes the bar of them (TIBarFit).
    @State private var faceplateWidth: CGFloat = 0
    @State private var barWithoutLabels: CGFloat = 0
    @State private var barWithLabels: CGFloat = 0
    private var debugPageEnabled: Bool { viewModel.portraitDebugPage }

    var body: some View {
        layout
        .dynamicTypeSize(.small ... .large)
        .sheet(item: .init(
            get: { viewModel.cardPickerMode.map { PickerItem(mode: $0) } },
            set: { viewModel.cardPickerMode = $0?.mode }
        )) { item in
            CardPickerView(
                mode: item.mode,
                cards: viewModel.savedCards,
                onArm: { viewModel.armCard($0) },
                onArmFresh: { name, confirmed, asked in viewModel.armFreshCard(name: name, confirmed: confirmed, asked: asked) }
            )
        }
        .sheet(isPresented: Binding(get: { viewModel.settingsSheetShown },
                                    set: { viewModel.settingsSheetShown = $0 })) {
            SettingsView()
                .environment(viewModel)
        }
        // The shell asks for the file dialog (Load Preset…) and the settings.
        .onChange(of: viewModel.presetDialogSerial) { openPresetDialog() }
        .onChange(of: viewModel.settingsSerial) { viewModel.settingsSheetShown = true }
        .fileImporter(
            isPresented: $filePickerPresented,
            allowedContentTypes: {
                switch filePickerMode {
                case .asm:       return Self.asmTypes
                case .preset, .none: return Self.presetTypes
                }
            }(),
            allowsMultipleSelection: false
        ) { result in
            // Capture mode before any reset — on iPad, set(false) may have
            // already fired (for tap-outside cancel) but filePickerMode is
            // intentionally NOT reset there, so it's always valid here.
            let mode = filePickerMode
            filePickerMode = nil
            filePickerPresented = false

            let urls: [URL]
            switch result {
            case .success(let picked):
                urls = picked
            case .failure(let error):
                TIAppLog.write("The file picker failed: \(error.localizedDescription)", level: .warn)
                return
            }
            guard let url = urls.first else { return }

            switch mode {
            case .asm:       viewModel.loadASMOverlayFile(url)
            case .preset: viewModel.loadPreset(url)
            case .none:      break
            }
        }
    }

    /// Load Preset… (More):
    /// presets only, as every dedicated loader takes its own kind.
    private func openPresetDialog() {
        filePickerModeBinding.wrappedValue = .preset
    }

    @ViewBuilder
    private var layout: some View {
        // The library's view rule: side by side where the printer fits
        // beside the faceplate (the debugger too where it fits), else the
        // pages; also the pages over an active fold, whose upper half
        // alone is too low.
        GeometryReader { geo in
            let view = TIViewRules.viewWidth(geo.size.width, height: geo.size.height)
            if view.sideBySide && !Self.hasHorizontalFold(geo) {
                HStack(spacing: 0) {
                    calculatorBody()
                        .fixedSize(horizontal: true, vertical: false)
                    Divider()
                    PrinterView()
                        .frame(minWidth: TIChrome.printerMinWidth, maxWidth: TIChrome.printerMaxWidth)
                    if view.debugger {
                        Divider()
                        // Clipped: its .background(Color) fills would otherwise
                        // spread into the safe area, past the other two columns.
                        DebugView(activeFilePickerMode: filePickerModeBinding)
                            .frame(minWidth: TIChrome.debuggerMinWidth)
                            .clipped()
                    }
                }
                .padding(.top, TIViewRules.mobileTopMargin(geo.safeAreaInsets.top))
            } else {
                portraitPages()
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .onAppear { portraitPage = .calc }
                    .onChange(of: debugPageEnabled) { _, enabled in
                        if !enabled && portraitPage == .debug { portraitPage = .printer }
                    }
            }
        }
    }

    private func calculatorBody() -> some View {
        ZStack {
            Chrome.window

            VStack(spacing: 0) {
                KeyboardView()
                    .onGeometryChange(for: CGFloat.self) { $0.size.width } action: { faceplateWidth = $0 }
                Spacer()
                let fit = TIBarFit.fit(faceplate: faceplateWidth, withoutLabels: barWithoutLabels, withLabels: barWithLabels)
                BarWidthLayout(width: fit.width) {
                    ControlBarView(labels: fit.labels)
                }
                .background {   // the bar measured both ways, unseen
                    VStack {
                        ControlBarView(labels: false).fixedSize()
                            .onGeometryChange(for: CGFloat.self) { $0.size.width } action: { barWithoutLabels = $0 }
                        ControlBarView(labels: true).fixedSize()
                            .onGeometryChange(for: CGFloat.self) { $0.size.width } action: { barWithLabels = $0 }
                    }
                    .hidden()
                    .accessibilityHidden(true)
                }
            }
            .background(Chrome.window)
            .padding(.horizontal, Chrome.panelInset)
            .padding(.vertical, Chrome.panelInset)
        }
    }

    // Identifiable wrapper so .sheet(item:) works with the enum
    private struct PickerItem: Identifiable {
        let mode: CardPickerView.Mode
        var id: Int { mode == .arm ? 0 : 1 }
    }

    /// An active fold across the window (iPhone Duo half open, "laptop" pose).
    /// https://developer.apple.com/documentation/technologyoverviews/preparing-your-app-for-iphone-duo
    private static func hasHorizontalFold(_ geo: GeometryProxy) -> Bool {
        guard #available(iOS 27.1, *) else { return false }
        return geo.reservedRegions(kind: .division).contains { $0.frame.width > $0.frame.height }
    }

    @ViewBuilder
    private func portraitPages() -> some View {
        ZStack {
            switch portraitPage {
            case .calc:
                calculatorBody()
                    .overlay(alignment: .topLeading) {
                        if debugPageEnabled {
                            pageArrow(systemImage: "chevron.left", id: "btn-page-left") { portraitPage = .debug }
                        }
                    }
                    .overlay(alignment: .topTrailing) {
                        pageArrow(systemImage: "chevron.right", id: "btn-page-right") { portraitPage = .printer }
                    }
            case .printer:
                PrinterView(portraitTopInset: 20)
                    .overlay(alignment: .topLeading) {
                        pageArrow(systemImage: "chevron.left", id: "btn-page-left") { portraitPage = .calc }
                    }
                    .overlay(alignment: .topTrailing) {
                        if debugPageEnabled {
                            pageArrow(systemImage: "chevron.right", id: "btn-page-right") { portraitPage = .debug }
                        }
                    }
            case .debug:
                DebugView(activeFilePickerMode: filePickerModeBinding, portraitTopInset: 36)
                    .overlay(alignment: .topLeading) {
                        pageArrow(systemImage: "chevron.left", id: "btn-page-left") { portraitPage = .printer }
                    }
                    .overlay(alignment: .topTrailing) {
                        pageArrow(systemImage: "chevron.right", id: "btn-page-right") { portraitPage = .calc }
                    }
            }
        }
    }

    private func pageArrow(systemImage: String, id: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            Image(systemName: systemImage)
                .font(.system(size: 14, weight: .semibold))
                .foregroundStyle(.white.opacity(0.7))
                .padding(8)
                .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
        }
        .buttonStyle(ChromeButtonStyle())
        .accessibilityIdentifier(id)
        .padding(.top, 2)
        .padding(.horizontal, 8)
        .padding(.bottom, 8)
    }
}

/// The control bar at its width (TIBarFit), never wider than the column;
/// centred under the faceplate by the column.
private struct BarWidthLayout: Layout {
    var width: CGFloat

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        guard let bar = subviews.first else { return .zero }
        let w = min(width, proposal.width ?? .infinity)
        return CGSize(width: w, height: bar.sizeThatFits(ProposedViewSize(width: w, height: proposal.height)).height)
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        subviews.first?.place(at: bounds.origin, proposal: ProposedViewSize(width: bounds.width, height: bounds.height))
    }
}

#Preview {
    CalculatorView()
        .environment(EmulatorViewModel())
}
