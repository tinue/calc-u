// CalcUTest -- the component of the product made for the shell's tests
// (LIB-15 design §2, T1), and the one global its calc.js defines. It shows
// what the tests read: the start state, the storage mode and its sentence,
// the notices, the machine, the save folder's files and the host services'
// answers, all as text and data- attributes. `window.calcuTestShells` holds
// each mounted component's shell, for the tests to send commands and
// requests (the products' hooks stay per product, spec Out of Scope).
// From LIB-8 (M-39): the paper panel of the React component set, with its
// owner and controls from `tp.printer`, and buttons that attach and detach
// the test printers TD-20 and TP-4 and press a machine key.
// From LIB-10 (M-40): the run-control buttons of the React component set
// for the level `cpu`, with labels of the product's.
// From LIB-14 (M-43): with the prop `shell` (host page: shell=1) the
// component is put together from the React set's shell instead (AC-22):
// the menu bar, a plain faceplate of 50 x 30 hit-map units at scale 4, the
// control bar with a media control for `test-card` and a product control,
// the area for panels with the paper panel, and the dialogs; the topics come
// from library/shell/tests/TestShellPart.hpp. `window.calcuTestActs` counts
// the product item `load-program`.

const CALCU_PRODUCT = "__CALCU_PRODUCT__";

// The JS form of TestShellPart.hpp's product() (LIB-14 §3.2).
const CALCU_TEST_SHELL_PRODUCT = {
  name: "Test-Product", version: "1.2.3", build: "abc123", helpUrl: "https://calc-u.org/test/",
  aboutLabel: "", aboutLines: ["A product made for the tests.", "No code of a product."],
  aboutLinks: [["Source", "https://example.org/source"], ["Privacy", "https://example.org/privacy"]],
  variantLabels: { "Model-A": "Revision", "test-plotter": "Pen" }, presetAccept: ".test",
  copyScreen: null, pasteText: true, moduleSlotPinOuts: ["module"], placesWithoutControl: ["host-drive"],
  items: [
    { group: "FileKinds", id: "load-program", label: "Load Program…", entry: "",
      act: () => { if (typeof window !== "undefined") window.calcuTestActs = (window.calcuTestActs || 0) + 1; } },
    { group: "MachineEnd", id: "disk-thing", label: "Disk Thing", entry: "tp.disk" },
  ],
};
// TestShellPart.hpp's hitMap(): keys 1 to 3 in a row, the display above.
const CALCU_TEST_HIT_MAP = {
  keys: [["1", { x: 0, y: 20, w: 10, h: 10 }], ["2", { x: 20, y: 20, w: 10, h: 10 }], ["3", { x: 40, y: 20, w: 10, h: 10 }]],
  display: { x: 0, y: 0, w: 50, h: 15 }, distance: 4, width: 50,
};
const CALCU_TEST_EXTRA = { Places: [<button key="tp-disk" type="button" data-calcu="tp-disk">Disk</button>] };

function CalcUTestCard({ entry }) {
  return <span className="calcu-test-card">card {entry && entry.id}</span>;
}
const CALCU_TEST_MEDIA = { "test-card": CalcUTestCard };

function CalcUTestShell({ shell }) {
  const s = useCalcuShell(shell, CALCU_TEST_SHELL_PRODUCT, CALCU_SHELL_WEB_TRAITS);
  const face = React.useRef(null);
  useShellFaceplate(shell, CALCU_TEST_HIT_MAP, face, 4);
  React.useEffect(() => { if (typeof window !== "undefined") window.calcuTestShellModel = s; });
  return (
    <div className="calcu-test-shell">
      <ShellMenuBar shell={s} />
      <div className="calcu-test-faceplate" data-calcu="test-faceplate" ref={face}
        style={{ display: s.romsMissing && !s.started ? "none" : undefined }} />
      <ShellRomsMissing shell={s} />
      <ShellControlBar shell={s} mediaControls={CALCU_TEST_MEDIA} extra={CALCU_TEST_EXTRA} />
      <ShellPanelArea shell={s} />
      <ShellDialogs shell={s} />
    </div>
  );
}

