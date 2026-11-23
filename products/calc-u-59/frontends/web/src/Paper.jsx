// The PC-100C's pane (Qt6: the splitter's second pane): the
// component set's paper panel (CalcuPaperPanel, library/web/components/),
// which draws the library's `paper PC-100C` from its marks. The panel is
// always there; its owner is the control state's (the
// PC-100C while it is plugged into `printer`, else none). The
// PC-100C is attached and detached by its button at the left of the
// panel's header (the shell's place control, `places`). Its controls are
// the PC-100C's PRINT and ADV (keys) and TRACE (a switch), with their
// positions and labels, as the control state lists them (`controls`).
// Copy and Cut are the panel's: the session's paper.copy / paper.cut through
// the page's clipboard service.

const NO_CONTROLS = [];
const PRINTER_PLACE = "printer";   // kTIPrinterPlace (core/TICatalogue.hpp)

// `owner`: the paper panel's owner of the control state ("PC-100C" or "");
// `places`, `onChoose`: the shell's place controls on the paper and its choose.
function PaperPane({ shell, owner, places, onChoose }) {
  const all = useTopic(shell, "control-state", (cs) => (cs && cs.controls) || NO_CONTROLS);
  const controls = all.filter((c) => c.place === PRINTER_PLACE)
    .map(({ id, form, on, label, shortLabel }) => ({ id, form, on, label, shortLabel }));
  return (
    <div className="calcu-printer" data-panel="paper">
      <CalcuPaperPanel shell={shell} owner={owner || null} controls={controls} places={places} onChoose={onChoose} />
    </div>
  );
}
