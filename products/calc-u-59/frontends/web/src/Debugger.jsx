// The debugger panel, wide layout only (Qt6/app/debug/, App/Views/DebugView):
// tabs CALCULATOR / CPU / LOG. Only the visible tab's snapshots are
// built (setLiveDebugEnabled / setCpuDebugEnabled follow it). It draws the
// product topics of the debug part (app/TIDebugPart): c59.calculator-debug,
// c59.program-listing, c59.cpu-debug, c59.heatmap, c59.asm-overlay and c59.debug-view, whose
// values are already the rows formatted by app/DebugText; the LOG tab is
// the session's `log` at its `log-level`. FREEZE, STEP and RESUME of each tab are the component set's
// run-control buttons for the level `program` (CALCULATOR) and `cpu` (CPU),
// on the library's `run-state`, which FREEZE ON START
// and ARMED read too; TRACE is the trace
// session's button, which can always be pressed (a trace folder that cannot
// be written refuses the start). Run and the CALCULATOR tab
// follow the control state's product entries c59.asm-run and
// c59.calculator-tab.
// It reads the topics through the library's shell (useTopic) and
// sends the commands by name (the page's context C59_PAGE, src/page.jsx).

const NO_ASM = { fileName: "", words: 0, status: "", active: false };
const NO_TRACE = { running: false, fileName: "" };
const usePage = () => React.useContext(C59_PAGE);
// The library's run-state as the tabs read it: frozen, and armed per level.
const useRun = (shell) => {
  const r = useTopic(shell, "run-state");
  return { frozen: !!(r && r.paused), pcArmed: !!r && r.armed === "program", scanArmed: !!r && r.armed === "cpu" };
};
const useEntries = (shell) => useTopic(shell, "control-state", (cs) => (cs && cs.product) || null);


// A chrome colour (app/TIChrome, the client's custom properties) at a share
// of its strength; grey(w): the text's colour at w.
const tint = (name, share) => `color-mix(in srgb, var(--calcu-${name}) ${Math.round(share * 100)}%, transparent)`;
const grey = (white) => tint("text", white);

function Section({ title, extra, children }) {
  return (
    <div className="calcu-section">
      <div className="calcu-section-head"><span>{title}</span>{extra}</div>
      {children !== undefined && <div className="calcu-section-body">{children}</div>}
    </div>
  );
}

const Run = ({ white = 0.85, color, children }) => <span style={{ color: color || grey(white) }}>{children}</span>;
const Gap = () => <span>{"   "}</span>;

// The debug tabs' headers as app/ describes them for every GUI
// (`c59.debug-headers`: title, buttons left to right with their labels and
// colours, the narrow rule); asked once per page.
let debugHeaders = null;
function useDebugHeaders(shell) {
  const [h, setH] = React.useState(debugHeaders);
  React.useEffect(() => {
    if (h || !shell) return;
    shell.request("c59.debug-headers", {}).then((r) => {
      if (r.done) { debugHeaders = r.value; setH(r.value); }
    });
  }, [shell]);
  return h;
}
// app/'s button IDs → the library's run-control buttons.
const RUN_CONTROL_OF = { resume: "resume", freeze: "pause", step: "step" };

/// A product entry of the control state: usable unless it says not.
const usable = (entries, name) => !entries || !entries[name] || entries[name].available !== false;

