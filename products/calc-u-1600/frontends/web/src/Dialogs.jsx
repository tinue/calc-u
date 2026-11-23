// The product's own dialogs: those of the program-file flows (the
// machine-code start address and its result, "Load which one?"), the
// media's Name & Save, and a notice as the shell's for what a flow refuses,
// each in the React set's ShellModal. Settings, About, the shell's notices
// and "Loading…" are the React set's (ShellDialogs). Each is shown through
// useDialogs()'s ask(), which resolves with the answer (null: cancelled), so
// the flows in CalcU1600.jsx read like MainWindow's.

// Cancel and OK (or `label`), the default button.
function OkCancel({ ok, cancel, label = "OK", disabled = false }) {
  return (
    <React.Fragment>
      <button type="button" onClick={cancel}>Cancel</button>
      <button type="button" className="calcu-shell-default" disabled={disabled} onClick={ok}>{label}</button>
    </React.Fragment>
  );
}

// A notice of the product's own flows (a file that cannot be read, a
// refused plan): as the shell's notice: the sentence
// as it is, no title, OK.
function NoticeDialog({ text, done }) {
  return (
    <ShellModal name="notice" onClose={() => done(true)}
      buttons={<button type="button" className="calcu-shell-default" data-calcu="notice-ok" autoFocus onClick={() => done(true)}>OK</button>}>
      <p className="calcu-shell-notice-text">{text}</p>
    </ShellModal>
  );
}

function ChoiceDialog({ title, text, options, done }) {
  const [index, setIndex] = React.useState(0);
  return (
    <ShellModal title={title} name="choice" onClose={() => done(null)}
      buttons={<OkCancel ok={() => done(index)} cancel={() => done(null)} />}>
      <p className="calcu-shell-notice-text">{text}</p>
      <select autoFocus value={index} onChange={(e) => setIndex(Number(e.target.value))}>
        {options.map((o, i) => <option key={i} value={i}>{o}</option>)}
      </select>
    </ShellModal>
  );
}

// Load Machine Code's start address (as Qt6/app/MachineCodeLoadDialog):
// each keystroke asks the session's plan (`c1600.code-plan`) with the text;
// Load is enabled only while it holds. Done with the text, or null.
function AddressDialog({ shell, plan, bytes, index, done }) {
  const [text, setText] = React.useState(plan.proposed || "");
  const [check, setCheck] = React.useState({ valid: false, hint: "", problem: false });
  React.useEffect(() => {
    let live = true;
    shell.request("c1600.code-plan", { bytes, file: index || 0, address: text }).then((r) => {
      if (!live) return;
      const p = r.done ? r.value : { refusal: r.reason };
      if (p.needsAddress) setCheck({ valid: false, hint: "", problem: false });
      else setCheck({ valid: !p.refusal, hint: p.refusal || p.where || "", problem: !!p.refusal });
    });
    return () => { live = false; };
  }, [text]);
  const ok = () => { if (check.valid) done(text); };
  return (
    <ShellModal title="Load Machine Code" name="machinecode" onClose={() => done(null)}
      buttons={<OkCancel ok={ok} cancel={() => done(null)} label="Load" disabled={!check.valid} />}>
      <label className="calcu-shell-setting">
        <span className="calcu-shell-setting-label">Start address:</span>
        <input type="text" autoFocus value={text} placeholder={plan.placeholder} onChange={(e) => setText(e.target.value)}
          onKeyDown={(e) => { if (e.key === "Enter") ok(); }} />
      </label>
      <div className="calcu-shell-setting">
        <span className="calcu-shell-setting-label">Length:</span><span>{plan.length} bytes</span>
      </div>
      {plan.cpuNote && <p className="calcu-shell-notice-text">{plan.cpuNote}</p>}
      {check.hint && <p className={check.problem ? "calcu-shell-problem" : "calcu-shell-notice-text"}>{check.hint}</p>}
    </ShellModal>
  );
}

function LoadedDialog({ result, done }) {
  return (
    <ShellModal title="Machine Code Loaded" name="loaded" onClose={() => done(true)}
      buttons={<button type="button" className="calcu-shell-default" onClick={() => done(true)}>OK</button>}>
      <p>{result.summary}.</p>
      <p>
        Keep BASIC from overwriting it:<br />
        {result.newCommand && <React.Fragment><b><tt>{result.newCommand}</tt></b><br /></React.Fragment>}
        <small>{result.newNote}</small>
      </p>
      <p>
        Start it:<br />
        <b><tt>{result.callCommand}</tt></b><br />
        <small>{result.callNote}</small>
      </p>
    </ShellModal>
  );
}

const DIALOGS = {
  notice: NoticeDialog, choice: ChoiceDialog,
  address: AddressDialog, loaded: LoadedDialog,
};

// ask(kind, props) shows one dialog and resolves with its answer.
function useDialogs() {
  const [current, setCurrent] = React.useState(null);
  const queue = React.useRef([]);
  const next = () => setCurrent(queue.current.length ? queue.current.shift() : null);
  const ask = React.useCallback((kind, props) => new Promise((resolve) => {
    const entry = { kind, props, resolve };
    setCurrent((c) => {
      if (c) { queue.current.push(entry); return c; }
      return entry;
    });
  }), []);
  let element = null;
  if (current) {
    const Dialog = DIALOGS[current.kind];
    element = <Dialog {...current.props} done={(value) => { current.resolve(value); next(); }} />;
  }
  return { ask, element, open: !!current };
}
