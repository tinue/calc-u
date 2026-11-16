// The calculator's dialogs, drawn inside the client (an embedding page has
// no window of ours to open): the card picker (Qt6/app/CardPickerDialog, on
// the library's media topic) and the saved media, the preset picker (bundled presets
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
// The library's media part (LIB-7 59, C59-7 AC-14, AC-15): Arm Card lists the
// saved cards of the save folder by the names inside them (an outdated one
// cannot be chosen, its hint the tooltip) and fits the one chosen
// (`media.choose`); Arm Fresh Card asks the name and fits a fresh card of it
// (`media.start-new`), asking first when a card of that name is saved. The
// cards are files of the persistent save folder (LIB-15 AC-20): Import… and
// Saved Media… (LIB-7 AC-30, AC-31, AC-36) in place of a download per card
// and an upload; the File menu takes them in M-57 (LIB-7 D-29).

const CARD_ASKS = "asks: ";

function SavedMedia({ engine, onClose }) {
  const media = useEngine(engine, (s) => s.media);
  return (
    <div className="calcu-modal" onPointerDown={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <CalcuSavedMediaDialog media={media} send={(c, a) => engine.media(c, a)} onClose={onClose} />
    </div>
  );
}

function CardPicker({ engine, mode, onClose, onSavedMedia }) {
  const reader = useEngine(engine, (s) => cardReaderOf(s.media));
  const [selected, setSelected] = React.useState(null);
  const [name, setName] = React.useState("");
  const [asking, setAsking] = React.useState(null);
  const [problem, setProblem] = React.useState("");
  const fresh = mode === "fresh";
  // D-10: the saved cards only; the fresh card is Arm Fresh Card's way in.
  const saved = (reader ? reader.picker : []).filter((e) => e.id.startsWith("saved:"));

  const arm = (entry) => { engine.call("armCard", entry); onClose(); };
  const start = (confirmed) => {
    const args = { place: "card-reader", template: "implicit:Fresh card", name: name.trim() };
    if (confirmed) args.confirmed = true;
    engine.media("media.start-new", args).then((r) => {
      if (r.done) onClose();
      else if (r.reason && r.reason.startsWith(CARD_ASKS)) setAsking(r.reason.slice(CARD_ASKS.length));
      else setProblem(r.reason || "");
    });
  };

  return (
    <Dialog
      title={fresh ? "Arm Fresh Card" : "Arm Card"}
      onClose={onClose}
      footer={<>
        <button type="button" onClick={() => engine.call("importMedia")}>Import…</button>
        <button type="button" onClick={onSavedMedia}>Saved Media…</button>
        <button type="button" onClick={onClose}>Cancel</button>
        {asking
          ? <button type="button" className="calcu-primary" onClick={() => start(true)}>Replace</button>
          : <button type="button" className="calcu-primary" disabled={fresh ? !name.trim() : !selected}
                    onClick={fresh ? () => start(false) : () => arm(selected)}>Arm</button>}
      </>}
    >
      {asking && <p role="alert">{asking}</p>}
      {problem && <p role="alert">{problem}</p>}
      {fresh && (
        <input
          className="calcu-textfield"
          aria-label="Card name"
          value={name}
          autoFocus
          onChange={(e) => { setName(e.target.value); setAsking(null); setProblem(""); }}
          onKeyDown={(e) => { if (e.key === "Enter" && name.trim()) start(false); }}
        />
      )}
      <ul className="calcu-list" aria-label="Saved cards">
        {saved.length === 0 && <li className="calcu-empty">No cards found</li>}
        {saved.map((e) => (
          <li key={e.id} className={selected === e.id ? "calcu-selected" : ""}>
            <button
              type="button"
              className="calcu-listitem"
              disabled={!e.canChoose}
              title={e.hint || e.tooltip || undefined}
              onClick={() => { setSelected(e.id); if (fresh) { setName(e.label); setAsking(null); } }}
              onDoubleClick={() => { if (!fresh && e.canChoose) arm(e.id); }}
            >{e.canChoose ? e.label : `${e.label} (outdated)`}</button>
          </li>
        ))}
      </ul>
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
  // LIB-7 AC-47: the modules of slot-1's picker (the module file's description, menu-sort order).
  const slot = useEngine(engine, (s) => (s.media && s.media.places || []).find((p) => p.place === "slot-1") || null);
  const modules = slot
    ? slot.picker.filter((e) => e.group === "module").map((e) => ({ id: e.id.replace(/^module:/, ""), menuTitle: e.label }))
    : (data ? data.modules : []);
  const [moduleID, setModuleID] = React.useState(model ? model.moduleID : "ML");
  const [traceMB, setTraceMB] = React.useState(settings.traceMaxMB);

  // The module applies when the dialog closes, as in both apps.
  const close = () => {
    if (model && moduleID !== model.moduleID) {
      if (slot) engine.call("chooseModule", moduleID ? `module:${moduleID}` : "");
      else engine.call("changeModule", moduleID);
    }
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
            {modules.map((m) => <option key={m.id} value={m.id}>{m.menuTitle}</option>)}
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
    case "armCard": return <CardPicker engine={engine} mode="arm" onClose={close} onSavedMedia={() => setDialog("savedMedia")} />;
    case "armFreshCard": return <CardPicker engine={engine} mode="fresh" onClose={close} onSavedMedia={() => setDialog("savedMedia")} />;
    case "savedMedia": return <SavedMedia engine={engine} onClose={close} />;
    case "preset": return <PresetPicker engine={engine} onClose={close} />;
    case "settings": return <Settings engine={engine} feedbackSetting={feedbackSetting} traceSetting={traceSetting} onClose={close} onAbout={() => setDialog("about")} />;
    case "about": return <About onClose={close} />;
    default: return null;
  }
}
