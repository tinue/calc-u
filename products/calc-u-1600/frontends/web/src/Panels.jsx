// The product's panels in the shell's panel area, as in the desktop app: the INSPECTOR (Qt6/app/DebugPanel, without
// TRACE/LOG) always, and the CE-158 printer output (Ce158PrinterWidget)
// while the product entry `c1600.ce158-printer-panel` says so, in the same
// chrome: a header strip, the output, a button bar with pill buttons. The
// plotters' paper is the library's paper panel (CalcuPaperPanel),
// which the shell's area draws from the control state's panel entry.

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

// `shell`: the React set's shell; the views are the product's requests
// `c1600.inspector-menu`, `c1600.inspector-view`, `c1600.module-yaml`.
function Inspector({ shell }) {
  const [entries, setEntries] = React.useState([]);
  const outRef = React.useRef(null);
  const add = (chunks) => setEntries((e) => e.concat(chunks).slice(-INSPECTOR_MAX_ENTRIES));
  const ask = (name, args) => (shell ? shell.request(name, args) : Promise.resolve({ value: null })).then((r) => r.value);

  React.useEffect(() => {
    const el = outRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [entries]);

  const showView = async (view) => {
    const text = await ask("c1600.inspector-view", { view });
    if (text) add([text]);
  };
  const menu = (dumps) => async () => {
    const list = (await ask("c1600.inspector-menu", { dumps })) || [];
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
          <button type="button" className="calcu-pill" title="The module in each slot, as initial-content: YAML for a module definition"
            onClick={async () => add((await ask("c1600.module-yaml", null)) || [])}>Module YAML</button>
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

// The library's paper panel in the product's chrome: always
// there, named after the plotter the control state's panel entry names;
// `ownWindow`: the paper's own window where the page offers it.
function PaperDock({ shell, owner, hint, ownWindow }) {
  const papers = useCalcuPaper(shell);
  const entry = owner ? papers.papers.find((p) => p.printer === owner) : null;
  const empty = !entry || papers.paper(entry.owner).empty;
  return (
    <section className="calcu-panel calcu-paper" data-panel="paper" data-owner={owner || ""}
      data-empty={empty ? "true" : "false"}>
      <CalcuPaperPanel shell={shell} owner={owner || null} hint={hint} ownWindow={ownWindow} />
    </section>
  );
}

// The CE-158's printer output: the stream topic `c1600.printer` after the
// last item cleared (a marker, not a count: the stream keeps only its last
// items and starts anew on a reset).
function Ce158Printer({ shell }) {
  const printed = useStream(shell, "c1600.printer");
  const [lastCleared, setLastCleared] = React.useState(null);
  const items = calcuStreamAfter(printed, lastCleared);
  const text = items.map((p) => p.text).join("");
  const outRef = React.useRef(null);
  React.useLayoutEffect(() => {
    const el = outRef.current;
    if (el && el.scrollHeight - el.clientHeight - el.scrollTop <= 40) el.scrollTop = el.scrollHeight;
  }, [text]);
  const empty = items.length === 0;
  return (
    <Panel title="CE-158" className="calcu-printer" panel="printer" empty={empty}
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-pill" disabled={empty} title="Save the printed bytes, exactly as sent, to a file."
            onClick={() => downloadBytes("ce158-printout.txt", new Blob(items.map((p) => p.bytes)))}>Save…</button>
          <button type="button" className="calcu-pill" disabled={empty} onClick={() => setLastCleared(items[items.length - 1])}>Clear</button>
        </React.Fragment>
      }>
      <div className="calcu-printer-serial">RS-232C: not available in the browser</div>
      <pre className="calcu-printer-output" ref={outRef}>{text}</pre>
    </Panel>
  );
}

// The pill menus' list and their dismissal.
function MenuList({ items, onDone, className }) {
  const [open, setOpen] = React.useState(-1);
  return (
    <div className={`calcu-menu ${className || ""}`} role="menu">
      {items.filter(Boolean).map((item, i) => {
        if (item.separator) return <div key={i} className="calcu-menu-separator" role="separator" />;
        if (item.heading) return <div key={i} className="calcu-menu-heading">{item.heading}</div>;
        const hasSub = !!item.submenu;
        return (
          <div key={i} className={`calcu-menu-item${item.disabled ? " disabled" : ""}${open === i ? " open" : ""}`}
            role={item.checked === undefined ? "menuitem" : "menuitemcheckbox"}
            aria-checked={item.checked === undefined ? undefined : !!item.checked}
            aria-disabled={!!item.disabled}
            onPointerEnter={() => setOpen(hasSub ? i : -1)}
            onClick={(e) => {
              e.stopPropagation();
              if (item.disabled) return;
              if (hasSub) { setOpen(i); return; }
              onDone();
              if (item.onClick) item.onClick();
            }}>
            <span className="calcu-menu-check">{item.checked ? "✓" : ""}</span>
            <span className="calcu-menu-label">{item.label}</span>
            <span className="calcu-menu-shortcut">{hasSub ? "▸" : item.shortcut || ""}</span>
            {hasSub && open === i && !item.disabled && <MenuList items={item.submenu} onDone={onDone} className="calcu-submenu" />}
          </div>
        );
      })}
    </div>
  );
}

// While `active`: a pointer down outside `ref`'s element or Escape calls
// `close` (an open menu, the menu bar's or a pill's).
function useDismiss(ref, active, close) {
  React.useEffect(() => {
    if (!active) return undefined;
    const outside = (e) => { if (!ref.current || !ref.current.contains(e.target)) close(); };
    const escape = (e) => { if (e.key === "Escape") close(); };
    document.addEventListener("pointerdown", outside, true);
    document.addEventListener("keydown", escape, true);
    return () => {
      document.removeEventListener("pointerdown", outside, true);
      document.removeEventListener("keydown", escape, true);
    };
  }, [active]);
}
