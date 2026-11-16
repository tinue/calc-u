// The docked panels under the control bar, as in the desktop app: the
// INSPECTOR (Qt6/app/DebugPanel, without TRACE/LOG) and the CE-158 printer
// output (Ce158PrinterWidget), in the same chrome: a header strip, the
// output, a button bar with pill buttons, light or dark with the system.
// The plotters' paper is the library's paper panel (CalcuPaperPanel, LIB-8),
// always shown (AC-36), docked by PaperDock below.

function Panel({ title, className, children, buttons, panel, empty }) {
  return (
    <section className={`calcu-panel ${className || ""}`} data-panel={panel} data-empty={empty ? "true" : "false"}>
      <header className="calcu-panel-header">{title}</header>
      <div className="calcu-panel-body">{children}</div>
      <div className="calcu-panel-buttons">{buttons}</div>
    </section>
  );
}

// A pill button with a drop-down menu; `entries()` resolves to its
// MenuList items ({label, disabled, onClick}).
function PillMenu({ label, entries }) {
  const [open, setOpen] = React.useState(false);
  const [items, setItems] = React.useState([]);
  const ref = React.useRef(null);
  useDismiss(ref, open, () => setOpen(false));
  return (
    <span className="calcu-pillmenu" ref={ref}>
      <button type="button" className="calcu-pill" aria-haspopup="true" aria-expanded={open}
        onClick={async () => {
          if (open) { setOpen(false); return; }
          setItems(await entries());
          setOpen(true);
        }}>
        {label} <span className="calcu-pill-arrow">▾</span>
      </button>
      {open && <MenuList items={items} onDone={() => setOpen(false)} className="calcu-pillmenu-list" />}
    </span>
  );
}

const INSPECTOR_MAX_ENTRIES = 200;

function Inspector({ engine }) {
  const [entries, setEntries] = React.useState([]);
  const outRef = React.useRef(null);
  const add = (chunks) => setEntries((e) => e.concat(chunks).slice(-INSPECTOR_MAX_ENTRIES));

  React.useEffect(() => {
    const el = outRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [entries]);

  const showView = async (view) => {
    const text = await engine.request("inspectorView", view);
    if (text) add([text]);
  };
  const menu = (dumps) => async () => {
    const list = await engine.request("inspectorMenu", dumps);
    return list.map((e) => ({ label: e.title, disabled: !e.enabled, onClick: () => showView(e.view) }));
  };

  return (
    <Panel title="INSPECTOR" className="calcu-inspector" panel="inspector"
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-pill" onClick={() => showView(0)}>Pointers</button>
          <PillMenu label="Memory" entries={menu(false)} />
          <PillMenu label="Dump" entries={menu(true)} />
          <span className="calcu-bar-stretch" />
          <button type="button" className="calcu-pill" title="The module in each slot, as initial-content: YAML for a card file"
            onClick={async () => add(await engine.request("cardYaml"))}>Card YAML</button>
          <button type="button" className="calcu-pill calcu-pill-icon" title="Clear" aria-label="Clear"
            onClick={() => setEntries([])}>
            <svg viewBox="0 0 16 16" aria-hidden="true"><path d="M3 4h10l-1 10H4L3 4zm2-2h6v1.5H5z" fill="currentColor" opacity="0.6" /></svg>
          </button>
        </React.Fragment>
      }>
      <pre className="calcu-inspector-output" ref={outRef}>{entries.join("\n")}</pre>
    </Panel>
  );
}

// The library's paper panel (LIB-8 AC-36): always there, named after the
// plotter plugged in (ui.paper, adapter P-A1 until LIB-4's panel entry).
function PaperDock({ engine }) {
  const owner = useEngine(engine, (s) => (s.ui && s.ui.paper ? s.ui.paper : null));
  const papers = useCalcuPaper(engine ? engine.shell : null);
  const entry = owner ? papers.papers.find((p) => p.printer === owner) : null;
  const empty = !entry || papers.paper(entry.owner).empty;
  return (
    <section className="calcu-panel calcu-paper" data-panel="paper" data-owner={owner || ""}
      data-empty={empty ? "true" : "false"}>
      <CalcuPaperPanel shell={engine ? engine.shell : null} owner={owner} />
    </section>
  );
}

function Ce158Printer({ engine }) {
  const printer = useEngine(engine, (s) => s.printer);
  const outRef = React.useRef(null);
  React.useLayoutEffect(() => {
    const el = outRef.current;
    if (el && el.scrollHeight - el.clientHeight - el.scrollTop <= 40) el.scrollTop = el.scrollHeight;
  }, [printer.text]);
  const empty = printer.chunks.length === 0;
  return (
    <Panel title="CE-158" className="calcu-printer" panel="printer" empty={empty}
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-pill" disabled={empty} title="Save the printed bytes, exactly as sent, to a file."
            onClick={() => downloadBytes("ce158-printout.txt", new Blob(printer.chunks))}>Save…</button>
          <button type="button" className="calcu-pill" disabled={empty} onClick={() => engine.clearPrinter()}>Clear</button>
        </React.Fragment>
      }>
      <div className="calcu-printer-serial">RS-232C: not available in the browser</div>
      <pre className="calcu-printer-output" ref={outRef}>{printer.text}</pre>
    </Panel>
  );
}
