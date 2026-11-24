#include "qt6/MacDisplay.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IOKit/graphics/IOGraphicsTypes.h>
#include <QScreen>

namespace calcu::qt6 {

bool macPanelSize(const QScreen& screen, int* width, int* height) {
    // Qt's screen is the NSScreen of its name, size and left edge (Qt keeps
    // the display id to itself; two screens of one kind share the name).
    const QRect g = screen.geometry();
    for (NSScreen* s in [NSScreen screens]) {
        const NSRect f = s.frame;
        if (QString::fromNSString(s.localizedName) != screen.name() || int(f.size.width) != g.width() ||
            int(f.size.height) != g.height() || int(f.origin.x) != g.x())
            continue;
        NSNumber* number = s.deviceDescription[@"NSScreenNumber"];
        if (!number) return false;
        const CGDirectDisplayID id = number.unsignedIntValue;
        // With the low-resolution duplicates: the native mode can be one of them.
        NSDictionary* options = @{(__bridge NSString*)kCGDisplayShowDuplicateLowResolutionModes : @YES};
        CFArrayRef modes = CGDisplayCopyAllDisplayModes(id, (__bridge CFDictionaryRef)options);
        if (!modes) return false;
        bool found = false;
        for (CFIndex i = 0; i < CFArrayGetCount(modes) && !found; ++i) {
            auto mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
            if (CGDisplayModeGetIOFlags(mode) & kDisplayModeNativeFlag) {
                *width = static_cast<int>(CGDisplayModeGetPixelWidth(mode));
                *height = static_cast<int>(CGDisplayModeGetPixelHeight(mode));
                found = true;
            }
        }
        CFRelease(modes);
        return found;
    }
    return false;
}

} // namespace calcu::qt6
