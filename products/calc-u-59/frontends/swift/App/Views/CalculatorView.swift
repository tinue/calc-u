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
    private static let presetTypes = [
        UTType(filenameExtension: "ti59") ?? .data,
        UTType(filenameExtension: "ti58") ?? .data,
        UTType(filenameExtension: "ti58c") ?? .data
    ]
    /// The Preset button's dialog: the preset types and any
    /// file; the session's `open` finds out what it is.
    private static let presetDialogTypes = presetTypes + [.item]


    #if !os(macOS)
    enum PortraitPage { case calc, printer, debug }
    @State private var portraitPage: PortraitPage = .calc
    private var debugPageEnabled: Bool { viewModel.portraitDebugPage }
    #endif

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
        #if !os(macOS)
        .sheet(isPresented: Binding(get: { viewModel.settingsSheetShown },
                                    set: { viewModel.settingsSheetShown = $0 })) {
            SettingsView()
                .environment(viewModel)
        }
        #endif
        // The shell asks for the file dialog (Load Preset…) and the settings (iOS).
        .onChange(of: viewModel.presetDialogSerial) { openPresetDialog() }
        .onChange(of: viewModel.settingsSerial) { viewModel.settingsSheetShown = true }
        .fileImporter(
            isPresented: $filePickerPresented,
            allowedContentTypes: {
                switch filePickerMode {
                case .asm:       return Self.asmTypes
                case .preset, .none: return Self.presetDialogTypes
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

            guard case .success(let urls) = result, let url = urls.first else {
                return
            }

            switch mode {
            case .asm:       viewModel.loadASMOverlayFile(url)
            case .preset: viewModel.loadPreset(url)
            case .none:      break
            }
        }
        #if os(macOS)
        .dropDestination(for: URL.self) { urls, _ in
            guard urls.count == 1, let url = urls.first, url.isFileURL else { return false }
            return viewModel.openDroppedFile(url)
        }
        #endif
    }

    /// Load Preset… (File menu, ⌘O; the Preset button on iOS): the preset
    /// types and any file, opened as what it is.
    private func openPresetDialog() {
        #if os(macOS)
        let panel = NSOpenPanel()
        panel.allowedContentTypes = Self.presetDialogTypes
        panel.allowsOtherFileTypes = true
        panel.message = "Select a .ti59, .ti58, or .ti58c preset, or any file Calc-U-59 opens"
        if panel.runModal() == .OK, let url = panel.url {
            viewModel.loadPreset(url)
        }
        #else
        filePickerModeBinding.wrappedValue = .preset
        #endif
    }

    @ViewBuilder
    private var layout: some View {
        #if os(macOS)
        // macOS: calculator | printer | debug
        // Calculator uses its intrinsic width; debug panel absorbs extra space.
        HStack(spacing: 0) {
            calculatorBody()
                .fixedSize(horizontal: true, vertical: false)
            Divider()
            PrinterView()
                .frame(minWidth: 220, maxWidth: 320)
            Divider()
            DebugView(activeFilePickerMode: filePickerModeBinding)
                .frame(minWidth: 220)
        }
        #else
        // iOS/iPadOS: side-by-side when wide, button-navigated pages otherwise
        // (Shared mobileLayout decides by the window's size and fold)
        GeometryReader { geo in
            // Hide button labels when the calculator is too narrow to fit them comfortably.
            let showLabels = TIViewRules.mobileBarLabels(geo.size.width)
            let layout = TIViewRules.mobileLayoutWidth(geo.size.width, height: geo.size.height,
                                                       horizontalFold: Self.hasHorizontalFold(geo))
            if layout != .paged {
                let printerWidth = TIViewRules.mobilePrinterWidth(layout, width: geo.size.width)
                HStack(spacing: 0) {
                    calculatorBody(showLabels: showLabels)
                        .fixedSize(horizontal: true, vertical: false)
                    Divider()
                    PrinterView()
                        .frame(minWidth: printerWidth.min, maxWidth: printerWidth.max)
                    if layout == .calcPrinterDebugger {
                        Divider()
                        // Clipped: its .background(Color) fills would otherwise
                        // spread into the safe area, past the other two columns.
                        DebugView(activeFilePickerMode: filePickerModeBinding)
                            .frame(minWidth: 220)
                            .clipped()
                    }
                }
                .padding(.top, TIViewRules.mobileTopMargin(geo.safeAreaInsets.top))
            } else {
                portraitPages(showLabels: showLabels)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .onAppear { portraitPage = .calc }
                    .onChange(of: debugPageEnabled) { _, enabled in
                        if !enabled && portraitPage == .debug { portraitPage = .printer }
                    }
            }
        }
        #endif
    }

    private func calculatorBody(showLabels: Bool = true) -> some View {
        ZStack {
            Color(red: 29/255, green: 29/255, blue: 28/255)

            VStack(spacing: 0) {
                KeyboardView()
                Spacer()
                ControlBarView(showLabels: showLabels)
                    // No ideal width: the faceplate alone sets the column's
                    // width where the column is sized to its content.
                    .frame(minWidth: 0, idealWidth: 0, maxWidth: .infinity)
            }
            .background(Color(white: 29.0/255.0))
            .padding(.horizontal, 8)
            .padding(.vertical, 8)
        }
    }

    // Identifiable wrapper so .sheet(item:) works with the enum
    private struct PickerItem: Identifiable {
        let mode: CardPickerView.Mode
        var id: Int { mode == .arm ? 0 : 1 }
    }

    #if !os(macOS)
    /// An active fold across the window (iPhone Duo half open, "laptop" pose).
    /// https://developer.apple.com/documentation/technologyoverviews/preparing-your-app-for-iphone-duo
    private static func hasHorizontalFold(_ geo: GeometryProxy) -> Bool {
        guard #available(iOS 27.1, *) else { return false }
        return geo.reservedRegions(kind: .division).contains { $0.frame.width > $0.frame.height }
    }

    @ViewBuilder
    private func portraitPages(showLabels: Bool) -> some View {
        ZStack {
            switch portraitPage {
            case .calc:
                calculatorBody(showLabels: showLabels)
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
    #endif
}

#Preview {
    CalculatorView()
        .environment(EmulatorViewModel())
}
