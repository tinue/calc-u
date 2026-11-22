#import "TIChrome.h"
#import "BridgeConvert.h"
#include <calcu59/app/TIChrome.hpp>

static const calcu::shell::ChromeColours& colours() { return cu59::tiChrome().colours(true); }
static const calcu::shell::ChromeSizes& sizes() { return cu59::tiChrome().sizes; }

static TIChromeColour ti(const calcu::shell::Colour& c) {
    return TIChromeColour{c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0};
}

@implementation TIChrome
+ (TIChromeColour)window { return ti(colours().window); }
+ (TIChromeColour)text { return ti(colours().text); }
+ (TIChromeColour)textDim { return ti(colours().textDim); }
+ (TIChromeColour)header { return ti(colours().header); }
+ (TIChromeColour)headerText { return ti(colours().headerText); }
+ (TIChromeColour)buttonBar { return ti(colours().buttonBar); }
+ (TIChromeColour)pill { return ti(colours().pill); }
+ (TIChromeColour)pillHover { return ti(colours().pillHover); }
+ (TIChromeColour)pillPressed { return ti(colours().pillPressed); }
+ (TIChromeColour)pillOff { return ti(colours().pillOff); }
+ (TIChromeColour)pillText { return ti(colours().pillText); }
+ (TIChromeColour)pillTextOff { return ti(colours().pillTextOff); }
+ (TIChromeColour)output { return ti(colours().output); }
+ (TIChromeColour)outputText { return ti(colours().outputText); }
+ (TIChromeColour)outputBorder { return ti(colours().outputBorder); }
+ (TIChromeColour)plugged { return ti(colours().plugged); }
+ (TIChromeColour)unplugged { return ti(colours().unplugged); }
+ (TIChromeColour)named:(NSString *)name { return ti(colours().named(cpp(name))); }

+ (CGFloat)panelHeaderHeight { return sizes().panelHeaderHeight; }
+ (CGFloat)panelButtonsHeight { return sizes().panelButtonsHeight; }
+ (CGFloat)panelInset { return sizes().panelInset; }
+ (TIPillShape)pillShape {
    const auto& p = sizes().pill;
    return TIPillShape{p.height, p.radius, p.paddingX, p.paddingY};
}
+ (CGFloat)pillFontSize { return sizes().pillFontSize; }
+ (BOOL)pillBold { return sizes().pillBold; }
+ (CGFloat)barHeight { return sizes().barHeight; }
+ (CGFloat)barGap { return sizes().barGap; }
+ (CGFloat)printerMinWidth { return sizes().printerMinWidth; }
+ (CGFloat)printerMaxWidth { return sizes().printerMaxWidth; }
+ (CGFloat)textScale:(CGFloat)faceplateWidth { return cu59::tiChrome().textScale(faceplateWidth); }
@end
