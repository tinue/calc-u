// CalcUTest -- the component of the product made for the shell's tests
// (LIB-15 design §2, T1), and the one global its calc.js defines. It shows
// what the tests read: the start state, the storage mode and its sentence,
// the notices, the machine, the save folder's files and the host services'
// answers, all as text and data- attributes. `window.calcuTestShells` holds
// each mounted component's shell, for the tests to send commands and
// requests (the products' hooks stay per product, spec Out of Scope).

const CALCU_PRODUCT = "__CALCU_PRODUCT__";

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
    </div>
  );
}

CalcUTest.version = "__CALC_VERSION__";
if (typeof window !== "undefined") window.CalcUTest = CalcUTest;
