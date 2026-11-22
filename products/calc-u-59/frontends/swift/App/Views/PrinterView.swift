import SwiftUI

// The paper panel: always there,
// with the paper of its owner under the owner's name, or an empty paper
// under none while no printer is attached; the PC-100C's button at the
// header's left plugs it in.
// Drawn from the marks and the text of the library's topic `paper PC-100C`,
// which the bridge keeps; Copy and Cut are the session's commands (the
// library makes the picture, the bridge puts it on the pasteboard: one
// entry with both).
struct PrinterView: View {
    @Environment(EmulatorViewModel.self) var viewModel
    @State private var dotMode = true
    @State private var showCopiedToast = false
    @State private var toastText = "Copied to clipboard"
    var portraitTopInset: CGFloat = 0

    var body: some View {
        ZStack {
            Chrome.window

            VStack(spacing: 0) {
                header
                paperStrip
                Spacer()
                buttonBar
                    .frame(height: Chrome.panelButtonsHeight)
                    .disabled(viewModel.paperOwner.isEmpty)   // the PC-100C's buttons, while it is attached
            }
            .background(Chrome.window)
            .padding(.horizontal, Chrome.panelInset)
            .padding(.vertical, Chrome.panelInset)
        }
        .overlay(alignment: .bottom) {
            if showCopiedToast {
                Text(toastText)
                    .font(.caption.bold())
                    .foregroundStyle(Chrome.text)
                    .padding(.horizontal, 14)
                    .padding(.vertical, 7)
                    .background(Chrome.toast)
                    .clipShape(RoundedRectangle(cornerRadius: 10))
                    .padding(.bottom, 48)
                    .transition(.opacity.combined(with: .scale(scale: 0.9)))
            }
        }
        .animation(.easeInOut(duration: 0.2), value: showCopiedToast)
        .onChange(of: viewModel.paperCopySerial) {
            toastText = viewModel.paperCopyMessage ?? "Copied to clipboard"
            showCopiedToast = true
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.8) { showCopiedToast = false }
        }
    }

    // MARK: - Header

    private var header: some View {
        HStack(spacing: 12) {
            // The place controls on the paper (the PC-100C); the owner's name
            // in the middle says what it is.
            ForEach(viewModel.paperPlaces, id: \.controlID) { control in
                PlaceButton(control: control, showLabel: false)
            }
            Spacer()
            // Dot / text toggle
            Button {
                dotMode.toggle()
            } label: {
                Image(systemName: dotMode ? "circle.grid.3x3.fill" : "text.alignleft")
                    .accessibilityLabel(dotMode ? "Text" : "Dots")
                    .chromePill()
            }
            .buttonStyle(.plain)
            .accessibilityIdentifier("paper-view-mode")
        }
        // The owner's name, none while no printer is attached: an overlay, so
        // it never changes the column's width; one line, centred.
        .overlay {
            Text(viewModel.paperOwner)
                .font(.system(size: 11, weight: .bold))
                .foregroundStyle(Chrome.headerText)
                .lineLimit(1)
                .fixedSize()
                .accessibilityIdentifier("paper-owner")
        }
        .frame(height: Chrome.panelHeaderHeight)
        .padding(.top, portraitTopInset)
        .background(Chrome.header)
    }

    // MARK: - Paper strip

    private var paperStrip: some View {
        ScrollView {
            if viewModel.paperOwner.isEmpty {   // no printer: a hint where the paper would start
                Text("No printer attached.")
                    .foregroundStyle(.secondary)
                    .padding(.top, 12)
                    .accessibilityIdentifier("paper-hint")
            } else if dotMode {
                dotStrip
            } else {
                textStrip
            }
        }
        .scrollIndicators(.never)
        .defaultScrollAnchor(.bottom)
        .background(Chrome.output)
        .overlay(
            RoundedRectangle(cornerRadius: 2)
                .stroke(Chrome.outputBorder, lineWidth: 0.5)
        )
        .shadow(color: .black.opacity(0.3), radius: 4, x: 0, y: 2)
    }

    // MARK: - Dot strip

    /// The paper in bands of EmulatorViewModel.paperBandUnits: only the bands
    /// in view are drawn (LazyVStack). Each row gets its band as a value: a
    /// cut empties `paperBands`, and SwiftUI may still update the rows in
    /// view before the ForEach sees the shorter array, so no row may read
    /// the array by index.
    private var dotStrip: some View {
        LazyVStack(spacing: 0) {
            ForEach(Array(viewModel.paperBands.enumerated()), id: \.offset) { i, dots in
                PaperBand(index: i, dots: dots)
            }
        }
        .padding(.horizontal, TIViewRules.printerPaperPaddingX)
        .padding(.vertical, TIViewRules.printerPaperPaddingY)
        .id(viewModel.printerClearID)
    }

    // MARK: - Text strip (fallback / copy-paste mode)

    private var textStrip: some View {
        Text(viewModel.printerLines.isEmpty ? " " : viewModel.printerLines.joined(separator: "\n"))
            .font(.system(size: 13, design: .monospaced))
            .foregroundStyle(Chrome.outputText)
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .textSelection(.enabled)
            .accessibilityIdentifier("paper-text")
            .id(viewModel.printerClearID)
    }

    // MARK: - Hardware button bar

    private var buttonBar: some View {
        GeometryReader { geo in
            // The narrow tape beside the debugger (iPhone Duo).
            let compact = TIViewRules.compactPrinterBar(geo.size.width)
            let printLabel = compact ? "PRN" : "PRINT"
            HStack(spacing: compact ? 6 : 12) {
                printerButton(printLabel, compact: compact) {
                    viewModel.pressPrinterPrint(true)
                } onRelease: {
                    viewModel.pressPrinterPrint(false)
                }

                Button {
                    viewModel.togglePrinterTrace()
                } label: {
                    HStack(spacing: 4) {
                        Text("T")
                        ChromeDot(on: viewModel.printerTrace)
                    }
                    .printerButtonChrome(compact: compact)
                }
                .buttonStyle(.plain)

                printerButton("ADV", compact: compact) {
                    viewModel.pressPrinterAdv(true)
                } onRelease: {
                    viewModel.pressPrinterAdv(false)
                }

                Spacer(minLength: 0)

                Button { viewModel.copyPaper() } label: {
                    Image(systemName: "doc.on.doc")
                        .font(.system(size: 13))
                        .foregroundStyle(Chrome.text)
                }
                .disabled(viewModel.printerLines.isEmpty || viewModel.paperOwner.isEmpty)
                .accessibilityLabel("Copy")
                .accessibilityIdentifier("paper-copy")

                Button { viewModel.cutPaper() } label: {
                    Image(systemName: "scissors")
                        .font(.system(size: 13))
                        .foregroundStyle(Chrome.text)
                }
                .disabled(viewModel.printerLines.isEmpty || viewModel.paperOwner.isEmpty)
                .accessibilityLabel("Cut")
                .accessibilityIdentifier("paper-cut")
            }
            .buttonStyle(ChromeButtonStyle())
            .padding(.horizontal, compact ? 6 : 12)
            .frame(maxWidth: .infinity)
            .frame(height: Chrome.panelButtonsHeight)
            .background(Chrome.buttonBar)
        }
    }

    // MARK: - Press-and-hold button helper

    @ViewBuilder
    private func printerButton(
        _ label: String,
        compact: Bool,
        onPress: @escaping () -> Void,
        onRelease: @escaping () -> Void
    ) -> some View {
        Text(label)
            .printerButtonChrome(compact: compact)
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { _ in onPress() }
                    .onEnded   { _ in onRelease() }
            )
    }
}