/// FREEZE / STEP / RESUME (the run-control buttons of the tab's level) and
/// A tab's header as app/'s debugHeader describes it (the order, the labels
/// and the colours as CSS variables, for the library's run-control buttons
/// too), at the panel's base font size (`size`).
function FreezeHeader({ cpu, size }) {
  const { shell } = usePage();
  const rs = useRun(shell);
  const all = useDebugHeaders(shell);
  const h = all && (cpu ? all.cpu : all.calculator);
  const narrow = !!all && size < all.narrowBelow;
  const look = { "--calcu-hb-size": `${size}px` };
  const labels = {};
  const text = {};
  (h ? h.buttons : []).forEach((b, i) => {
    const name = RUN_CONTROL_OF[b.id] || b.id;
    look[`--calcu-hb-order-${name}`] = i + 1;
    look[`--calcu-hb-colour-${name}`] = b.colour;
    text[name] = narrow ? b.narrowLabel : b.label;
    if (RUN_CONTROL_OF[b.id]) labels[name] = text[name];
  });
  const level = cpu ? "cpu" : "program";
  const armed = cpu ? rs.scanArmed : rs.pcArmed;
  const canArm = !rs.frozen && !armed;
  return (
    <div className="calcu-debug-header" style={look}>
      <span className="calcu-debug-title">{h ? (narrow ? h.narrowTitle : h.title) : ""}</span>
      <CalcuRunControlButtons shell={shell} level={level} labels={labels} />
      <button type="button" className="calcu-hb" data-calcu="freeze-on-start" disabled={!canArm}
        onClick={() => shell.send("pause-on-start", { level })}>{text["freeze-on-start"]}</button>
      <button type="button" className="calcu-hb" data-calcu="freeze-armed" disabled={!armed}
        onClick={() => shell.send("pause-on-start", { off: true })}>{text.armed}</button>
    </div>
  );
}

/// A list that keeps one row in view (centred, or at the bottom while running).
function useScrollTo(ref, index, mode, deps) {
  React.useLayoutEffect(() => {
    const list = ref.current;
    if (!list || index < 0 || !list.children[index]) return;
    const row = list.children[index];
    if (mode === "bottom") list.scrollTop = list.scrollHeight;
    else list.scrollTop = row.offsetTop - list.clientHeight / 2 + row.offsetHeight / 2;
  }, deps);
}

function LiveDebug({ font }) {
  const { shell } = usePage();
  const s = useTopic(shell, "c59.calculator-debug") || null;
  const listing = useTopic(shell, "c59.program-listing") || null;
  const frozen = useRun(shell).frozen;
  // Paused: the whole listing of the program that runs; else the window.
  const full = frozen && listing && listing.steps.length > 0;
  const steps = s ? (full ? listing.steps : s.steps) : [];
  const highlight = full ? listing.stepHighlight : s && s.stepHighlight;
  const stepsRef = React.useRef(null);
  const current = steps.findIndex((r) => r.current);
  useScrollTo(stepsRef, current, "center", [s, listing, full]);
  const dim = (on) => (on ? 0.85 : 0.4);
  return (
    <div className="calcu-debug-tab" style={{ fontSize: font }}>
      <FreezeHeader size={font - 2} />
      {s && <>
        <div className="calcu-debug-fixed">
          <Section title="DEBUG"><Run white={0.65}>{s.fA}</Run><Gap /><Run white={0.65}>{s.fB}</Run><Gap /><Run>{s.hir0}</Run></Section>
        </div>
        <div className="calcu-debug-scroll">
          <Section title="PARTITION"><Run white={0.75}>{s.partition}</Run></Section>
          <Section title={s.source} />
          <Section title="PROGRAM STEPS">
            <div className="calcu-steps" ref={stepsRef}>
              {steps.map((r, i) => (
                <div key={i} style={r.current ? { background: highlight, color: "var(--calcu-text)" } : { color: grey(0.6) }}>{r.text}</div>
              ))}
            </div>
          </Section>
          <Section title="">
            <Run>{s.fix}</Run><Gap /><Run white={s.angleKnown ? 0.85 : 0.4}>{s.angle}</Run><Gap />
            <Run white={dim(s.eng)}>Eng</Run><Gap /><Run white={dim(s.second)}>2nd</Run><Gap /><Run white={dim(s.inv)}>INV</Run>
          </Section>
          <Section title="RETURN ADDRESS">
            {[5, 4, 3].map((i) => <React.Fragment key={i}><Run color={s.returns[i].color}>{s.returns[i].text}</Run><Gap /></React.Fragment>)}
            <br />
            {[2, 1, 0].map((i) => <React.Fragment key={i}><Run color={s.returns[i].color}>{s.returns[i].text}</Run><Gap /></React.Fragment>)}
          </Section>
          <Section title="FLAGS">
            {s.flags.map((on, i) => (
              <React.Fragment key={i}><Run white={on ? 0.85 : 0.6}>{`F${i}:${on ? 1 : 0}`}</Run>{i === 4 ? <br /> : <Gap />}</React.Fragment>
            ))}
          </Section>
          <Section title="HIR">
            {s.hir.map((h, i) => (
              <React.Fragment key={i}><Run white={h.zero ? 0.6 : 0.85}>{h.text}</Run>{i === 3 ? <br /> : <Gap />}</React.Fragment>
            ))}
          </Section>
          <Section title=""><Run color={s.xColor}>{s.x}</Run><Gap /><Run white={s.tZero ? 0.6 : 0.85}>{s.t}</Run></Section>
          <Section title="VARIABLES">
            {s.vars.length === 0
              ? <Run white={0.35}>(all zero)</Run>
              : s.vars.map((v, i) => <div key={i}><Run>{v}</Run></div>)}
          </Section>
        </div>
      </>}
    </div>
  );
}

