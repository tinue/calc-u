#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

/// Swift face of the Shared/ data tables (7-segment
/// patterns, printer font, canvas/key layout, matrix codes). Type
/// translation only — the tables and rules live in Shared/.
@interface TISharedTables : NSObject

// ── LED display (Shared/SevenSegment.hpp) ───────────────────────────────────
/// What cell `index` (0 = rightmost) shows: segments and opacity 0–1 (cu59::ledCell).
+ (uint8_t)ledCellSegmentsAt:(NSInteger)index glyph:(uint8_t)glyph level:(uint8_t)level
                 calcOpacity:(double)calcOpacity opacity:(double *)opacity
    NS_SWIFT_NAME(ledCell(index:glyph:level:calcOpacity:opacity:));

// ── PC-100C printer font (Shared/PrinterFont.hpp) ───────────────────────────
/// Row 0–6 of a 6-bit character code: bit 4 = leftmost column.
+ (uint8_t)printerFontRowForCode:(uint8_t)code row:(NSInteger)row;

// ── Canvas / key layout (Shared/KeyboardLayout.hpp) ─────────────────────────
@property (class, readonly) CGFloat canvasAspect;
@property (class, readonly) CGRect displayRect;
@property (class, readonly) CGRect cardRect;
@property (class, readonly) CGFloat keyboardYStart;
@property (class, readonly) CGFloat keyboardYScale;
/// Key rect in keyboard-image space; row 0–8, col 0–4.
+ (CGRect)keyRectAtRow:(NSInteger)row col:(NSInteger)col NS_SWIFT_NAME(keyRect(row:col:));
/// Matrix code (state-file key code) of the key at row 0–8, col 0–4.
+ (uint8_t)matrixCodeAtRow:(NSInteger)row col:(NSInteger)col NS_SWIFT_NAME(matrixCode(row:col:));
/// row×5 + col of the key whose widened hit rect contains the point, or -1.
+ (NSInteger)keyIndexAtX:(CGFloat)nx y:(CGFloat)ny NS_SWIFT_NAME(keyIndex(x:y:));

@end

NS_ASSUME_NONNULL_END
