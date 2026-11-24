import SwiftUI
import UniformTypeIdentifiers

struct DebugView: View {
    @Environment(EmulatorViewModel.self) var vm
    @Binding var activeFilePickerMode: CalculatorView.FilePickerMode?
    var portraitTopInset: CGFloat = 0

    var body: some View {
        VStack(spacing: 0) {
            // Tab bar: the panel header, as high as the printer's beside it.
            HStack(spacing: 0) {
                // The product entry c59.calculator-tab:
                // not while an ASM overlay runs.
                tabButton("CALCULATOR", .live)
                    .disabled(!vm.calculatorTabAvailable)
                    .opacity(vm.calculatorTabAvailable ? 1.0 : 0.35)
                tabButton("CPU", .cpu)
                tabButton("RUN LOG", .log)
                Spacer()
            }
            .frame(height: Chrome.panelHeaderHeight)
            .background(Chrome.debugHeader)
            .padding(.top, portraitTopInset)
            .background(Chrome.debugHeader)

            // Tab content. Held to the space below the tab bar: content taller
            // than the panel (CPU tab in a short window) would otherwise push
            // the VStack past its frame and lay itself over the tabs.
            Group {
                switch vm.debugTab {
                case .live: LiveDebugView()
                case .cpu:
                    VStack(spacing: 0) {
                        CPUInspectorView()
                            .frame(maxHeight: .infinity)

                        Divider().background(Chrome.pill)

                        ASMDebugContent(activeFilePickerMode: $activeFilePickerMode)
                    }
                case .log:
                    StaticDebugContent()
                }
            }
            .frame(maxHeight: .infinity, alignment: .top)
            .clipped()
            .contentShape(Rectangle())
        }
        .background(Chrome.debugBackground)
        // The chrome's inset above and below, as the printer column has it,
        // so the headers and the button bars side by side line up.
        .padding(.vertical, Chrome.panelInset)
        .background(Chrome.debugBackground)
        .onChange(of: vm.asmOverlayActive) { _, active in
            if active { vm.debugTab = .cpu }
        }
        // Applies to every default-style Button in this subtree (tab bar,
        // CPUInspectorView, LiveDebugView, ASMDebugContent, StaticDebugContent) —
        // see ChromeButtonStyle in ChromeControlStyles.swift.
        .buttonStyle(ChromeButtonStyle())
    }

    private func tabButton(_ label: String, _ tab_: DebugTab) -> some View {
        Button(label) { vm.debugTab = tab_ }
            .font(.system(size: 10, weight: .bold))
            .foregroundStyle(vm.debugTab == tab_ ? Chrome.text : Chrome.text.opacity(0.4))
            .padding(.horizontal, 12)
            .frame(maxHeight: .infinity)
            .background(vm.debugTab == tab_ ? Chrome.tabSelected : Color.clear)
            .accessibilityIdentifier("btn-tab-\(label.lowercased())")
    }
}

// MARK: - Static Debug Content (original DebugView body)

private struct StaticDebugContent: View {
    @Environment(EmulatorViewModel.self) var vm

    var body: some View {
        VStack(spacing: 0) {
            header
            outputArea
            buttonBar
        }
        .background(Chrome.debugBackground)
    }

    // MARK: - Header

    private var header: some View {
        Text("RUN LOG")
            .font(.caption.bold())
            .foregroundStyle(Chrome.text.opacity(0.6))
            .frame(maxWidth: .infinity)
            .padding(.vertical, 6)
            .background(Chrome.debugHeader)
    }

    // MARK: - Selectable output area

    private var outputArea: some View {
        ScrollViewReader { proxy in
            ScrollView {
                Text(vm.debugDisplayText.isEmpty ? " " : vm.debugDisplayText)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(Chrome.logText)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                    .textSelection(.enabled)
                    .id(vm.debugClearID)
                Color.clear.frame(height: 1).id("bottom")
            }
            .background(Chrome.section)
            .overlay(
                RoundedRectangle(cornerRadius: 2)
                    .stroke(Chrome.pill, lineWidth: 0.5)
            )
            .onChange(of: vm.debugDisplayText) {
                withAnimation(.easeOut(duration: 0.15)) {
                    proxy.scrollTo("bottom", anchor: .bottom)
                }
            }
        }
    }

    // MARK: - Button bar

