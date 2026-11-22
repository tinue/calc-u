// ShellControlBar -- the control bar of the React set: the groups of useCalcuShell's bar
// model left to right; pickers with the empty place named ("No printer"); a button for
// a place one peripheral fits; the library's module picker and the media owners'
// controls as the product hands them in; the product's own controls in the
// group it names (it reads its own product entries). A group
// without a control takes no room. Names: control-bar,
// bar-model, bar-slot-<place>, bar-place-<place>, bar-media-<media place>.
// End holds the More button at the right edge on every platform (bar-more,
// ShellMoreMenu; `compact`: its icon alone).
//
//   <ShellControlBar shell={s} slotPicker={Component?} mediaControls={{kind: Component}}
//                    extra={{AfterModel|Slots|Places|Media|End: [elements]}} compact={bool} />
//
// slotPicker and mediaControls get {shell: s, entry} (the place or media place
// entry); slotPicker also `short` (the bar model's shortShown). A group the
// model pushes (a spread bar) takes up the room before it.

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

// A place's control, on the bar or on the paper panel: a button where one
// peripheral fits (pressed while plugged in), a picker where several do.
// Name: bar-place-<place>. `children`: the button's content (its label).
function ShellPlaceControl({ control, onChoose, children }) {
  const c = control;
  if (c.kind === "picker") return <ShellPicker name={`bar-place-${c.id}`} choices={c.choices} onChoose={onChoose} />;
  const choice = c.choices[0] || {};
  const disabled = choice.enabled === false;
  return (
    <button type="button" className={"calcu-shell-place" + (c.plugged ? " calcu-shell-plugged" : "")}
      data-calcu={`bar-place-${c.id}`} aria-label={c.label} aria-pressed={c.plugged} aria-disabled={disabled ? "true" : undefined}
      title={disabled && choice.reason ? choice.reason : c.label}
      onClick={() => { if (!disabled) onChoose(choice); }}>
      {children === undefined ? c.label : children}
    </button>
  );
}

// The set's icon of the More button.

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
            {Slot ? <Slot shell={s} entry={c.entry} short={c.shortShown} />
              : <ShellPicker name={`bar-slot-picker-${c.id}`} choices={c.choices} onChoose={s.choose} />}
          </span>
        );
      case "button":
      case "picker":
        return <ShellPlaceControl key={c.id} control={c} onChoose={s.choose} />;
      case "more":
        return <ShellMoreMenu key="more" shell={s} items={c.choices} label={c.label} icon={c.icon} compact={compact} />;
      case "media": {
        const Media = mediaControls && mediaControls[c.mediumKind];
        return Media ? <span key={c.id} data-calcu={`bar-media-${c.id}`}><Media shell={s} entry={c.entry} /></span> : null;
      }
      default:
        return null;
    }
  };
  return (
    <div className={"calcu-shell-bar" + (compact ? " calcu-shell-compact" : "")} data-calcu="control-bar">
      {s.bar.map((g) => {
        const own = (extra && extra[g.group]) || [];
        const shown = g.controls.map(render).filter(Boolean);
        if (!shown.length && !own.length) return null;   // no entry, no control, no gap
        return (
          <span key={g.group} className={"calcu-shell-bar-group" + (g.pushed ? " calcu-shell-pushed" : "")}
            data-calcu={`bar-group-${g.group}`}>
            {shown}
            {own}
          </span>
        );
      })}
    </div>
  );
}
