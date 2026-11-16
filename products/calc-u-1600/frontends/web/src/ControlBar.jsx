// The control bar under the faceplate, as Qt6/app/ControlBar: the model
// picker, the memory-slot pickers with their Name & Save buttons, the
// model's own peripheral toggles, the CE-1600F disk group (picker, side,
// save, motor lamp) and the tape group (picker, new tape, counter). What is
// shown and enabled comes from the engine's control state (c1600.ui); the
// pickers are the places of LIB-7's topic `media` (M-47), drawn in today's
// widgets until LIB-14 1600 (M-49, design D-25), and choosing sends
// `media.choose`.

const MODEL_NAMES = ["PC-1500", "PC-1500A", "PC-1600"];
const EMPTY_LABEL = "–empty–";
const SEPARATOR_LABEL = "──────────";

// The standard "save" glyph: a diskette.
function SaveIcon() {
  return (
    <svg className="calcu-save-icon" viewBox="0 0 16 16" aria-hidden="true">
      <path d="M2.5 1.5h9l2.5 2.5v9.5a1 1 0 0 1-1 1h-10.5a1 1 0 0 1-1-1v-11a1 1 0 0 1 1-1z" fill="#3d6fb6" />
      <rect x="4" y="1.5" width="7" height="4.5" fill="#e9eef6" />
      <rect x="8.5" y="2.3" width="1.6" height="2.9" fill="#3d6fb6" />
      <rect x="3.5" y="8.5" width="9" height="6" rx="0.5" fill="#fff" />
    </svg>
  );
}

// The place of the topic `media` named `word` ("slot-1", "floppy", "tape").
function mediaPlace(media, word) {
  return ((media && media.places) || []).find((p) => p.place === word) || null;
}

// A place's picker (LIB-7 AC-8, AC-9): "–empty–", then the entries of the
// topic in its order, a separator between groups; an entry's description is
// its tooltip, an outdated one disabled with its hint. The options' values
// are the labels (one per picker); choosing sends `media.choose`.
function MediaPickerSelect({ engine, place, disabled, label, name, minWidth }) {
  const entries = (place && place.picker) || [];
  const held = place && place.holds ? place.holds.name : "";
  const options = [];
  let group = null;
  entries.forEach((e, i) => {
    if (e.group === "empty") {
      options.push(<option key="empty" value="">{EMPTY_LABEL}</option>);
      return;
    }
    if (group !== null && group !== e.group && group !== "empty")
      options.push(<option key={`sep${i}`} disabled value={`\u0000${i}`}>{SEPARATOR_LABEL}</option>);
    group = e.group;
    const outdated = !e.canChoose && e.hint && e.hint.indexOf("outdated") >= 0;
    options.push(
      <option key={`${i}:${e.id}`} value={e.label} disabled={!e.canChoose}
        title={(e.canChoose ? e.tooltip : e.hint) || undefined}>
        {outdated ? `${e.label} (outdated)` : e.label}
      </option>);
  });
  const selected = entries.find((e) => e.label === held && e.group !== "empty");
  // A medium the place holds that no entry lists (a tape started under a
  // name, its file written at the first motor stop, LIB-7 AC-20): shown
  // as today's tape picker showed the tape in the deck.
  if (held && !selected) options.push(<option key="held" value={held}>{held}</option>);
  return (
    <select className="calcu-combo" value={held} disabled={disabled || !place} aria-label={label} name={name}
      title={(selected && selected.tooltip) || undefined} style={minWidth ? { minWidth } : undefined}
      onChange={(ev) => {
        const v = ev.target.value;
        const e = v === "" ? entries.find((x) => x.group === "empty") : entries.find((x) => x.label === v && x.group !== "empty");
        if (e && place) engine.send("media.choose", { place: place.place, entry: e.id });
      }}>
      {options}
    </select>
  );
}

function BarSeparator() {
  return <span className="calcu-bar-separator" aria-hidden="true" />;
}

function PeripheralToggle({ label, state, onToggle }) {
  if (!state || !state.onControlBar) return null;
  return (
    <button type="button" className={`calcu-toggle${state.attached ? " checked" : ""}`} aria-pressed={state.attached}
      disabled={!state.enabled} onClick={onToggle} title={`Attach/detach the ${label} (requires a power cycle)`}>
      {label}
    </button>
  );
}

function tapeCounter(lamps) {
  if (!lamps.tapeMode) return "–";
  const s = lamps.tapeSeconds;
  return `${lamps.tapeMode === "play" ? "▶" : "●"} ${Math.floor(s / 60)}:${String(s % 60).padStart(2, "0")}`;
}

