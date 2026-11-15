// ShellModal -- the one modal frame of the React set (LIB-14 design §2.3):
// a dialog over the component, which takes the pointer while it is open
// (a notice must be closed before the window can be used again, AC-17); and
// ShellDialogs, which shows the settings dialog, About, the notice and
// "Loading…" of useCalcuShell (§3.10).
//
//   <ShellModal name="settings-dialog" title="Settings" onClose={fn}>…</ShellModal>
//   <ShellDialogs shell={s} savedMedia={Component?} />
//
// savedMedia: LIB-7's saved media dialog (CalcuSavedMediaDialog), shown for
// File ▸ Saved Media… with {shell: s.shell, onClose}.

function ShellModal({ name, title, onClose, children, buttons }) {
  return (
    <div className="calcu-shell-modal-backdrop">
      <div className="calcu-shell-modal" role="dialog" aria-modal="true" aria-label={title || undefined} data-calcu={name}
        onKeyDown={(e) => { if (e.key === "Escape" && onClose) onClose(); }}>
        {title && <div className="calcu-shell-modal-title">{title}</div>}
        <div className="calcu-shell-modal-body">{children}</div>
        <div className="calcu-shell-modal-buttons">{buttons}</div>
      </div>
    </div>
  );
}

function ShellDialogs({ shell, savedMedia }) {
  const s = shell;
  const Saved = savedMedia;
  return (
    <React.Fragment>
      {s.loading && <ShellLoading />}
      {s.open === "settings" && <ShellSettings shell={s} />}
      {s.open === "about" && <ShellAbout shell={s} />}
      {s.open === "saved-media" && Saved && <Saved shell={s.shell} onClose={s.close} />}
      {s.notice && <ShellNotice shell={s} />}
    </React.Fragment>
  );
}
