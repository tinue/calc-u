#pragma once
#include "shell/MenuModel.hpp"

// The view rule of every GUI of both products: the faceplate is as large as
// the window allows, fitted in both directions with its proportions kept.
// The room left over then lies beside it (the window is relatively wider
// than the faceplate) or below it (relatively taller), and that side picks
// the view: the panels next to the faceplate, or underneath it. Where that
// room is less than the panels' minimum, the panels do not go there and the
// product's fallback applies (Calc-U-59: its pages; Calc-U-1600: the
// calculator alone, or on mobile with the room below the panels alone).
// A desktop window changes its view with its size, a mobile one with its
// rotation; only the panels-alone case is mobile's alone.

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
    // On mobile where the room below takes the panels: the panels alone,
    // without the faceplate (a phone in portrait, whose keys would be too
    // small). A sliver of room below (a phone in landscape whose safe
    // area makes the window a little narrower) keeps the calculator.
    bool panelsAloneOnMobileBelow = false;
};

struct View {
    ViewSide side = ViewSide::Below;
    bool panelsFit = false;      // the room on that side takes the panels
    bool panelsAlone = false;    // the panels without the faceplate (they fill the window)
    double faceplateWidth = 0, faceplateHeight = 0;
    double room = 0;             // the room left over on that side (a width beside, a height below)
};

View view(const ViewInputs&);

} // namespace calcu::shell
