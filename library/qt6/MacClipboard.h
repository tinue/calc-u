#pragma once
#include <QImage>
#include <QString>

// The Qt6 component set's macOS clipboard path: ClipboardService puts a
// paper's picture through it, and a product's own Copy of the screen may too.
#ifdef Q_OS_MACOS
namespace calcu::qt6 {
// Writes `source` to the general pasteboard as an NSImage sized to
// widthPt x heightPt (Cocoa points, 1/72in). Needed because Qt's
// cross-platform QClipboard::setImage()/setPixmap() silently ignore both
// QImage::devicePixelRatio() and setDotsPerMeterX/Y() when converting to
// NSImage on macOS -- confirmed empirically: the pasted image always reports
// 72 DPI / a 1:1 pixel-to-point size no matter what, so pasting or printing
// "at 100%" would render roughly 8x too large for a 600 DPI render. This
// bypasses Qt's clipboard bridge entirely via
// `NSPasteboard.general.writeObjects([nsImage])`: the resulting
// NSBitmapImageRep carries every source pixel (so print quality is
// unaffected), but the NSImage's/rep's own `size` is set to the true
// physical dimensions in points, which is what AppKit consumers (Preview,
// the print system) actually read to determine physical size. A PNG
// flavour (public.png) of the same bitmap is put on the pasteboard too,
// and `text`, when not empty, as a plain-text flavour.
//
// Returns false (nothing written) if `source` is empty/invalid -- caller
// should fall back to Qt's own clipboard path in that case.
//
// `pasteboard`: the name of a pasteboard of its own (the tests, so that they
// leave the user's clipboard alone); nullptr: the general pasteboard.
bool macSetClipboardImage(const QImage& source, double widthPt, double heightPt, const QString& text = {},
                          const char* pasteboard = nullptr);
// The size in points and in pixels of the image on that pasteboard (the
// tests); false without one.
bool macPasteboardImageSize(const char* pasteboard, double* widthPt, double* heightPt, int* widthPx, int* heightPx);
} // namespace calcu::qt6
#endif
