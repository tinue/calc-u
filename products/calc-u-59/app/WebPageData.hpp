#pragma once
#include "ModuleCatalog.hpp"
#include "sessions/Value.hpp"
#include <vector>

// Adapter W59-1 (LIB-15 design §5; removed in M-57, LIB-14 59): the static
// data the Web page draws with, in the page's own form, answered by the
// product requests of TIWebPart: `c59.static-data` (models, modules, the
// canvas layout, the heatmap's grid and
// colours), `c59.led-geometry` (the classic LED style's shapes for a w × h
// digit cell) and `c59.mobile-rules` (what the app page shows at a size).
// What 2.3.0's Web bindings had as free functions (bindings.cpp staticData,
// ledGeometry, WebController::mobileRules), unchanged in content.

namespace cu59 {

calcu::sessions::Value webStaticData(const std::vector<ModuleMetadata>& modules);
calcu::sessions::Value webLedGeometry(double w, double h);
calcu::sessions::Value webMobileRules(double w, double h);

} // namespace cu59
