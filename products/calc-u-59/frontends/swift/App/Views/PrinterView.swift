import SwiftUI

// The paper panel (C59-8 AC-12, EC-5; LIB-8 AC-12 to AC-20): always there,
// with the paper of its owner under the owner's name, or an empty paper
// under none while no printer is attached (the bar's "PC-100C" plugs it in).
// Drawn from the marks and the text of the library's topic `paper PC-100C`,
// which the bridge keeps; Copy and Cut are the session's commands (the
// library makes the picture, the bridge puts it on the pasteboard: macOS
// two entries, the picture first; iOS one with both, AC-13).
struct PrinterView: View {
    @Environment(EmulatorViewModel.self) var viewModel
    @State private var dotMode = true
    @State private var showCopiedToast = false
    @State private var toastText = "Copied to clipboard"
    var portraitTopInset: CGFloat = 0

    var body: some View {
        ZStack {
            Color(red: 29/255, green: 29/255, blue: 28/255)

            VStack(spacing: 0) {
                header
                paperStrip
                Spacer()
                buttonBar
                    .frame(height: 50)
                    .disabled(viewModel.paperOwner.isEmpty)   // the PC-100C's buttons, while it is attached
            }
            .background(Color(white: 29.0/255.0))
            .padding(.horizontal, 8)
            .padding(.vertical, 8)
        }
        .overlay(alignment: .bottom) {
            if showCopiedToast {
                Text(toastText)
                    .font(.caption.bold())
                    .foregroundStyle(.white)
                    .padding(.horizontal, 14)
                    .padding(.vertical, 7)
                    .background(Color.black.opacity(0.75))
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
            Spacer()
            // The owner's name; none while no printer is attached (EC-5).
            Text(viewModel.paperOwner)
                .font(.system(size: 11, weight: .bold))
                .foregroundStyle(.white)
                .padding(.vertical, 6)
                .accessibilityIdentifier("paper-owner")
            Spacer()
            // Dot / text toggle
            Button {
                dotMode.toggle()
            } label: {
                Image(systemName: dotMode ? "circle.grid.3x3.fill" : "text.alignleft")
                    .accessibilityLabel(dotMode ? "Text" : "Dots")
                    .font(.system(size: 11, weight: .bold))
                    .foregroundStyle(.white)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(Color(white: 0.25))
                    .clipShape(RoundedRectangle(cornerRadius: 6))
            }
            .buttonStyle(.plain)
            .accessibilityIdentifier("paper-view-mode")
        }
        .padding(.top, 8 + portraitTopInset).padding(.bottom, 8)
        .background(Color(white: 0.08))
    }

    // MARK: - Paper strip

    private var paperStrip: some View {
        ScrollView {
            if viewModel.paperOwner.isEmpty {
                Color.clear.frame(height: 1)   // an empty paper under none (EC-5)
            } else if dotMode {
                dotStrip
            } else {
                textStrip
            }
        }
        .scrollIndicators(.never)
        .defaultScrollAnchor(.bottom)
        .background(Color(red: 0.96, green: 0.94, blue: 0.88))
        .overlay(
            RoundedRectangle(cornerRadius: 2)
                .stroke(Color(white: 0.7), lineWidth: 0.5)
        )
        .shadow(color: .black.opacity(0.3), radius: 4, x: 0, y: 2)
    }

    // MARK: - Dot strip

    /// The paper in bands of EmulatorViewModel.paperBandUnits: only the bands
    /// in view are drawn (LazyVStack).
    private var dotStrip: some View {
        LazyVStack(spacing: 0) {
            ForEach(viewModel.paperBands.indices, id: \.self) { i in
                PaperBand(index: i, dots: viewModel.paperBands[i])
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
            .foregroundStyle(Color(white: 0.15))
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
                        Circle()
                            .fill(viewModel.printerTrace ? Color.red : Color.gray.opacity(0.4))
                            .frame(width: 8, height: 8)
                    }
                    .printerButtonChrome(compact: compact, shade: 0.25)
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
                        .foregroundStyle(.white)
                }
                .disabled(viewModel.printerLines.isEmpty || viewModel.paperOwner.isEmpty)
                .accessibilityLabel("Copy")
                .accessibilityIdentifier("paper-copy")

                Button { viewModel.cutPaper() } label: {
                    Image(systemName: "scissors")
                        .font(.system(size: 13))
                        .foregroundStyle(.white)
                }
                .disabled(viewModel.printerLines.isEmpty || viewModel.paperOwner.isEmpty)
                .accessibilityLabel("Cut")
                .accessibilityIdentifier("paper-cut")
            }
            .buttonStyle(ChromeButtonStyle())
            .padding(.horizontal, compact ? 6 : 12)
            .frame(maxWidth: .infinity)
            .frame(height: 50)
            .background(Color(white: 0.15))
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
            .printerButtonChrome(compact: compact, shade: 0.28)
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
    var dotColor: Color = Color(white: 0.12)

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
    /// The printer bar's button look; `compact` on the narrow tape.
    func printerButtonChrome(compact: Bool, shade: Double) -> some View {
        font(.caption.bold())
            .foregroundStyle(.white)
            .frame(minWidth: compact ? 30 : 36)
            .padding(.horizontal, compact ? 6 : 10)
            .padding(.vertical, 6)
            .background(Color(white: shade))
            .clipShape(RoundedRectangle(cornerRadius: 6))
    }
}
