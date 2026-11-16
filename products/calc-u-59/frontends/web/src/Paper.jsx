// The PC-100C's pane (LIB-8 59, M-53; Qt6: the splitter's second pane): the
// component set's paper panel (CalcuPaperPanel, library/web/components/),
// which draws the library's `paper PC-100C` from its marks, with the
// PC-100C's connect toggle above it (adapter P-E1, until the bar has the
// printer's control, M-57). The panel's owner is the PC-100C while it is
// connected (c59.web.printer); its controls are the PC-100C's PRINT and ADV
// (keys) and TRACE (a switch, its position from c59.web.printer). Copy and
// Cut are the panel's: the session's paper.copy / paper.cut through the
// page's clipboard service.

const PC100C_CONTROLS = (trace) => [
  { id: "PRINT", form: "key", on: false },
  { id: "ADV", form: "key", on: false },
  { id: "TRACE", form: "switch", on: !!trace },
];

function PaperPane({ engine }) {
  const printer = useEngine(engine, (s) => s.printer);
  const shell = engine ? engine.shell : null;
  const dot = (on) => (on ? "●" : "○");
  return (
    <div className="calcu-printer" data-panel="paper">
      <div className="calcu-printer-header">
        <span />
        <button type="button" className="calcu-pill" aria-pressed={printer.connected}
          title="Connect or disconnect the PC-100C (resets the calculator)"
          onClick={() => engine && engine.call("togglePrinterConnection")}>
          PC-100C <span className={printer.connected ? "calcu-dot-on" : ""}>{dot(printer.connected)}</span>
        </button>
        <span />
      </div>
      <CalcuPaperPanel shell={shell} owner={printer.connected ? "PC-100C" : null}
        controls={PC100C_CONTROLS(printer.trace)} />
    </div>
  );
}
