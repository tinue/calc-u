import SwiftUI
import AudioToolbox

/// Full calculator canvas: base image (display + empty card slot + keyboard),
/// with runtime overlays for LED display and ML-01 card.
struct KeyboardView: View {
    @Environment(EmulatorViewModel.self) var viewModel

    // Canvas geometry (2064×4075 base image, display/card overlays, keyboard
    // section) lives in app/KeyboardLayout.hpp; rects are normalized.
    private static let imageAspect: CGFloat = TISharedTables.canvasAspect
    private static let displayRect = TISharedTables.displayRect
    private static let cardRect = TISharedTables.cardRect
    private static let kbYStart: CGFloat = TISharedTables.keyboardYStart
    private static let kbYScale: CGFloat = TISharedTables.keyboardYScale
    /// The base image's width in pixels: the hit map's unit (cu59::faceplateHitMap).
    private static let canvasPixels: CGFloat = TISharedTables.canvasWidthPixels

    // ── Colors ──────────────────────────────────────────────────────────

    private static let cardEdge = Color(red: 0x0A/255.0, green: 0x08/255.0, blue: 0x06/255.0)
    private static let maroon = Color(red: 0x1C/255.0, green: 0x06/255.0, blue: 0x06/255.0)

    /// The art by the control state's model (no copy of the model facts).
    private var canvasImageName: String { viewModel.modelID == "TI-59" || viewModel.modelID.isEmpty ? "ti59_base" : "ti58_base" }

    @State private var pointerIsDown = false
    /// The one pointer of the touch tracker.
    private static let pointer: Int64 = 1

    // ── Body ───────────────────────────────────────────────────────────────

    var body: some View {
        ZStack(alignment: .topLeading) {
            // Base canvas: display panel, empty card slot, keyboard
            Image(canvasImageName)
                .resizable()
                .scaledToFill()

            GeometryReader { geo in
                let w = geo.size.width
                let h = geo.size.height

                // ── Cue card overlay ────────────────────────────────────
                let cardW = w
                let cardH = h * Self.cardRect.height
                let cardY = h * Self.cardRect.minY

                CueCardView(content: viewModel.cueCardContent, litCorner: viewModel.litCardCorner)
                    .frame(width: cardW, height: cardH)
                    .position(x: cardW / 2, y: cardY + cardH / 2)
                    // The cue card the product topic names, in its template.
                    .accessibilityElement(children: .ignore)
                    .accessibilityLabel(viewModel.cueCardContent?.title ?? "")
                    .accessibilityValue(viewModel.cueCardContent?.template.rawValue ?? "")
                    .accessibilityIdentifier("cue-card")

                // ── LED display ─────────────────────────────────────────
                LEDDisplayView(
                    segments:      viewModel.displaySegments,
                    levels:        viewModel.displayLevels,
                    dpLevels:      viewModel.dpLevels,
                    calcIndicatorOpacity: viewModel.calcIndicatorOpacity,
                    fontStyle:     viewModel.ledStyle
                )
                .equatable()
                .accessibilityIdentifier("led-display")   // UI tests: a press here is full speed
                .frame(width:  w * Self.displayRect.width,
                       height: h * Self.displayRect.height)
                .position(x: w * Self.displayRect.midX,
                          y: h * Self.displayRect.midY)

                // ── Key hit-testing ─────────────────────────────────────
                // iOS 27 regressed DragGesture(minimumDistance: 0)'s reliability for
                // continuous press-and-hold (onEnded firing ~1ms after onChanged
                // regardless of real touch duration — see UIKitTouchTracker's doc
                // comment), so touches are tracked through raw UIKit.
                // The library's pointer rule: the key under the pointer, the
                // display for full speed; the card's corners first.
                let scale = w / Self.canvasPixels
                let handleChanged: (CGPoint) -> Void = { location in
                    if !pointerIsDown {
                        pointerIsDown = true
                        // A press, not a drag across the card, feeds a side.
                        if viewModel.tapCard(x: location.x / w, y: location.y / h) { return }
                        if viewModel.pointerDown(Self.pointer, x: location.x, y: location.y, scale: scale) {
                            viewModel.keyFeedback()
                        }
                        return
                    }
                    if viewModel.pointerMove(Self.pointer, x: location.x, y: location.y, scale: scale) {
                        viewModel.keyFeedback()
                    }
                }
                let handleEnded: () -> Void = {
                    pointerIsDown = false
                    viewModel.pointerUp(Self.pointer)
                }

                UIKitTouchTracker(onChanged: handleChanged, onEnded: handleEnded)
                    .frame(width: w, height: h)

                // ── Press highlight ─────────────────────────────────────
                // Simulate key press: shift key down-right and expose black background.
                // Physical-keyboard presses light the same highlight, so a typed key
                // looks exactly like a clicked one.
                if let pk = viewModel.pointerKey ?? viewModel.keyboardHeldKey {
                    let r = TISharedTables.keyRect(row: pk / 5, col: pk % 5)
                    let keyW = w * r.width
                    let keyH = h * r.height * Self.kbYScale
                    let canvasX = w * r.minX
                    let canvasY = h * (Self.kbYStart + r.minY * Self.kbYScale)
                    let shiftX: CGFloat = 3    // tunable: shift right (points)
                    let shiftY: CGFloat = 3    // tunable: shift down (points)

                    // Layer 1 — black fill at original key position (erases the key)
                    Rectangle()
                        .fill(Color.black)
                        .frame(width: keyW, height: keyH)
                        .position(x: canvasX + keyW / 2,
                                  y: canvasY + keyH / 2)
                        .allowsHitTesting(false)

                    // Layer 2 — key region of base image, shifted down-right
                    // Crop the image to show only the key rect, then position it shifted
                    Image(canvasImageName)
                        .resizable()
                        .frame(width: w, height: h)
                        .offset(x: -canvasX, y: -canvasY)   // align key's top-left to frame origin
                        .frame(width: keyW, height: keyH,
                               alignment: .topLeading)
                        .clipped()
                        .position(x: canvasX + keyW / 2 + shiftX,
                                  y: canvasY + keyH / 2 + shiftY)
                        .allowsHitTesting(false)
                }

                // ── "Loading…": a small sign below the display;
                // it takes no touches, so the keys under it can be used.
                if viewModel.loadingShown {
                    HStack(spacing: 6) {
                        ProgressView().controlSize(.small)
                        Text("Loading…").font(.caption.bold())
                    }
                    .padding(.horizontal, 10)
                    .padding(.vertical, 5)
                    .background(.ultraThinMaterial, in: Capsule())
                    .position(x: w / 2, y: h * Self.displayRect.maxY + 18)
                    .allowsHitTesting(false)
                    .accessibilityElement(children: .combine)
                    .accessibilityIdentifier("loading")
                }
            }
        }
        .aspectRatio(Self.imageAspect, contentMode: .fit)
        .accessibilityElement(children: .contain)
        .accessibilityIdentifier("faceplate")
        .onDisappear {
            // If this view is torn down mid-gesture (e.g. rapid panel switching),
            // the touch tracker's end never comes: let go of any held key.
            pointerIsDown = false
            viewModel.releasePointers()
            viewModel.keyboardRelease()
        }
    }
}

#Preview {
    KeyboardView()
        .environment(EmulatorViewModel())
        .background(Color(white: 0.08))
}
