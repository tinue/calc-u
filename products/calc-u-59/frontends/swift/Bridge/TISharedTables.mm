#import "TISharedTables.h"
#include <calcu59/app/KeyIds.hpp>
#include <calcu59/app/KeyboardLayout.hpp>
#include <calcu59/app/TIShellProduct.hpp>
#include <calcu59/app/SevenSegment.hpp>

namespace {
CGRect toCG(const cu59::NRect& r) { return CGRectMake(r.x, r.y, r.w, r.h); }
}

@implementation TISharedTables

+ (uint8_t)ledCellSegmentsAt:(NSInteger)index glyph:(uint8_t)glyph level:(uint8_t)level
                 calcOpacity:(double)calcOpacity opacity:(double *)opacity {
    cu59::LedCell cell = cu59::ledCell(static_cast<int>(index), glyph, level, calcOpacity);
    *opacity = cell.opacity;
    return cell.segments;
}

+ (CGFloat)canvasAspect   { return cu59::kCanvasAspect; }
+ (CGRect)displayRect     { return toCG(cu59::kDisplayRect); }
+ (CGRect)cardRect        { return toCG(cu59::kCardRect); }
+ (CGFloat)keyboardYStart { return cu59::kKeyboardYStart; }
+ (CGFloat)keyboardYScale { return cu59::kKeyboardYScale; }

+ (CGFloat)canvasWidthPixels { return cu59::kCanvasWidth; }

+ (CGRect)keyRectAtRow:(NSInteger)row col:(NSInteger)col {
    if (row < 0 || row >= 9 || col < 0 || col >= 5) return CGRectZero;
    return toCG(cu59::kKeyRects[row][col]);
}

+ (uint8_t)matrixCodeAtRow:(NSInteger)row col:(NSInteger)col {
    return cu59::matrixCode(static_cast<int>(row), static_cast<int>(col));
}



@end
