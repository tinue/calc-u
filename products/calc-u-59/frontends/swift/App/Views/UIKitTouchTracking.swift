import SwiftUI

#if !os(macOS)
import UIKit

/// Raw UIKit touch tracking, bypassing SwiftUI's `DragGesture`.
///
/// On iOS 27, `DragGesture(minimumDistance: 0)` often fires `onEnded` within
/// ~1ms of `onChanged`, as if the touch was released the instant it began,
/// however long the touch is held (about 1 in 20 presses registers; iOS 26.5
/// is reliable). `UIView.touchesBegan` / `touchesMoved` / `touchesEnded` /
/// `touchesCancelled`, one layer below SwiftUI's gesture recognition, don't
/// show this, so `KeyboardView` tracks its keys with this type.
///
/// Only single-touch tracking is needed: the calculator's key matrix has no
/// multi-touch behavior.
struct UIKitTouchTracker: UIViewRepresentable {
    var onChanged: (CGPoint) -> Void
    var onEnded: () -> Void

    final class TrackingView: UIView {
        var onChanged: ((CGPoint) -> Void)?
        var onEnded: (() -> Void)?

        override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
            guard let touch = touches.first else { return }
            onChanged?(touch.location(in: self))
        }

        override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
            guard let touch = touches.first else { return }
            onChanged?(touch.location(in: self))
        }

        override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
            onEnded?()
        }

        override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
            onEnded?()
        }
    }

    func makeUIView(context: Context) -> TrackingView {
        let view = TrackingView()
        view.backgroundColor = .clear
        view.isMultipleTouchEnabled = false
        view.onChanged = onChanged
        view.onEnded = onEnded
        return view
    }

    func updateUIView(_ uiView: TrackingView, context: Context) {
        uiView.onChanged = onChanged
        uiView.onEnded = onEnded
    }
}
#endif
