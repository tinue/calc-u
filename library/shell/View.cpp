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
    // The natural orientation always shows the faceplate; turned, only a
    // faceplate too narrow to use gives way to the panels.
    const bool turned = (aspect >= 1) != (in.width >= in.height);
    v.panelsAlone = in.mobile && turned && v.side == ViewSide::Below && v.panelsFit &&
                    v.faceplateWidth < in.faceplateUsableWidth;
    const double across = v.side == ViewSide::Beside ? v.room : in.width;
    const bool besideThem = in.panelsAreDebugger ||
                            (in.debuggerMinWidth > 0 && across >= in.panelsMinWidth + in.debuggerMinWidth);
    v.debuggerFits = v.panelsFit && !v.panelsAlone && besideThem && in.height >= in.debuggerMinHeight;
    return v;
}

} // namespace calcu::shell