    // Two rows, each a panel button bar, the lower one at the panel's
    // bottom beside the printer's.
    private var buttonBar: some View {
        VStack(spacing: 0) {
            // Row 1: Main action buttons
            HStack(spacing: 8) {
                Button("SCOM") { vm.debugDumpSCOM() }
                Button("Memory") { vm.debugDumpMemory() }
                Button("Prog") { vm.debugDumpProg() }
                Button("Vars") { vm.debugDumpVars() }

                Spacer()

                // Clear button
                Button {
                    vm.clearDebug()
                } label: {
                    Image(systemName: "trash")
                        .foregroundStyle(Chrome.text.opacity(0.6))
                }
                .disabled(vm.debugDisplayText.isEmpty)
            }
            .frame(height: Chrome.panelButtonsHeight)

            // Row 2: Status toggles
            HStack(spacing: 8) {
                Button { vm.cIndicatorDebug.toggle() } label: {
                    HStack(spacing: 4) {
                        Text("TRACE")
                        ChromeDot(on: vm.cIndicatorDebug, colour: Chrome.orange)
                    }
                    .chromePill()
                }
                .buttonStyle(.plain)
                // TRACE can always be pressed; a trace folder that cannot be
                // written refuses the start.
                .accessibilityIdentifier("btn-trace")

                Button { vm.toggleDebug() } label: {
                    HStack(spacing: 4) {
                        Text("RUN LOG")
                        ChromeDot(on: vm.debugLevel != .off, colour: vm.debugLevel == .debug ? Chrome.red : Chrome.orange)
                    }
                    .chromePill()
                }
                .buttonStyle(.plain)

                Spacer()
            }
            .frame(height: Chrome.panelButtonsHeight)
        }
        .font(.caption.bold())
        .foregroundStyle(Chrome.text)
        .padding(.horizontal, 10)
        .background(Chrome.buttonBar)
    }
}

private struct ASMDebugContent: View {
    @Environment(EmulatorViewModel.self) var vm
    @Binding var activeFilePickerMode: CalculatorView.FilePickerMode?

    var body: some View {
        VStack(spacing: 0) {
            Text("ASM OVERLAY")
                .font(.caption.bold())
                .foregroundStyle(Chrome.text.opacity(0.6))
                .frame(maxWidth: .infinity)
                .padding(.vertical, 6)
                .background(Chrome.debugHeader)

            VStack(alignment: .leading, spacing: 8) {
                Text("File: \(vm.asmFileName)")
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(Chrome.text.opacity(0.85))
                    .accessibilityIdentifier("asm-file")
                Text("Words: \(vm.asmWordCount)")
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(Chrome.text.opacity(0.75))
                    .accessibilityIdentifier("asm-words")
                Text(vm.asmStatusMessage)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(Chrome.text.opacity(0.7))
                    .fixedSize(horizontal: false, vertical: true)
                    .accessibilityIdentifier("asm-status")
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(10)
            .background(Chrome.section)
            .overlay(
                RoundedRectangle(cornerRadius: 2)
                    .stroke(Chrome.pill, lineWidth: 0.5)
            )
            .padding(8)

            HStack(spacing: 8) {
                Button("Select File") {
                    activeFilePickerMode = .asm
                }
                .buttonStyle(.plain)
                .chromePill()
                .accessibilityIdentifier("btn-asm-select")

                Button("Run") {
                    vm.runASMOverlay()
                }
                .buttonStyle(.plain)
                .chromePill()
                .disabled(!vm.canRunASM)
                .accessibilityIdentifier("btn-asm-run")

                Button("Clear") {
                    vm.clearASMOverlay()
                }
                .buttonStyle(.plain)
                .chromePill()

                Spacer()
            }
            .padding(.horizontal, 10)
            .frame(height: Chrome.panelButtonsHeight)
            .background(Chrome.buttonBar)
        }
        .background(Chrome.debugBackground)
    }
}

#Preview {
    @Previewable @State var mode: CalculatorView.FilePickerMode? = nil
    DebugView(activeFilePickerMode: $mode)
        .environment({
            let vm = EmulatorViewModel()
            vm.debugLevel = .info
            vm.debugDisplayText = """
                ── RAM (part=60) ──
                RAM[60] = 3.141592653589
                RAM[65] = 7.77e+22
                ── SCOM ──
                SCOM[9] = [6, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
                """
            return vm
        }())
        .frame(width: 260, height: 400)
}
