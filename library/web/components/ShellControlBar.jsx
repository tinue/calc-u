// ShellControlBar -- the control bar of the React set: the groups of useCalcuShell's bar
// model left to right; pickers with the empty place as "None"; a button for
// a place one peripheral fits; the library's module picker and the media owners'
// controls as the product hands them in; the product's own controls in the
// group it names (it reads its own product entries). A group
// without a control takes no room. Names: control-bar,
// bar-model, bar-slot-<place>, bar-place-<place>, bar-media-<media place>.
// In a window without a menu bar the bar
// also holds Actions (Reset in Start; Preset, Settings in End), each a
// button with the set's icon and its label unless `compact`
// (bar-action-<id>), and the More button (bar-more, ShellMoreMenu).
//
//   <ShellControlBar shell={s} slotPicker={Component?} mediaControls={{kind: Component}}
//                    extra={{Start|AfterModel|Slots|Places|Media|End: [elements]}} compact={bool} />
//
// slotPicker and mediaControls get {shell: s, entry} (the place or media place entry).

function ShellPicker({ name, choices, onChoose }) {
  const current = choices.findIndex((c) => c.checked);
  return (
    <select className="calcu-shell-picker" data-calcu={name} value={current < 0 ? "" : String(current)}
      onChange={(e) => {
        const c = choices[Number(e.target.value)];
        if (c && c.enabled !== false && c.command) onChoose(c);
      }}>
      {current < 0 && <option value="" disabled>—</option>}
      {choices.map((c, i) => (
        <option key={c.id} value={String(i)} disabled={c.enabled === false} title={c.reason || undefined}>{c.label}</option>
      ))}
    </select>
  );
}

// The set's icons of the bar's actions.
const CALCU_SHELL_ACTION_ICONS = { reset: "↺", "load-preset": "▤", settings: "⚙", more: "⋯" };

function ShellActionButton({ s, control, compact }) {
  const item = control.item || {};
  const disabled = item.enabled === false;
  return (
    <button type="button" className="calcu-shell-action" data-calcu={`bar-action-${control.id}`}
      aria-label={control.label} aria-disabled={disabled ? "true" : undefined}
      title={disabled && item.reason ? item.reason : control.label}
      onClick={() => { if (!disabled) s.choose(item); }}>
      <span className="calcu-shell-action-icon" aria-hidden="true">{CALCU_SHELL_ACTION_ICONS[control.id] || ""}</span>
      {!compact && <span className="calcu-shell-action-label">{control.label}</span>}
    </button>
  );
}

function ShellControlBar({ shell, slotPicker, mediaControls, extra, compact }) {
  const s = shell;
  if (!s.started) return null;   // no control state: no bar
  const Slot = slotPicker;
  const render = (c) => {
    switch (c.kind) {
      case "model":
        return <ShellPicker key="model" name="bar-model" choices={c.choices} onChoose={s.choose} />;
      case "slot":
        return (
          <span key={c.id} className="calcu-shell-slot" data-calcu={`bar-slot-${c.id}`}>
            {c.label && <span className="calcu-shell-slot-number">{c.label}</span>}
            {Slot ? <Slot shell={s} entry={c.entry} /> : <ShellPicker name={`bar-slot-picker-${c.id}`} choices={c.choices} onChoose={s.choose} />}
          </span>
        );
      case "button": {
        const choice = c.choices[0] || {};
        const disabled = choice.enabled === false;
        return (
          <button key={c.id} type="button" className={"calcu-shell-place" + (c.plugged ? " calcu-shell-plugged" : "")}
            data-calcu={`bar-place-${c.id}`} aria-pressed={c.plugged} aria-disabled={disabled ? "true" : undefined}
            title={disabled && choice.reason ? choice.reason : undefined}
            onClick={() => { if (!disabled) s.choose(choice); }}>
            {c.label}
          </button>
        );
      }
      case "picker":
        return <ShellPicker key={c.id} name={`bar-place-${c.id}`} choices={c.choices} onChoose={s.choose} />;
      case "action":
        return <ShellActionButton key={c.id} s={s} control={c} compact={compact} />;
      case "more":
        return <ShellMoreMenu key="more" shell={s} items={c.choices} label={c.label} compact={compact} />;
      case "media": {
        const Media = mediaControls && mediaControls[c.mediumKind];
        return Media ? <span key={c.id} data-calcu={`bar-media-${c.id}`}><Media shell={s} entry={c.entry} /></span> : null;
      }
      default:
        return null;
    }
  };
  return (
    <div className="calcu-shell-bar" data-calcu="control-bar">
      {s.bar.map((g) => {
        const own = (extra && extra[g.group]) || [];
        const shown = g.controls.map(render).filter(Boolean);
        if (!shown.length && !own.length) return null;   // no entry, no control, no gap
        return (
          <span key={g.group} className="calcu-shell-bar-group" data-calcu={`bar-group-${g.group}`}>
            {shown}
            {own}
          </span>
        );
      })}
    </div>
  );
}
