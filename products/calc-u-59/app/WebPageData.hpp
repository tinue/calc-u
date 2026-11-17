#pragma once
#include "ModuleCatalog.hpp"
#include "sessions/Value.hpp"
#include <vector>

// The drawing material of the Web page (LIB-14 D-41: product requests of
// TIWebPart under their names): `c59.led-geometry` (the classic LED style's
// shapes for a w × h digit cell) and `c59.mobile-rules` (what the app page
// shows at a size). What 2.3.0's Web bindings had as free functions
// (bindings.cpp ledGeometry, WebController::mobileRules), unchanged in
// content. W59-1's `c59.static-data` went in M-57: the faceplate is
// `c59.faceplate-map`, the heatmap grid `c59.heatmap-grid`.

namespace cu59 {

calcu::sessions::Value webLedGeometry(double w, double h);
calcu::sessions::Value webMobileRules(double w, double h);

} // namespace cu59