function ControlBar({ engine, actions }) {
  const ui = useEngine(engine, (s) => s.ui);
  const media = useEngine(engine, (s) => s.media);
  const lamps = useEngine(engine, (s) => s.lamps);
  const busy = useEngine(engine, (s) => s.busy);
  if (!ui) return <div className="calcu-controlbar" />;

  const slot = (n) => {
    const place = mediaPlace(media, `slot-${n}`);
    return (
      <React.Fragment key={n}>
        <span className="calcu-bar-label">{n}:</span>
        <MediaPickerSelect engine={engine} place={place} disabled={busy} label={`Slot ${n}`} name={`slot${n}`} minWidth="11em" />
        <button type="button" className="calcu-iconbutton" disabled={!place || !place.nameAndSave} title="Name & Save"
          aria-label={`Name & Save slot ${n}`} onClick={() => actions.nameAndSave(`slot-${n}`)}>
          <SaveIcon />
        </button>
      </React.Fragment>
    );
  };
  const floppy = mediaPlace(media, "floppy");
  const tape = mediaPlace(media, "tape");
  const side = floppy && floppy.holds && floppy.holds.side ? 1 : 0;

  return (
    <div className="calcu-controlbar" role="toolbar">
      <select className="calcu-combo" value={ui.model} aria-label="Model" name="model" disabled={busy}
        onChange={(e) => engine.call("pickModel", Number(e.target.value))}>
        {MODEL_NAMES.map((n, i) => <option key={n} value={i}>{n}</option>)}
      </select>
      <BarSeparator />
      {slot(1)}
      {ui.slotCount >= 2 && <BarSeparator />}
      {ui.slotCount >= 2 && slot(2)}
      <span className="calcu-bar-stretch" />
      <BarSeparator />
      <PeripheralToggle label="CE-150" state={ui.ce150} onToggle={() => engine.call("togglePeripheral", 0)} />
      <PeripheralToggle label="CE-158" state={ui.ce158} onToggle={() => engine.call("togglePeripheral", 2)} />
      <PeripheralToggle label="CE-1600P" state={ui.ce1600p} onToggle={() => engine.call("togglePeripheral", 1)} />
      {ui.floppyShown && (
        <React.Fragment>
          <span className="calcu-bar-label">Disk:</span>
          <MediaPickerSelect engine={engine} place={floppy} disabled={!ui.floppyEnabled || busy} label="Disk" name="disk" minWidth="8em" />
          <button type="button" className="calcu-button calcu-compact" disabled={!ui.floppyEnabled || !floppy || !floppy.holds}
            title="Eject and turn the disk over" aria-label="Disk side"
            onClick={() => engine.send("media.turn", { place: "floppy" })}>
            {String(side)}
          </button>
          <button type="button" className="calcu-iconbutton" disabled={!floppy || !floppy.nameAndSave} title="Name & Save"
            aria-label="Name & Save disk" onClick={() => actions.nameAndSave("floppy")}>
            <SaveIcon />
          </button>
          <span className={`calcu-lamp${lamps.floppyMotor ? " on" : ""}`} data-lamp="floppy"
            title="Drive active -- wait for this to go dark before turning the disk over">●</span>
        </React.Fragment>
      )}
      {ui.tapeShown && (
        <React.Fragment>
          <BarSeparator />
          <span className="calcu-bar-label">Tape:</span>
          <MediaPickerSelect engine={engine} place={tape} disabled={busy} label="Tape" name="tape" minWidth="8em" />
          <button type="button" className="calcu-iconbutton" title="A new tape to record onto (CSAVE)"
            aria-label="New tape" onClick={() => actions.recordTape()}>
            <SaveIcon />
          </button>
          <button type="button" className="calcu-iconbutton" disabled={!tape || !tape.nameAndSave} title="Name & Save"
            aria-label="Name & Save tape" onClick={() => actions.nameAndSave("tape")}>
            <SaveIcon />
          </button>
          <span className={`calcu-tape-counter${lamps.tapeMotor ? " on" : ""}`} data-lamp="tape"
            title={lamps.tapeMode === "play" ? `Playing ${lamps.tapeFile}` : lamps.tapeMode === "record" ? `Recording onto ${lamps.tapeFile}` : "No tape"}>
            {tapeCounter(lamps)}
          </span>
        </React.Fragment>
      )}
    </div>
  );
}