function CalcUTest(props) {
  const embedded = embedProps(props);
  const ref = React.useRef(null);
  const size = useFillParent(ref, embedded);
  const shell = useShell(CALCU_PRODUCT, embedded);
  const start = useShellState(shell, (s) => s.start);
  const failure = useShellState(shell, (s) => s.failure);
  const machine = useTopic(shell, "tp.machine");
  const files = useTopic(shell, "tp.files");
  const storage = useTopic(shell, "web.storage");
  const notices = useStream(shell, "notices");
  const answers = useStream(shell, "tp.answers");
  const printer = useTopic(shell, "tp.printer");

  React.useEffect(() => {
    if (!shell || typeof window === "undefined") return undefined;
    window.calcuTestShells = window.calcuTestShells || [];
    window.calcuTestShells.push(shell);
    return () => {
      const i = window.calcuTestShells.indexOf(shell);
      if (i >= 0) window.calcuTestShells.splice(i, 1);
    };
  }, [shell]);

  const ask = (service, args) => shell && shell.send("tp.ask", { service, args: args || {} });
  if (props.shell) {
    return (
      <div className="calcu-client calcu-test" ref={ref} tabIndex={0}
        style={{ width: size.width || undefined, height: size.height || undefined }}
        data-start={start} data-model={(machine && machine.model) || ""}>
        <CalcUTestShell shell={shell} />
      </div>
    );
  }
  return (
    <div className="calcu-client calcu-test" ref={ref} tabIndex={0}
      style={{ width: size.width || undefined, height: size.height || undefined }}
      data-start={start} data-model={(machine && machine.model) || ""} data-preset={(machine && machine.preset) || ""}
      data-keyboard={String(embedded.keyboard)} data-storage={(storage && storage.mode) || ""}
      data-width={size.width} data-height={size.height}>
      <div className="calcu-test-title">{CALCU_PRODUCT} {CalcUTest.version}</div>
      {start === "starting" && <div className="calcu-test-status" role="status">{CALCU_SENTENCES.starting(CALCU_PRODUCT)}</div>}
      {start === "failed" && <div className="calcu-test-failed" role="alert">{failure}</div>}
      {storage && storage.sentence && <div className="calcu-test-banner" role="status">{storage.sentence}</div>}
      <ul className="calcu-test-notices">{notices.map((n, i) => <li key={i}>{n.sentence}</li>)}</ul>
      <ul className="calcu-test-files">{((files && files.files) || []).map((f) => <li key={f}>{f}</li>)}</ul>
      <div className="calcu-test-buttons">
        <button type="button" className="calcu-test-pick" onClick={() => ask("file-picker", { accept: "", multiple: false })}>Pick</button>
        <button type="button" className="calcu-test-open" onClick={() => ask("open-url", { url: "about:blank#calcu" })}>Open</button>
        <button type="button" className="calcu-test-copy" onClick={() => ask("clipboard-text", { text: "calcu copied" })}>Copy</button>
      </div>
      <pre className="calcu-test-answers">{JSON.stringify(answers)}</pre>
      <div className="calcu-test-paper-buttons">
        <button type="button" className="calcu-test-attach-td20"
          onClick={() => shell && shell.send("tp.attach", { place: "printer", peripheral: "test-dotter", online: true })}>TD-20</button>
        <button type="button" className="calcu-test-attach-tp4"
          onClick={() => shell && shell.send("tp.attach", { place: "aux", peripheral: "test-plotter" })}>TP-4</button>
        <button type="button" className="calcu-test-detach"
          onClick={() => { if (shell) { shell.send("tp.detach", { place: "printer" }); shell.send("tp.detach", { place: "aux" }); } }}>Detach</button>
        <button type="button" className="calcu-test-press" onClick={() => shell && shell.send("tp.press", { key: "1" })}>Press 1</button>
      </div>
      <div className="calcu-test-run-control">
        <CalcuRunControlButtons shell={shell} level="cpu" labels={{ pause: "FREEZE", step: "STEP", resume: "RESUME" }} />
      </div>
      <div className="calcu-test-paper">
        <CalcuPaperPanel shell={shell} owner={(printer && printer.owner) || null}
          controls={(printer && printer.controls) || []} />
      </div>
    </div>
  );
}

CalcUTest.version = "__CALC_VERSION__";
CalcUTest.buttonUse = calcuButtonUse;   // LIB-10: the buttons' rule, for the table of run-control.spec.js
if (typeof window !== "undefined") window.CalcUTest = CalcUTest;
