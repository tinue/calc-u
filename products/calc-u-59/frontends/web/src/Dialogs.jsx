// The calculator's own dialogs, drawn inside the client (an embedding page
// has no window of ours to open): the card dialogs (Qt6: app/
// CardPickerDialog, on the library's media topic), opened by the product
// items Arm Card… and Arm Fresh Card… (File menu, or the bar's card
// controls). The preset picker, Settings and About are the shell's: Load
// Preset… and Examples ▸, ShellSettings, ShellAbout; File ▸ Saved Media…
// is the library's ShellSavedMedia.

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
// The library's media part: Arm Card lists the
// saved cards of the save folder by the names inside them (one that cannot
// be chosen now is disabled, its hint the tooltip) and fits the one chosen
// (`media.choose`); Arm Fresh Card asks the name and fits a fresh card of it
// (`media.start-new`), asking first when a card of that name is saved. The
// cards are files of the persistent save folder: Import Media… and Saved
// Media… are the shell's File menu items (behind the More button in
// portrait), not buttons of this dialog.

function CardPicker({ shell, mode, onClose }) {
  const reader = cardReaderOf(useTopic(shell, "media"));
  const [selected, setSelected] = React.useState(null);
  const [name, setName] = React.useState("");
  const [asking, setAsking] = React.useState(null);
  const [problem, setProblem] = React.useState("");
  const fresh = mode === "fresh";
  // The saved cards only; the fresh card is Arm Fresh Card's way in.
  const saved = (reader ? reader.picker : []).filter((e) => e.id.startsWith("saved:"));

  const arm = (entry) => { shell.send("media.choose", { place: "card-reader", entry }); onClose(); };
  const start = (confirmed) => {
    const args = { place: "card-reader", template: "implicit:Fresh card", name: name.trim() };
    if (confirmed) args.confirmed = true;
    shell.send("media.start-new", args).then((r) => {
      const question = r.done ? null : calcuAskedQuestion(r.reason);
      if (r.done) onClose();
      else if (question !== null) setAsking(question);
      else setProblem(r.reason || "");
    });
  };

  return (
    <Dialog
      title={fresh ? "Arm Fresh Card" : "Arm Card"}
      onClose={onClose}
      footer={<>
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
            >{e.label}</button>
          </li>
        ))}
      </ul>
    </Dialog>
  );
}

