import Foundation

#if os(macOS)
import AppKit
#endif

enum AppInfo {
    // The About texts and links of every frontend (app/TIShellProduct).
    static var subtitle: String { TIShell.aboutLines.first ?? "" }
    static var aboutSummary: String { TIShell.aboutLines.dropFirst().joined(separator: "\n") }
    static var aboutLinks: [(title: String, url: String)] {
        TIShell.aboutLinks.compactMap { $0.count == 2 ? ($0[0], $0[1]) : nil }
    }
}

#if os(macOS)
extension AppInfo {
    static var aboutPanelOptions: [NSApplication.AboutPanelOptionKey: Any] {
        [
            // The panel shows "Version <applicationVersion> (<version>)".
            .applicationVersion: TIShell.versionText,
            .version: TIShell.buildId,
            .credits: aboutCredits
        ]
    }

    private static let aboutCredits: NSAttributedString = {
        let text = NSMutableAttributedString()

        let centered = NSMutableParagraphStyle()
        centered.alignment = .center

        let subtitleAttrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: 12, weight: .semibold),
            .foregroundColor: NSColor.labelColor,
            .paragraphStyle: centered
        ]

        let bodyAttrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: 11),
            .foregroundColor: NSColor.secondaryLabelColor,
            .paragraphStyle: centered
        ]

        text.append(NSAttributedString(string: "\(subtitle)\n", attributes: subtitleAttrs))
        text.append(NSAttributedString(string: "\(aboutSummary)\n\n", attributes: bodyAttrs))

        for (i, link) in aboutLinks.enumerated() {
            if i > 0 { text.append(NSAttributedString(string: "\n", attributes: bodyAttrs)) }
            text.append(linkLine(title: link.title, urlString: link.url, paragraph: centered))
        }

        return text
    }()

    private static func linkLine(title: String, urlString: String, paragraph: NSParagraphStyle) -> NSAttributedString {
        guard let url = URL(string: urlString) else {
            return NSAttributedString(string: title, attributes: [
                .font: NSFont.systemFont(ofSize: 11),
                .paragraphStyle: paragraph
            ])
        }

        return NSAttributedString(string: title, attributes: [
            .font: NSFont.systemFont(ofSize: 11),
            .foregroundColor: NSColor.linkColor,
            .underlineStyle: NSUnderlineStyle.single.rawValue,
            .link: url,
            .paragraphStyle: paragraph
        ])
    }
}
#endif