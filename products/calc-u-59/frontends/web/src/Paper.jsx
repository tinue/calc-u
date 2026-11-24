// The PC-100C's pane (Qt6: the splitter's second pane): the
// component set's paper panel (CalcuPaperPanel, library/web/components/),
// which draws the library's `paper PC-100C` from its marks. The panel is
// always there; its owner is the control state's (the
// PC-100C while it is plugged into `printer`, else none). The
// PC-100C is attached and detached by its button at the left of the
// panel's header (the shell's place control, `places`). Its controls are
// the PC-100C's PRINT and ADV (keys) and TRACE (a switch), which the panel
// takes from the control state with their positions and labels.
// Copy and Cut are the panel's: the session's paper.copy / paper.cut through
// the page's clipboard service.

// `paper`: the shell's paper panel ({owner, ownerPlace, hint, places});
// `onChoose`: the shell's choose, for the place controls on the paper;
// `ownWindow`: the paper's own window where the page offers it.
function PaperPane({ shell, paper, onChoose, ownWindow }) {
  return (
    <div className="calcu-printer" data-panel="paper">
      <CalcuPaperPanel shell={shell} owner={paper.owner || null} ownerPlace={paper.ownerPlace || null}
        hint={paper.hint} places={paper.places || []} onChoose={onChoose} ownWindow={ownWindow} />
    </div>
  );
}
