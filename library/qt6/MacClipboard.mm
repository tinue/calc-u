#include "qt6/MacClipboard.h"

#import <Cocoa/Cocoa.h>
#include <cstring>

namespace calcu::qt6 {

bool macSetClipboardImage(const QImage& source, double widthPt, double heightPt, const QString& text,
                          const char* pasteboardName) {
    if (source.isNull() || source.width() <= 0 || source.height() <= 0) return false;
    if (widthPt <= 0 || heightPt <= 0) return false;

    // Format_RGBA8888 has a platform-independent byte order (R,G,B,A per
    // pixel) matching NSBitmapImageRep's expected non-premultiplied RGBA
    // layout -- avoids the endianness ambiguity of Format_ARGB32's packed
    // 32-bit value.
    const QImage image = source.convertToFormat(QImage::Format_RGBA8888);
    const int w = image.width();
    const int h = image.height();

    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
        pixelsWide:w
        pixelsHigh:h
        bitsPerSample:8
        samplesPerPixel:4
        hasAlpha:YES
        isPlanar:NO
        colorSpaceName:NSDeviceRGBColorSpace
        bytesPerRow:0
        bitsPerPixel:0];
    if (!rep) return false;

    unsigned char* dst = [rep bitmapData];
    const NSInteger dstStride = [rep bytesPerRow];
    for (int y = 0; y < h; ++y) {
        std::memcpy(dst + y * dstStride, image.constScanLine(y), static_cast<size_t>(w) * 4);
    }
    [rep setSize:NSMakeSize(widthPt, heightPt)];

    NSImage* nsImage = [[NSImage alloc] initWithSize:NSMakeSize(widthPt, heightPt)];
    [nsImage addRepresentation:rep];

    NSPasteboard* pasteboard = pasteboardName ? [NSPasteboard pasteboardWithName:@(pasteboardName)]
                                              : [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    if ([pasteboard writeObjects:@[nsImage]] != YES) return false;
    // Also offer a real PNG (public.png) alongside the NSImage's TIFF, for
    // consumers that only take PNG. The rep's point size gives it the
    // physical resolution (pHYs). Best-effort: the NSImage is already there.
    NSData* png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    if (png) [pasteboard setData:png forType:NSPasteboardTypePNG];
    // A text flavour on the same item: text editors paste it, image
    // consumers still take the image.
    if (!text.isEmpty()) [pasteboard setString:text.toNSString() forType:NSPasteboardTypeString];
    return true;
}

bool macPasteboardImageSize(const char* pasteboardName, double* widthPt, double* heightPt, int* widthPx, int* heightPx) {
    NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName:@(pasteboardName)];
    NSArray* images = [pasteboard readObjectsForClasses:@[ [NSImage class] ] options:@{}];
    if (images.count == 0) return false;
    NSImage* image = images.firstObject;
    NSImageRep* rep = image.representations.firstObject;
    if (!rep) return false;
    *widthPt = image.size.width;
    *heightPt = image.size.height;
    *widthPx = static_cast<int>(rep.pixelsWide);
    *heightPx = static_cast<int>(rep.pixelsHigh);
    [pasteboard releaseGlobally];
    return true;
}

} // namespace calcu::qt6
