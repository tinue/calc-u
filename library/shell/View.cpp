#include "shell/View.hpp"
#include <algorithm>

namespace calcu::shell {

View view(const ViewInputs& in) {
    View v;
    const double boxWidth = std::max(0.0, in.width - in.fixedWidth);
    const double boxHeight = std::max(0.0, in.height - in.fixedHeight);
    const double aspect = in.faceplateAspect > 0 ? in.faceplateAspect : 1;
    v.faceplateWidth = std::min(boxWidth, boxHeight * aspect);
    v.faceplateHeight = v.faceplateWidth / aspect;
    // Relatively wider than the faceplate: the room is beside it.
    v.side = boxWidth > boxHeight * aspect ? ViewSide::Beside : ViewSide::Below;
    v.room = v.side == ViewSide::Beside ? boxWidth - v.faceplateWidth : boxHeight - v.faceplateHeight;
    const double least = v.side == ViewSide::Beside ? in.panelsMinWidth : in.panelsMinHeight;
    v.panelsFit = least > 0 && v.room >= least;
    v.panelsAlone = in.panelsAloneOnMobileBelow && in.platform == Platform::Mobile && v.side == ViewSide::Below;
    return v;
}

} // namespace calcu::shell