function Heatmap({ data, currentPC }) {
  const { shell } = usePage();
  const steps = useTopic(shell, "c59.heatmap") || null;
  const ref = React.useRef(null);
  const [hover, setHover] = React.useState(-1);
  const { cols, rows, colors } = data;

  React.useEffect(() => {
    const canvas = ref.current;
    canvas.width = cols;
    canvas.height = rows;
    const ctx = canvas.getContext("2d");
    // The PC's cell in the chrome's heatmap-pc ("#rrggbb", TIChrome).
    const hex = getComputedStyle(canvas).getPropertyValue("--calcu-heatmap-pc").trim();
    const pcRgb = /^#[0-9a-f]{6}$/i.test(hex) ? [1, 3, 5].map((k) => parseInt(hex.slice(k, k + 2), 16)) : [0, 0, 0];
    const img = ctx.createImageData(cols, rows);
    const d = img.data;
    for (let i = 0; i < cols * rows; i++) {
      const c = currentPC === i ? pcRgb : colors[steps ? steps[i] : 0];
      d[i * 4] = c[0];
      d[i * 4 + 1] = c[1];
      d[i * 4 + 2] = c[2];
      d[i * 4 + 3] = 255;
    }
    ctx.putImageData(img, 0, 0);
  }, [steps, currentPC]);

  const onMove = (e) => {
    const r = e.currentTarget.getBoundingClientRect();
    const x = Math.floor(((e.clientX - r.left) / r.width) * cols), y = Math.floor(((e.clientY - r.top) / r.height) * rows);
    setHover(x >= 0 && x < cols && y >= 0 && y < rows ? y * cols + x : -1);
  };
  const hex = (a) => "0x" + a.toString(16).toUpperCase().padStart(4, "0");
  return (
    <Section title="ROM HEATMAP" extra={<span className="calcu-section-extra">
      <span className="calcu-hover">{hover >= 0 ? hex(hover) : ""}</span>
      <button type="button" className="calcu-clr" onClick={() => shell.send("c59.heatmap-reset")}>CLR</button>
    </span>}>
      <canvas ref={ref} className="calcu-heatmap" style={{ aspectRatio: `${cols} / ${rows}` }}
        onPointerMove={onMove} onPointerLeave={() => setHover(-1)} />
    </Section>
  );
}

function AsmOverlay() {
  const { shell } = usePage();
  const asm = useTopic(shell, "c59.asm-overlay") || NO_ASM;
  const entries = useEntries(shell);
  const select = (e) => readFileInput(e, "text").then((f) => { if (f) shell.send("c59.asm-load", { text: f.content, name: f.name }); });
  return (
    <Section title="ASM OVERLAY">
      <div>File: {asm.fileName}</div>
      <div>Words: {asm.words}</div>
      <div>Status: {asm.status}</div>
      <div className="calcu-buttonrow">
        <label className="calcu-pill calcu-filebutton-inline">Select File<input type="file" onChange={select} /></label>
        <button type="button" className="calcu-pill" disabled={asm.words === 0 || !usable(entries, "c59.asm-run")}
          onClick={() => shell.send("c59.asm-run")}>Run</button>
        <button type="button" className="calcu-pill" onClick={() => shell.send("c59.asm-clear")}>Clear</button>
      </div>
    </Section>
  );
}

