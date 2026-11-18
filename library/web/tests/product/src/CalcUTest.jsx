// CalcUTest -- the component of the product made for the shell's tests
// and the one global its calc.js defines. It shows
// what the tests read: the start state, the storage mode and its sentence,
// the notices, the machine, the save folder's files and the host services'
// answers, all as text and data- attributes. `window.calcuTestShells` holds
// each mounted component's shell, for the tests to send commands and
// requests (the products' hooks stay per product).
// The paper panel of the React component set, with its
// owner and controls from `tp.printer`, and buttons that attach and detach
// the test printers TD-20 and TP-4 and press a machine key.
// The run-control buttons of the React component set
// for the level `cpu`, with labels of the product's.
// With the prop `shell` (host page: shell=1) the
// component is put together from the React set's shell instead:
// the menu bar, a plain faceplate of 50 x 30 hit-map units at scale 4, the
// control bar with a media control for `test-card` and a product control,
// the area for panels with the paper panel, and the dialogs; the topics come
// from library/shell/tests/TestShellPart.hpp. `window.calcuTestActs` counts
// the product item `load-program`.
// In the product Test-M, whose session hosts the library's
// MediaPart, the media control of the bar is the React set's media
// components over the topic `media`: a CalcuMediaPicker per place, with a
// Name & Save button (CalcuNameAndSaveDialog) where the place offers it;
// File ▸ Saved Media… shows CalcuSavedMediaDialog.
// With the prop `faceOrigin` (host page:
// origin=N) the hit map's (0, 0) is N logical pixels into the faceplate, and
// a press at hit-map x ≥ 60 is the product's (`window.calcuTestProductPresses`
// counts them); the storage banner above the menu bar.

const CALCU_PRODUCT = "__CALCU_PRODUCT__";

// The JS form of TestShellPart.hpp's product().
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

// Test-M: every place of the topic `media`, whatever the scripted
// control state names.
function CalcUTestMediaPlaces({ shell }) {
  const [naming, setNaming] = React.useState(null);
  const media = shell.inputs.media;
  const places = (media && media.places) || [];
  const send = (c, a) => shell.shell.send(c, a);
  return (
    <span className="calcu-test-media">
      {places.map((p) => (
        <span key={p.place}>
          <CalcuMediaPicker place={p} send={send} />
          {p.nameAndSave ? (
            <button type="button" data-calcu={`media-name-and-save-${p.place}`} onClick={() => setNaming(p.place)}>Name &amp; Save</button>
          ) : null}
        </span>
      ))}
      {naming ? (
        <CalcuNameAndSaveDialog place={places.find((p) => p.place === naming)} prompt="Instance name:" send={send}
          onClose={() => setNaming(null)} />
      ) : null}
    </span>
  );
}
const CALCU_TEST_MEDIA_M = { "test-card": CalcUTestMediaPlaces };

function CalcUTestSavedMedia({ shell, onClose }) {
  const media = useTopic(shell, "media");
  return <CalcuSavedMediaDialog media={media} send={(c, a) => shell.send(c, a)} onClose={onClose} />;
}

function CalcUTestShell({ shell, faceOrigin }) {
  const s = useCalcuShell(shell, CALCU_TEST_SHELL_PRODUCT, CALCU_SHELL_WEB_TRAITS);
  const face = React.useRef(null);
  const origin = faceOrigin || 0;
  useShellFaceplate(shell, CALCU_TEST_HIT_MAP, face, {
    scale: 4,
    origin: { x: origin, y: origin },
    productFirst: (x) => {
      if (!origin || x < 60) return false;
      window.calcuTestProductPresses = (window.calcuTestProductPresses || 0) + 1;
      return true;
    },
  });
  React.useEffect(() => { if (typeof window !== "undefined") window.calcuTestShellModel = s; });
  return (
    <div className="calcu-test-shell">
      <ShellStorageBanner shell={shell} />
      <ShellMenuBar shell={s} />
      <div className="calcu-test-faceplate" data-calcu="test-faceplate" ref={face}
        style={{ display: s.romsMissing && !s.started ? "none" : undefined,
                 height: origin ? origin + 160 : undefined, width: origin ? origin + 320 : undefined }} />
      <ShellRomsMissing shell={s} />
      <ShellControlBar shell={s} mediaControls={CALCU_PRODUCT === "Test-M" ? CALCU_TEST_MEDIA_M : CALCU_TEST_MEDIA}
        extra={CALCU_TEST_EXTRA} />
      <ShellPanelArea shell={s} />
      <ShellDialogs shell={s} savedMedia={CALCU_PRODUCT === "Test-M" ? CalcUTestSavedMedia : undefined} />
    </div>
  );
}

function CalcUTest(props) {
  const embedded = embedProps(props, { name: "CalcUTest", layouts: ["auto"], layout: "auto" });
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
        <CalcUTestShell shell={shell} faceOrigin={props.faceOrigin} />
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
CalcUTest.buttonUse = calcuButtonUse;   // the buttons' rule, for the table of run-control.spec.js
if (typeof window !== "undefined") window.CalcUTest = CalcUTest;
