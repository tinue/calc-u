// The PC-100C's pane (LIB-8 59, M-53; Qt6: the splitter's second pane): the
// component set's paper panel (CalcuPaperPanel, library/web/components/),
// which draws the library's `paper PC-100C` from its marks. The panel is
// always there (LIB-4 AC-13); its owner is the control state's (the
// PC-100C while it is plugged into `printer`, else none). Since M-57 the
// PC-100C is attached and detached by the button "PC-100C" on the shell's
// control bar (LIB-14 AC-27), not by a toggle in the pane. Its controls are
// the PC-100C's PRINT and ADV (keys) and TRACE (a switch, its position from
// c59.printer). Copy and Cut are the panel's: the session's paper.copy /
// paper.cut through the page's clipboard service.

const PC100C_CONTROLS = (trace) => [
  { id: "PRINT", form: "key", on: false },
  { id: "ADV", form: "key", on: false },
  { id: "TRACE", form: "switch", on: !!trace },
];

// `owner`: the paper panel's owner of the control state ("PC-100C" or "").
function PaperPane({ shell, owner }) {
  const printer = useTopic(shell, "c59.printer");
  return (
    <div className="calcu-printer" data-panel="paper">
      <CalcuPaperPanel shell={shell} owner={owner || null} controls={PC100C_CONTROLS(printer && printer.trace)} />
    </div>
  );
}
