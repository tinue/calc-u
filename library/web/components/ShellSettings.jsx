// ShellSettings -- the settings dialog of the React set: titled "Settings",
// a section per group of the dialog model, a change sent as `set-setting` at
// once, Close as the only button. It follows the `settings` topic and is
// held back by nothing. Folder rows show the folder in use with "Reset" (a
// Web frontend has no folder picker). A startup preset row ends with the
// model's "Choose…" (`row.choose`), which opens the file picker; the row
// shows the stored value until the settings topic changes. Names: settings-dialog,
// setting-<section>-<key>, settings-close.

// No setting has it as its value: a file path or bundle path never starts with NUL.
const CALCU_CHOOSE_VALUE = "\u0000choose";

function ShellSettingRow({ s, row }) {
  const r = row.row;
  const name = `setting-${r.section}-${r.key}`;
  const set = (v) => s.setSetting(r.section, r.key, v);
  let control = null;
  if (r.widget === "startup-model" || r.widget === "choice" || r.widget === "startup-preset") {
    const chooses = r.widget === "startup-preset" && !!row.choose;
    const changed = (v) => { if (chooses && v === CALCU_CHOOSE_VALUE) s.chooseStartupPreset(r.key); else set(v); };
    control = (
      <select data-calcu={name} value={row.value} onChange={(e) => changed(e.target.value)}>
        {row.choices.map(([v, l]) => <option key={v} value={v}>{l}</option>)}
        {chooses && <option key={CALCU_CHOOSE_VALUE} value={CALCU_CHOOSE_VALUE}>{row.choose}</option>}
      </select>
    );
  } else if (r.widget === "folder") {
    control = (
      <span data-calcu={name} className="calcu-shell-folder">
        <span className="calcu-shell-folder-shown" data-calcu={`${name}-shown`}>{row.shown}</span>
        <button type="button" data-calcu={`${name}-reset`} disabled={!row.resettable} onClick={() => set(null)}>Reset</button>
      </span>
    );
  } else if (r.widget === "integer") {
    control = (
      <input type="number" data-calcu={name} min={row.min === null ? undefined : row.min}
        max={row.max === null ? undefined : row.max} defaultValue={row.value} key={row.value}
        onBlur={(e) => { if (e.target.value !== row.value) set(Number(e.target.value)); }}
        onKeyDown={(e) => { if (e.key === "Enter") e.currentTarget.blur(); }} />
    );
  } else if (r.widget === "text") {
    control = <input type="text" data-calcu={name} defaultValue={row.value} key={row.value} onBlur={(e) => set(e.target.value)} />;
  } else if (r.widget === "note") {
    return <div className="calcu-shell-note" data-calcu={name}>{r.label}</div>;
  }
  return (
    <label className="calcu-shell-setting">
      <span className="calcu-shell-setting-label">{r.label}</span>
      {control}
    </label>
  );
}

function ShellSettings({ shell }) {
  const s = shell;
  return (
    <ShellModal name="settings-dialog" title="Settings" onClose={s.close}
      buttons={<button type="button" data-calcu="settings-close" onClick={s.close}>Close</button>}>
      {s.dialog.map((section) => (
        <fieldset key={section.title} className="calcu-shell-section">
          <legend>{section.title}</legend>
          {section.rows.map((row) => <ShellSettingRow key={`${row.row.section}.${row.row.key}`} s={s} row={row} />)}
        </fieldset>
      ))}
    </ShellModal>
  );
}
