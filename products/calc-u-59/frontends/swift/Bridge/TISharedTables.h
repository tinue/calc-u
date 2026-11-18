#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

/// Swift face of the app/ data tables (7-segment
/// patterns, canvas/key layout, matrix codes; the paper comes as dots from
/// the core). Type
/// translation only — the tables and rules live in app/.
@interface TISharedTables : NSObject

// ── LED display (app/SevenSegment.hpp) ───────────────────────────────────
/// What cell `index` (0 = rightmost) shows: segments and opacity 0–1 (cu59::ledCell).
+ (uint8_t)ledCellSegmentsAt:(NSInteger)index glyph:(uint8_t)glyph level:(uint8_t)level
                 calcOpacity:(double)calcOpacity opacity:(double *)opacity
    NS_SWIFT_NAME(ledCell(index:glyph:level:calcOpacity:opacity:));

// ── Canvas / key layout (app/KeyboardLayout.hpp) ─────────────────────────
@property (class, readonly) CGFloat canvasAspect;
/// The base image's width in pixels: the unit of the faceplate's hit map (cu59::kCanvasWidth).
@property (class, readonly) CGFloat canvasWidthPixels;
@property (class, readonly) CGRect displayRect;
@property (class, readonly) CGRect cardRect;
@property (class, readonly) CGFloat keyboardYStart;
@property (class, readonly) CGFloat keyboardYScale;
/// Key rect in keyboard-image space; row 0–8, col 0–4.
+ (CGRect)keyRectAtRow:(NSInteger)row col:(NSInteger)col NS_SWIFT_NAME(keyRect(row:col:));
/// Matrix code (state-file key code) of the key at row 0–8, col 0–4.
+ (uint8_t)matrixCodeAtRow:(NSInteger)row col:(NSInteger)col NS_SWIFT_NAME(matrixCode(row:col:));

@end

NS_ASSUME_NONNULL_END
