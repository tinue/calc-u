// The calculator's dialogs, drawn inside the client (an embedding page has
// no window of ours to open): the card picker (Qt6/app/CardPickerDialog,
// over the page session's card folder), the preset picker (bundled presets
// and file upload), Settings (Qt6/app/SettingsDialog, App/Views/
// SettingsView) and About.

const CALC_VERSION = "__CALC_VERSION__";
const HELP_URL = "https://calc-u.org/59/";

function Dialog({ title, onClose, children, footer }) {
  React.useEffect(() => {
    const onKey = (e) => { if (e.key === "Escape") { e.stopPropagation(); onClose(); } };
    window.addEventListener("keydown", onKey, true);
    return () => window.removeEventListener("keydown", onKey, true);
  }, [onClose]);
  return (
    <div className="calcu-modal" onPointerDown={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <div className="calcu-dialog" role="dialog" aria-label={title}>
        <div className="calcu-dialog-title">{title}</div>
        <div className="calcu-dialog-body">{children}</div>
        {footer && <div className="calcu-dialog-footer">{footer}</div>}
      </div>
    </div>
  );
}

function readFileInput(e, as) {
  const file = e.target.files && e.target.files[0];
  e.target.value = "";
  if (!file) return Promise.resolve(null);
  return (as === "text" ? file.text() : file.arrayBuffer().then((b) => new Uint8Array(b)))
    .then((content) => ({ name: file.name, content }));
}

// ── Cards ───────────────────────────────────────────────────────────────────

function CardPicker({ engine, mode, onClose }) {
  const card = useEngine(engine, (s) => s.card);
  const [selected, setSelected] = React.useState(null);
  const [name, setName] = React.useState(card.fileName);
  const save = mode === "save";

  const [replacing, setReplacing] = React.useState(null);

  const read = (n) => { engine.call("fitCard", n, null); onClose(); };
  const upload = (e) => readFileInput(e, "bytes").then((f) => {
    if (!f) return;
    engine.call("fitCard", f.name, f.content);
    onClose();
  });
  const fitBlank = (file) => { engine.call("fitBlankCard", file); onClose(); };
  const write = () => engine.request("freshCardFileName", name.trim()).then((file) => {
    if (card.cards.includes(file)) setReplacing(file);
    else fitBlank(file);
  });

  return (
    <Dialog
      title={save ? "Fit Fresh Card" : "Fit Card"}
      onClose={onClose}
      footer={<>
        <span className="calcu-dialog-note">Cards last for this page session — download one to keep it.</span>
        <button type="button" onClick={onClose}>Cancel</button>
        {replacing
          ? <button type="button" className="calcu-primary" onClick={() => fitBlank(replacing)}>Replace</button>
          : <button type="button" className="calcu-primary" disabled={save ? !name.trim() : !selected}
                    onClick={save ? write : () => read(selected)}>Fit</button>}
      </>}
    >
      {replacing && (
        <p role="alert">Replace card {replacing}? Its first write replaces both sides and the cue card.</p>
      )}
      {save && (
        <input
          className="calcu-textfield"
          aria-label="Card name"
          value={name}
          autoFocus
          onChange={(e) => { setName(e.target.value); setReplacing(null); }}
          onKeyDown={(e) => { if (e.key === "Enter" && name.trim()) write(); }}
        />
      )}
      <ul className="calcu-list" aria-label="Session cards">
        {card.cards.length === 0 && <li className="calcu-empty">{save ? "No existing cards" : "No cards in this session"}</li>}
        {card.cards.map((n) => (
          <li key={n} className={selected === n ? "calcu-selected" : ""}>
            <button
              type="button"
              className="calcu-listitem"
              onClick={() => { setSelected(n); if (save) { setName(n); setReplacing(null); } }}
              onDoubleClick={() => (save ? setName(n) : read(n))}
            >{n}</button>
            <button type="button" className="calcu-iconbutton" title={`Download ${n}`} aria-label={`Download ${n}`}
              onClick={() => engine.download("card", n)}>⤓</button>
          </li>
        ))}
      </ul>
      {!save && (
        <label className="calcu-filebutton">
          Upload card file…
          <input type="file" accept=".u59,.U59,.bin" onChange={upload} />
        </label>
      )}
    </Dialog>
  );
}

// ── Presets ─────────────────────────────────────────────────────────────────

function PresetPicker({ engine, onClose }) {
  const [presets, setPresets] = React.useState(null);
  // LIB-6 AC-31, AC-42: the Example entries of the preset list, in their
  // groups and order; one loads by its bundle path, an upload by its content.
  React.useEffect(() => {
    engine.request("presetList").then((all) => setPresets(all.filter((p) => p.kind === "example")))
      .catch(() => setPresets([]));
  }, [engine]);
  const pick = (path) => {
    onClose();
    engine.loadBundledPreset(path);
  };
  const upload = (e) => readFileInput(e, "text").then((f) => {
    if (!f) return;
    onClose();
    engine.loadPresetContent(f.content, f.name);
  });
  return (
    <Dialog title="Load Preset" onClose={onClose}
      footer={<button type="button" onClick={onClose}>Cancel</button>}>
      <ul className="calcu-list" aria-label="Presets">
        {presets === null && <li className="calcu-empty">Loading…</li>}
        {(presets || []).map((p, i) => (
          <li key={p.bundlePath}>
            {(i === 0 || presets[i - 1].group !== p.group) && <div className="calcu-group">{p.group}</div>}
            <button type="button" className="calcu-listitem" onClick={() => pick(p.bundlePath)}>
              {p.description || p.name}<span className="calcu-filename">{p.bundlePath.replace(/^examples:/, "")}</span>
            </button>
          </li>
        ))}
      </ul>
      <label className="calcu-filebutton">
        Open file…
        <input type="file" accept=".ti59,.ti58,.ti58c" onChange={upload} />
      </label>
    </Dialog>
  );
}

// ── Settings ────────────────────────────────────────────────────────────────

function Settings({ engine, feedbackSetting, traceSetting, onClose, onAbout }) {
  const data = useEngine(engine, (s) => s.data);
  const model = useEngine(engine, (s) => s.model);
  const settings = useEngine(engine, (s) => s.settings);
  const [moduleID, setModuleID] = React.useState(model ? model.moduleID : "ML");
  const [traceMB, setTraceMB] = React.useState(settings.traceMaxMB);

  // The module applies when the dialog closes, as in both apps.
  const close = () => {
    if (model && moduleID !== model.moduleID) engine.call("changeModule", moduleID);
    const mb = Number(traceMB);
    if (traceSetting && mb > 0 && mb !== settings.traceMaxMB) engine.call("setTraceMaxMB", mb);
    onClose();
  };
  return (
    <Dialog title="Settings" onClose={close}
      footer={<button type="button" className="calcu-primary" onClick={close}>Done</button>}>
      <div className="calcu-form">
        <label>Startup Calculator
          <select value={settings.startupModel} onChange={(e) => engine.call("setStartupModel", Number(e.target.value))}>
            <option value={-1}>Last Used</option>
            {(data ? data.models : []).map((m) => <option key={m.model} value={m.model}>{m.name}</option>)}
          </select>
        </label>
        <label>Solid State Module
          <select value={moduleID} onChange={(e) => setModuleID(e.target.value)}>
            {(data ? data.modules : []).map((m) => <option key={m.id} value={m.id}>{m.menuTitle}</option>)}
          </select>
        </label>
        {feedbackSetting && (
          <label>Keyboard Feedback
            <select value={settings.keyboardFeedback === 2 ? 2 : 0} onChange={(e) => engine.call("setKeyboardFeedback", Number(e.target.value))}>  {/* 0 off, 2 click; no haptics in a browser */}
              <option value={0}>Off</option>
              <option value={2}>Click Sound</option>
            </select>
          </label>
        )}
        {traceSetting && (
          <label>Maximum Trace File Size (MB)
            <input type="number" min="1" max="1000" value={traceMB} onChange={(e) => setTraceMB(e.target.value)} />
          </label>
        )}
        <div className="calcu-form-row"><span>Version</span><span>{CALC_VERSION}</span></div>
        <div className="calcu-form-row">
          <a href={HELP_URL} target="_blank" rel="noopener">Calc-U-59 Help</a>
          <button type="button" className="calcu-link" onClick={onAbout}>About</button>
        </div>
      </div>
    </Dialog>
  );
}

function About({ onClose }) {
  return (
    <Dialog title="About Calc-U-59" onClose={onClose}
      footer={<button type="button" className="calcu-primary" onClick={onClose}>OK</button>}>
      <p>A TI-59, TI-58 and TI-58C emulator with the PC-100C printer.</p>
      <p>Version {CALC_VERSION}</p>
      <p><a href={HELP_URL} target="_blank" rel="noopener">calc-u.org/59</a></p>
    </Dialog>
  );
}

// Which dialog is open, and the element that draws it.
function DialogHost({ engine, dialog, setDialog, feedbackSetting, traceSetting }) {
  const close = () => setDialog(null);
  switch (dialog) {
    case "readCard": return <CardPicker engine={engine} mode="load" onClose={close} />;
    case "writeCard": return <CardPicker engine={engine} mode="save" onClose={close} />;
    case "preset": return <PresetPicker engine={engine} onClose={close} />;
    case "settings": return <Settings engine={engine} feedbackSetting={feedbackSetting} traceSetting={traceSetting} onClose={close} onAbout={() => setDialog("about")} />;
    case "about": return <About onClose={close} />;
    default: return null;
  }
}
