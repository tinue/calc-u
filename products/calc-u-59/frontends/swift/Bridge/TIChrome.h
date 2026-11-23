#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

// Swift face of Calc-U-59's chrome (app/TIChrome: the library's
// calcu::shell::Chrome): the colours and sizes the Qt6 and Web GUIs draw
// with too. Type translation only.

/// An sRGB colour, each channel 0–1.
typedef struct {
    CGFloat red, green, blue, alpha;
} TIChromeColour;

/// The pill buttons' shape (both products'), in points.
typedef struct {
    CGFloat height, radius, paddingX, paddingY;
} TIPillShape;

@interface TIChrome : NSObject
// ── Colours (calcu::shell::ChromeColours; dark only) ───────────────────
@property (class, readonly) TIChromeColour window;
@property (class, readonly) TIChromeColour text;
@property (class, readonly) TIChromeColour textDim;
@property (class, readonly) TIChromeColour header;
@property (class, readonly) TIChromeColour headerText;
@property (class, readonly) TIChromeColour buttonBar;
@property (class, readonly) TIChromeColour pill;
@property (class, readonly) TIChromeColour pillHover;
@property (class, readonly) TIChromeColour pillPressed;
@property (class, readonly) TIChromeColour pillOff;
@property (class, readonly) TIChromeColour pillText;
@property (class, readonly) TIChromeColour output;
@property (class, readonly) TIChromeColour outputText;
@property (class, readonly) TIChromeColour outputBorder;
@property (class, readonly) TIChromeColour plugged;
@property (class, readonly) TIChromeColour unplugged;
/// The product's own colour by name (cu59::chrome::k…, "debug-header");
/// magenta when there is none.
+ (TIChromeColour)named:(NSString *)name;

// ── Sizes (calcu::shell::ChromeSizes), points ────────────────────────────
@property (class, readonly) CGFloat panelHeaderHeight;
@property (class, readonly) CGFloat panelButtonsHeight;
@property (class, readonly) CGFloat panelInset;
@property (class, readonly) TIPillShape pillShape;
@property (class, readonly) CGFloat pillFontSize;
@property (class, readonly) BOOL pillBold;
@property (class, readonly) CGFloat barHeight;
@property (class, readonly) CGFloat barGap;
@property (class, readonly) CGFloat printerMinWidth;
@property (class, readonly) CGFloat printerMaxWidth;
@property (class, readonly) CGFloat debuggerMinWidth;
/// The paper panel's row: the gap between its buttons and at its ends, and
/// the compact row's (TIViewRules.paperRowCompact).
@property (class, readonly) CGFloat paperRowGap;
@property (class, readonly) CGFloat paperRowCompactGap;
/// The bar's and the panels' text scale for a faceplate this wide.
@end

NS_ASSUME_NONNULL_END
