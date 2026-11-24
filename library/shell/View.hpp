#pragma once
#include "shell/MenuModel.hpp"

// The view rule of every GUI of both products: the faceplate is as large as
// the window allows, fitted in both directions with its proportions kept.
// The room left over then lies beside it (the window is relatively wider
// than the faceplate) or below it (relatively taller), and that side picks
// the view: the panels next to the faceplate, or underneath it. Where that
// room is less than the panels' minimum, the panels do not go there and the
// product's fallback applies (Calc-U-59: its pages; Calc-U-1600: the
// calculator alone).
// Every calculator has its natural orientation, its faceplate's (the TI-59
// portrait, the PC-1500/1600 landscape): a window in it always shows the
// whole faceplate, and the panels only take what is left. Only a mobile
// window turned the other way may leave the faceplate out, and only where
// it would be too narrow to use (a phone): then the panels are alone.
// A desktop window changes its view with its size, a mobile one with its
// rotation.

namespace calcu::shell {

enum class ViewSide { Beside, Below };

struct ViewInputs {
    double width = 0, height = 0;   // the room of the client (the window less what is outside it)
    double faceplateAspect = 1;     // the faceplate's width / height
    // What sits around the faceplate whatever the view: the bar, the menu
    // bar and the margins above and below it (fixedHeight), the margins
    // beside it (fixedWidth).
    double fixedWidth = 0, fixedHeight = 0;
    // The least room the panels take beside (a width) and below (a
    // height); 0: the product has no panels on that side.
    double panelsMinWidth = 0, panelsMinHeight = 0;
    bool mobile = false;   // a mobile window (shell::Traits::mobile)
    // The debugger beside the panels (the product's chrome's
    // debuggerMinWidth, debuggerMinHeight): the room more it takes on that
    // side (below: across the window's width) and the least height of the
    // window; 0: the product has none.
    double debuggerMinWidth = 0, debuggerMinHeight = 0;
    // The panels are the debugger alone (the paper in a window of its
    // own): panelsMinWidth/panelsMinHeight are the debugger's, and the
    // debugger fits where the panels do and the window is
    // debuggerMinHeight high.
    bool panelsAreDebugger = false;
    // The least faceplate width whose keys a finger can use (the chrome's
    // faceplateUsableWidth). A mobile window turned against the faceplate's
    // orientation, with the room below taking the panels, shows the panels
    // alone where the faceplate would be narrower (a phone; turned, the
    // calculator has the bar); 0: never.
    double faceplateUsableWidth = 0;
};

struct View {
    ViewSide side = ViewSide::Below;
    bool panelsFit = false;      // the room on that side takes the panels
    bool panelsAlone = false;    // the panels without the faceplate (they fill the window)
    // The debugger beside the panels: the panels fit and the faceplate is
    // shown, the room takes the debugger too and the window is high enough.
    bool debuggerFits = false;
    double faceplateWidth = 0, faceplateHeight = 0;
    double room = 0;             // the room left over on that side (a width beside, a height below)
};

View view(const ViewInputs&);

} // namespace calcu::shell
