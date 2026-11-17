// The product's own dialogs (LIB-14 Part D, D.4): those of C1600-2's flows
// (the machine-code start address and its result, "Load which one?"), the
// name prompts and questions of the media flows, LIB-7's Name & Save, and a
// notice as the shell's for what a flow refuses. Settings, About, the
// shell's notices and "Loading…" are the React set's (ShellDialogs). Each is
// shown through useDialogs()'s ask(), which resolves with the answer (null:
// cancelled), so the flows in CalcU1600.jsx read like MainWindow's.

function Modal({ title, children, buttons, onCancel, className, name }) {
  const ref = React.useRef(null);
  React.useEffect(() => {
    const el = ref.current;
    const focusable = el && el.querySelector("input, select, button.default, button");
    if (focusable) focusable.focus();
  }, []);
  return (
    <div className="calcu-modal-backdrop" onPointerDown={(e) => e.stopPropagation()}>
      <div className={`calcu-modal ${className || ""}`} role="dialog" aria-modal="true" aria-label={title}
        data-dialog={name} ref={ref}
        onKeyDown={(e) => {
          e.stopPropagation();
          if (e.key === "Escape" && onCancel) onCancel();
        }}>
        <div className="calcu-modal-title">{title}</div>
        <div className="calcu-modal-body">{children}</div>
        {buttons && <div className="calcu-modal-buttons">{buttons}</div>}
      </div>
    </div>
  );
}

// A notice of the product's own flows (a file that cannot be read, a
// refused plan): as the shell's notice (LIB-14 AC-17, AC-38): the sentence
// as it is, no title, OK.
function NoticeDialog({ text, done }) {
  return (
    <ShellModal name="notice" onClose={() => done(true)}
      buttons={<button type="button" data-calcu="notice-ok" autoFocus onClick={() => done(true)}>OK</button>}>
      <p className="calcu-shell-notice-text">{text}</p>
    </ShellModal>
  );
}

function ConfirmDialog({ title, text, done }) {
  return (
    <Modal title={title} name="confirm" onCancel={() => done(null)}
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-button" onClick={() => done(null)}>No</button>
          <button type="button" className="calcu-button default" onClick={() => done(true)}>Yes</button>
        </React.Fragment>
      }>
      <p className="calcu-modal-text">{text}</p>
    </Modal>
  );
}

// With a `checkbox` label the answer is { text, checked }; without, the text.
function PromptDialog({ title, label, checkbox, done }) {
  const [value, setValue] = React.useState("");
  const [checked, setChecked] = React.useState(false);
  const ok = () => done(checkbox ? { text: value.trim(), checked } : value.trim());
  return (
    <Modal title={title} name="prompt" onCancel={() => done(null)}
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-button" onClick={() => done(null)}>Cancel</button>
          <button type="button" className="calcu-button default" onClick={ok}>OK</button>
        </React.Fragment>
      }>
      <label className="calcu-form-row">
        <span>{label}</span>
        <input type="text" value={value} onChange={(e) => setValue(e.target.value)}
          onKeyDown={(e) => { if (e.key === "Enter") ok(); }} />
      </label>
      {checkbox && (
        <label className="calcu-form-row">
          <input type="checkbox" checked={checked} onChange={(e) => setChecked(e.target.checked)} />
          <span>{checkbox}</span>
        </label>
      )}
    </Modal>
  );
}

function ChoiceDialog({ title, text, options, done }) {
  const [index, setIndex] = React.useState(0);
  return (
    <Modal title={title} name="choice" onCancel={() => done(null)}
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-button" onClick={() => done(null)}>Cancel</button>
          <button type="button" className="calcu-button default" onClick={() => done(index)}>OK</button>
        </React.Fragment>
      }>
      <p className="calcu-modal-text">{text}</p>
      <select className="calcu-combo calcu-wide" value={index} onChange={(e) => setIndex(Number(e.target.value))}>
        {options.map((o, i) => <option key={i} value={i}>{o}</option>)}
      </select>
    </Modal>
  );
}

// Load Machine Code's start address (C1600-2 AC-18; Qt6/app/MachineCodeLoadDialog):
// each keystroke asks the session's plan (`c1600.plan-code`) with the text;
// Load is enabled only while it holds. Done with the text, or null.
function AddressDialog({ shell, plan, bytes, index, done }) {
  const [text, setText] = React.useState(plan.proposed || "");
  const [check, setCheck] = React.useState({ valid: false, hint: "" });
  React.useEffect(() => {
    let live = true;
    shell.request("c1600.plan-code", { bytes, file: index || 0, address: text }).then((r) => {
      if (!live) return;
      const p = r.done ? r.value : { refusal: r.reason };
      if (p.needsAddress) setCheck({ valid: false, hint: "" });
      else setCheck({ valid: !p.refusal, hint: p.refusal || p.where || "" });
    });
    return () => { live = false; };
  }, [text]);
  const ok = () => { if (check.valid) done(text); };
  return (
    <Modal title="Load Machine Code" name="machinecode" onCancel={() => done(null)} className="calcu-address"
      buttons={
        <React.Fragment>
          <button type="button" className="calcu-button" onClick={() => done(null)}>Cancel</button>
          <button type="button" className="calcu-button default" disabled={!check.valid} onClick={ok}>Load</button>
        </React.Fragment>
      }>
      <div className="calcu-form">
        <label className="calcu-form-row">
          <span>Start address:</span>
          <input type="text" value={text} placeholder={plan.placeholder} onChange={(e) => setText(e.target.value)}
            onKeyDown={(e) => { if (e.key === "Enter") ok(); }} />
        </label>
        <div className="calcu-form-row"><span>Length:</span><span>{plan.length} bytes</span></div>
      </div>
      {plan.cpuNote && <p className="calcu-modal-text">{plan.cpuNote}</p>}
      {check.hint && <p className="calcu-modal-text calcu-hint">{check.hint}</p>}
    </Modal>
  );
}

function LoadedDialog({ result, done }) {
  return (
    <Modal title="Machine Code Loaded" name="loaded" onCancel={() => done(true)}
      buttons={<button type="button" className="calcu-button default" onClick={() => done(true)}>OK</button>}>
      <div className="calcu-modal-text calcu-selectable">
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
      </div>
    </Modal>
  );
}

// Name & Save (LIB-7 AC-16, AC-40; design D-25): the library's dialog with
// today's prompt ("Instance name:" with the write-protect switch where the
// module has one still off, "Disk name:", "Cassette name:"); a name an instance
// has asks before it is replaced.
function NameAndSaveDialog({ shell, place, prompt, done }) {
  return (
    <Modal title="Name & Save" name="name-and-save" onCancel={() => done(null)}>
      <CalcuNameAndSaveDialog place={place} prompt={prompt} send={(c, a) => shell.send(c, a)}
        onClose={(saved) => done(saved ? true : null)} />
    </Modal>
  );
}

const DIALOGS = {
  notice: NoticeDialog, confirm: ConfirmDialog, prompt: PromptDialog, choice: ChoiceDialog,
  address: AddressDialog, loaded: LoadedDialog, nameAndSave: NameAndSaveDialog,
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
