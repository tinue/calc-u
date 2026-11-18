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
    /// The one pointer of a mouse or of the touch tracker.
    private static let pointer: Int64 = 1

    /// Whether physical keypresses go to the calculator. Only consumed on macOS,
    /// where the calculator shares the window with the printer and debug panels —
    /// see `PhysicalKeyboardModifier` below.
    @FocusState private var isKeyboardFocused: Bool

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
                // comment). Track touches via raw UIKit there instead; macOS mouse
                // input is unaffected and keeps the DragGesture it always used.
                // The library's pointer rule: the key under the pointer, the
                // display for full speed; the card's corners first.
                let scale = w / Self.canvasPixels
                let handleChanged: (CGPoint) -> Void = { location in
                    // Touching the calculator claims physical-keyboard
                    // focus back from whichever panel had it. The drag
                    // gesture consumes the click, so relying on
                    // .focusable()'s own click-to-focus isn't enough.
                    isKeyboardFocused = true
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

                #if os(macOS)
                Color.clear
                    .contentShape(Rectangle())
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onChanged { handleChanged($0.location) }
                            .onEnded { _ in handleEnded() }
                    )
                #else
                UIKitTouchTracker(onChanged: handleChanged, onEnded: handleEnded)
                    .frame(width: w, height: h)
                #endif

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
        .physicalKeyboard(viewModel: viewModel, focus: $isKeyboardFocused)
        .onDisappear {
            // If this view is torn down mid-gesture (e.g. rapid panel switching),
            // the DragGesture's onEnded never fires: let go of any held key.
            pointerIsDown = false
            viewModel.releasePointers()
            viewModel.keyboardRelease()
        }
    }
}

// MARK: - Physical keyboard (macOS)

private extension View {
    /// Route physical key presses to the calculator, on the platforms that have a
    /// keyboard to route. No-op off macOS: iOS/iPadOS is deliberately out of
    /// scope (see `reference/AppLayer.md` § "Physical Keyboard Mapping").
    @ViewBuilder
    func physicalKeyboard(viewModel: EmulatorViewModel, focus: FocusState<Bool>.Binding) -> some View {
        #if os(macOS)
        modifier(PhysicalKeyboardModifier(viewModel: viewModel, focus: focus))
        #else
        self
        #endif
    }
}

#if os(macOS)
/// Makes the calculator focusable and feeds key-down/key-up to the emulator.
///
/// Focus-scoped for now, and deliberately without a focus indicator: the Mac
/// window shows the calculator, the printer and the debug panels side by side,
/// so keys currently only reach the calculator while it holds focus — otherwise
/// arrow keys meant for the CPU inspector's instruction list would also step the
/// program. It takes focus on appear and reclaims it on touch, so typing works at
/// launch with nothing to select.
///
/// This is a stepping stone, not the target design. The Mac is meant to end up
/// with one unified input surface where every key has exactly one meaning and
/// focus stops mattering at all (the three panels only stay separate because
/// iPhone portrait swipes between them). See `TODO.md` § UI. Until then, note
/// that clicking into the debug panel silently stops keys reaching the
/// calculator, with nothing on screen to say so.
private struct PhysicalKeyboardModifier: ViewModifier {
    let viewModel: EmulatorViewModel
    let focus: FocusState<Bool>.Binding

    private var isFocused: Bool { focus.wrappedValue }

    /// The host key event of a key press, or nil for a key the calculator
    /// does not claim (a command chord, a key with no character or name).
    /// The mapping lives in app/TITyping (the library's typing); this
    /// only translates SwiftUI's KeyPress into the library's form.
    private struct HostKey {
        let physical: String, named: String
        let produced: UInt32, carried: UInt32
        let shift: Bool, control: Bool, option: Bool, command: Bool
    }
    private func hostKey(for keyPress: KeyPress) -> HostKey? {
        let key = keyPress.key.character
        let modifiers = keyPress.modifiers
        let command = modifiers.contains(.command), control = modifiers.contains(.control)
        let option = modifiers.contains(.option)
        if command || control || option { return nil }   // menus and the system
        var named = ""
        if keyPress.key == .escape { named = "escape" }
        else if keyPress.key == .return { named = "return" }
        else if key == "\u{3}" { named = "enter" }   // the number pad's Enter
        let produced = keyPress.characters.unicodeScalars.count == 1 ? keyPress.characters.unicodeScalars.first!.value : 0
        // The letter the key carries on the user's layout.
        let carried: UInt32 = key.isLetter && key.isASCII ? (key.lowercased().unicodeScalars.first?.value ?? 0) : 0
        if named.isEmpty && produced == 0 && carried == 0 { return nil }
        return HostKey(physical: String(key).lowercased(), named: named, produced: produced, carried: carried,
                       shift: modifiers.contains(.shift), control: control, option: option, command: command)
    }

    func body(content: Content) -> some View {
        content
            .focusable()
            .focusEffectDisabled()
            .focused(focus)
            .onAppear { focus.wrappedValue = true }
            .onChange(of: isFocused) { _, focused in
                // Every key held comes up, after its minimum hold.
                if !focused { viewModel.keyboardRelease() }
            }
            // Subscribing to .down and .up only — the host's auto-repeat
            // arrives as .repeat and is dropped here (the engine ignores it
            // too).
            .onKeyPress(phases: [.down, .up]) { keyPress in
                guard let k = hostKey(for: keyPress) else { return .ignored }
                switch keyPress.phase {
                case .down:
                    viewModel.hostKeyDown(physical: k.physical, named: k.named, produced: k.produced, carried: k.carried,
                                          shift: k.shift, control: k.control, option: k.option, command: k.command)
                    return .handled
                case .up:
                    viewModel.hostKeyUp(physical: k.physical)
                    return .handled
                default:
                    return .ignored
                }
            }
    }
}
#endif

#Preview {
    KeyboardView()
        .environment(EmulatorViewModel())
        .background(Color(white: 0.08))
}
