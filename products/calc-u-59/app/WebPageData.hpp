#pragma once
#include "ModuleCatalog.hpp"
#include "sessions/Value.hpp"
#include <vector>

// The drawing material of the Web page (product requests of TIWebPart under
// their names): `c59.led-geometry` (the classic LED style's shapes for a
// w × h digit cell) and `c59.mobile-rules` (what the app page shows at a
// size). The faceplate is `c59.faceplate-map`, the heatmap grid
// `c59.heatmap-grid` (both in TIWebPart).

namespace cu59 {

calcu::sessions::Value webLedGeometry(double w, double h);
calcu::sessions::Value webMobileRules(double w, double h);

} // namespace cu59
