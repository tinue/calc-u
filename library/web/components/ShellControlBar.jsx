// ShellControlBar -- the control bar of the React set (LIB-14 design §3.4,
// §3.10; spec AC-8 to AC-10, AC-42): the groups of useCalcuShell's bar
// model left to right; pickers with the empty place as "None"; a button for
// a place one peripheral fits; LIB-7's module picker and the media owners'
// controls as the product hands them in; the product's own controls in the
// group it names (it reads its own product entries, AC-42). A group
// without a control takes no room (B4). Names (§3.10): control-bar,
// bar-model, bar-slot-<place>, bar-place-<place>, bar-media-<media place>.
//
//   <ShellControlBar shell={s} slotPicker={Component?} mediaControls={{kind: Component}}
//                    extra={{AfterModel|Slots|Places|Media|End: [elements]}} />
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

function ShellControlBar({ shell, slotPicker, mediaControls, extra }) {
  const s = shell;
  if (!s.started) return null;   // B7 (AC-19)
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
        if (!shown.length && !own.length) return null;   // B4: no gap
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