// MARK: - Band renderer

/// One band of the paper: its dots, each a disc of the PC-100C's dot size
/// with its ink (the grey of its darkness), across the paper's whole width.
struct PaperBand: View {
    let index: Int
    let dots: [TIPaperDot]
    var dotColor: Color = Chrome.ink

    private static let widthUnits = TIViewRules.printerWidthUnits
    private static let bandUnits = EmulatorViewModel.paperBandUnits
    private static let dotRadiusFraction = TIViewRules.printerDotRadiusFraction

    var body: some View {
        Canvas { ctx, size in
            let pitch = size.width / Self.widthUnits
            let r = pitch * Self.dotRadiusFraction
            let top = Double(index) * Self.bandUnits
            for d in dots {
                let x = d.x * pitch, y = (d.y - top) * pitch
                let rect = CGRect(x: x - r, y: y - r, width: 2 * r, height: 2 * r)
                ctx.fill(Path(ellipseIn: rect), with: .color(dotColor.opacity(d.ink)))
            }
        }
        .aspectRatio(Self.widthUnits / Self.bandUnits, contentMode: .fit)
    }
}

// MARK: - Preview

#Preview {
    let vm = EmulatorViewModel()
    vm.printerLines = [
        "  -3.1415926536",
        "   1.2345678901",
        "",
        "   SIN",
        "   0.",
    ]
    vm.paperReachUnits = 40   // blank bands: good enough to check the layout
    return PrinterView()
        .environment(vm)
        .frame(width: 220, height: 400)
}

private extension View {
    /// The printer bar's button look: a pill, narrower on the narrow tape
    /// (`compact`).
    func printerButtonChrome(compact: Bool) -> some View {
        chromePill(minWidth: compact ? 30 : 36)
    }
}
