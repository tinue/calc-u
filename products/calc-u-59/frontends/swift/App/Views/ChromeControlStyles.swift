import SwiftUI

/// Opts a `Button` out of the system's automatic "Button Shapes" accessibility
/// decoration, which only decorates controls using the default/automatic
/// style — see
/// https://developer.apple.com/documentation/swiftui/environmentvalues/accessibilityshowbuttonshapes
/// Any control given an *explicit* style (this one, or even Apple's own
/// `.plain`) is left alone by the system. Without this, default-style
/// buttons got an oversized system-drawn shape when the setting was on,
/// inflating their layout footprint enough to shove surrounding UI around.
///
/// Deliberately renders `configuration.label` unmodified — these controls
/// sit on a photorealistic calculator skin where exact visual parity with
/// the undecorated look matters more than reproducing the Button Shapes
/// affordance itself.
struct ChromeButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .opacity(configuration.isPressed ? 0.5 : 1.0)
            .contentShape(Rectangle())
    }
}

/// Same rationale as `ChromeButtonStyle`, for `Menu` and `Picker(.menu)`
/// controls, which style via `MenuStyle` rather than `ButtonStyle`.
/// `Menu(configuration)` rebuilds the default menu trigger appearance
/// exactly, so this is purely an opt-out of the automatic decoration with
/// no visual change.
struct ChromeMenuStyle: MenuStyle {
    func makeBody(configuration: Configuration) -> some View {
        Menu(configuration)
    }
}

// MARK: - Chrome

extension Color {
    /// A colour of the chrome (TIChrome, app/TIChrome).
    init(_ c: TIChromeColour) {
        self.init(.sRGB, red: Double(c.red), green: Double(c.green), blue: Double(c.blue), opacity: Double(c.alpha))
    }
}

/// Calc-U-59's look and layout (app/TIChrome through the bridge), the same
/// values the Qt6 and Web GUIs draw with.
enum Chrome {
    static let window = Color(TIChrome.window)
    static let text = Color(TIChrome.text)
    static let header = Color(TIChrome.header)
    static let headerText = Color(TIChrome.headerText)
    static let buttonBar = Color(TIChrome.buttonBar)
    static let pill = Color(TIChrome.pill)
    static let pillText = Color(TIChrome.pillText)
    static let output = Color(TIChrome.output)
    static let outputText = Color(TIChrome.outputText)
    static let outputBorder = Color(TIChrome.outputBorder)
    static let plugged = Color(TIChrome.plugged)
    static let unplugged = Color(TIChrome.unplugged)
    // The product's own (cu59::chrome).
    static let debugBackground = named("debug-background")
    static let debugHeader = named("debug-header")
    static let section = named("section")
    static let tabSelected = named("tab-selected")
    static let orange = named("orange")
    static let red = named("red")
    static let yellow = named("yellow")
    static let green = named("green")
    static let stepCurrent = named("step-current")
    static let heatmapPc = named("heatmap-pc")
    static let hover = named("hover")
    static let logText = named("log-text")
    static let toast = named("toast")
    static let ink = named("ink")
    static func named(_ name: String) -> Color { Color(TIChrome.named(name)) }

    static let panelHeaderHeight = TIChrome.panelHeaderHeight
    static let panelButtonsHeight = TIChrome.panelButtonsHeight
    static let panelInset = TIChrome.panelInset
    static let pillShape = TIChrome.pillShape
    static let barHeight = TIChrome.barHeight
    static let barGap = TIChrome.barGap
    static let paperRowGap = TIChrome.paperRowGap
    static let paperRowCompactGap = TIChrome.paperRowCompactGap
    static var pillFont: Font {
        .system(size: TIChrome.pillFontSize, weight: TIChrome.pillBold ? .bold : .regular)
    }
}

extension View {
    /// A pill button's look (the chrome's shape, colours and font).
    func chromePill() -> some View {
        let shape = Chrome.pillShape
        return font(Chrome.pillFont)
            .foregroundStyle(Chrome.pillText)
            .padding(.horizontal, shape.paddingX)
            .frame(minHeight: shape.height)
            .background(Chrome.pill)
            .clipShape(RoundedRectangle(cornerRadius: shape.radius))
    }
}

/// The dot of a place button or a toggle: in its colour while on (plugged
/// in), the chrome's unplugged grey while off.
struct ChromeDot: View {
    var on: Bool
    var colour: Color = Chrome.plugged
    var body: some View {
        Circle().fill(on ? colour : Chrome.unplugged).frame(width: 8, height: 8)
    }
}