const NO_ROWS = [];
function CpuInspector({ data, font }) {
  const { shell } = usePage();
  const rows = useTopic(shell, "c59.cpu-debug") || NO_ROWS;
  const frozen = useRun(shell).frozen;
  const [picked, setPicked] = React.useState(-1);
  const listRef = React.useRef(null);
  const current = rows.findIndex((r) => r.current);
  // Running: follow the newest; frozen: the current instruction, unless one was picked.
  const auto = frozen ? current : rows.length - 1;
  React.useEffect(() => setPicked(-1), [frozen]);
  const selected = frozen && picked >= 0 && picked < rows.length ? picked : auto;
  useScrollTo(listRef, selected, frozen ? "center" : "bottom", [rows]);
  // Running, only the newest row carries registers (bindings.cpp postCpu).
  const f = rows[selected] && rows[selected].registers ? rows[selected] : undefined;
  const value = grey(0.85);
  return (
    <div className="calcu-debug-tab" style={{ fontSize: font }}>
      <FreezeHeader cpu={true} size={font - 2} />
      <div className="calcu-cpu-list" ref={listRef}>
        {rows.map((r, i) => (
          <div key={i} onClick={() => setPicked(i)}
            className={i === selected ? "calcu-cpu-selected" : ""}
            style={{ color: grey(r.history ? 0.9 : 0.35), background: r.current ? tint("green", 0.35) : undefined }}>
            {r.text}
          </div>
        ))}
      </div>
      <div className="calcu-debug-scroll">
        <Section title="REGISTERS">
          {f && f.registers.map(([n, v]) => <div key={n}><b style={{ color: grey(0.6) }}>{n}</b> <Run>{v}</Run></div>)}
        </Section>
        <Section title="CONTROL REGISTERS">
          {f && <>
            <div style={{ color: value }}>{f.KR}</div>
            <div style={{ color: value }}>{f.fA}</div>
            <div style={{ color: value }}>{f.fB}</div>
            <div><Run>{f.SR}</Run><Gap /><Run>{f.R5}</Run></div>
            <div>
              <Run color={f.cond ? tint("yellow", 0.4) : "var(--calcu-yellow)"}>{`COND: ${f.cond ? 1 : 0}`}</Run><Gap />
              <Run color={f.idle ? "var(--calcu-text)" : grey(0.45)}>{`IDLE: ${f.idle ? 1 : 0}`}</Run><Gap />
              <Run>{f.digit}</Run>
            </div>
          </>}
        </Section>
        <Section title="DISPLAY STATE">
          {f && <><Run>{f.lit}</Run><Gap />
            <Run color={f.displayOn ? tint("green", 0.85) : tint("red", 0.7)}>{f.displayOn ? "ON" : "OFF"}</Run></>}
        </Section>
        <Heatmap data={data} currentPC={current >= 0 ? rows[current].pc : -1} />
        <AsmOverlay />
      </div>
    </div>
  );
}

function LogTab() {
  const page = usePage();
  const shell = page.shell;
  const logLevel = useTopic(shell, "log-level");
  const log = { lines: page.log.lines, level: logLevel ? logLevel.level : "off" };
  const trace = useTopic(shell, "trace") || NO_TRACE;
  const traces = useTraces(page.traces);
  const ref = React.useRef(null);
  const text = React.useMemo(() => log.lines.join("\n"), [log.lines]);
  React.useLayoutEffect(() => { ref.current.scrollTop = ref.current.scrollHeight; }, [text]);
  // The session's log level: off → info → debug → off.
  const LEVELS = ["off", "info", "debug"];
  const level = Math.max(0, LEVELS.indexOf(log.level));
  const labels = ["LOG ○", "LOG ●", "LOG ●"];
  const mb = (n) => (n / 1e6).toFixed(n < 1e5 ? 2 : 1) + " MB";
  return (
    <div className="calcu-debug-tab">
      <div className="calcu-debug-header calcu-center"><span className="calcu-debug-title">DEBUG LOG</span></div>
      <textarea className="calcu-log" ref={ref} readOnly value={text} aria-label="Debug log" />
      <div className="calcu-log-bar">
        {traces.length > 0 && (
          <div className="calcu-traces" aria-label="Trace files">
            {traces.map((t) => (
              <button key={t.name} type="button" className="calcu-link" title={`Download ${t.name}`}
                onClick={() => page.traces.download(t.name)}>⤓ {t.name} ({mb(t.size)})</button>
            ))}
          </div>
        )}
        <div className="calcu-buttonrow">
          {[["SCOM", "scom"], ["Memory", "memory"], ["Prog", "prog"], ["Vars", "vars"]].map(([label, what]) => (
            <button key={what} type="button" className="calcu-pill" onClick={() => shell.send("c59.dump", { what })}>{label}</button>
          ))}
          <span className="calcu-spacer" />
          <button type="button" className="calcu-pill" title="Clear the log" aria-label="Clear the log" disabled={!text}
            onClick={() => page.log.clear()}>🗑</button>
        </div>
        <div className="calcu-buttonrow calcu-log-buttons">
          <button type="button" className="calcu-pill" aria-pressed={trace.running}
            title="Write every executed instruction to the binary trace file (downloaded when switched off)"
            style={trace.running ? { color: "var(--calcu-orange)" } : undefined}
            onClick={() => shell.send("trace", trace.running ? { off: true } : { button: true })}>{trace.running ? "TRACE ●" : "TRACE ○"}</button>
          <button type="button" className="calcu-pill" title="Log level: off / info (how presets run) / debug (every register write)"
            style={level ? { color: level === 1 ? "var(--calcu-orange)" : "var(--calcu-red)" } : undefined}
            onClick={() => shell.send("log-level", { level: LEVELS[(level + 1) % 3] })}>{labels[level]}</button>
        </div>
      </div>
    </div>
  );
}

function Debugger() {
  const { shell } = usePage();
  // The ROM heatmap's grid and colours (`c59.heatmap-grid`), asked once.
  const [data, setData] = React.useState(null);
  React.useEffect(() => {
    if (!shell) return;
    shell.request("c59.heatmap-grid").then((r) => { if (r.done) setData(r.value); });
  }, [shell]);
  const asmActive = (useTopic(shell, "c59.asm-overlay") || NO_ASM).active;
  const entries = useEntries(shell);
  const calculatorClosed = asmActive || !usable(entries, "c59.calculator-tab");
  const font = useTopic(shell, "c59.web.debug-font") || 11;
  const view = useTopic(shell, "c59.debug-view");
  const panelRequest = React.useMemo(() => (view ? { panel: Math.max(0, ["calculator", "cpu", "log"].indexOf(view.view)), n: view.requests } : { panel: 0, n: 0 }),
    [view && view.view, view && view.requests]);
  const [tab, setTab] = React.useState(panelRequest.panel);
  React.useEffect(() => { if (panelRequest.n) setTab(panelRequest.panel); }, [panelRequest.n]);
  const rootRef = React.useRef(null);
  const shown = calculatorClosed && tab === 0 ? 1 : tab;  // the overlay runs on the CPU tab

  React.useEffect(() => {
    if (!shell || !data) return undefined;
    shell.send("c59.show-calculator-debug", { on: shown === 0 });
    shell.send("c59.show-cpu-debug", { on: shown === 1 });
    return undefined;
  }, [shell, data, shown]);
  React.useEffect(() => () => {
    if (!shell) return;
    shell.send("c59.show-calculator-debug", { on: false });
    shell.send("c59.show-cpu-debug", { on: false });
  }, [shell]);

  React.useEffect(() => {
    const el = rootRef.current;
    if (!shell || !data) return undefined;
    const ro = new ResizeObserver(() => shell.send("c59.web.debug-width", { width: el.clientWidth }));
    ro.observe(el);
    return () => ro.disconnect();
  }, [shell, data]);

  return (
    <div className="calcu-debugger" data-panel="debug" ref={rootRef}>
      <div className="calcu-tabs" role="tablist">
        {["CALCULATOR", "CPU", "LOG"].map((t, i) => (
          <button key={t} type="button" role="tab" aria-selected={shown === i} disabled={i === 0 && calculatorClosed}
            onClick={() => setTab(i)}>{t}</button>
        ))}
      </div>
      {data && shown === 0 && <LiveDebug font={font + 2} />}
      {data && shown === 1 && <CpuInspector data={data} font={font + 2} />}
      {data && shown === 2 && <LogTab />}
    </div>
  );
}
