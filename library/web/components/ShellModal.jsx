// ShellModal -- the one modal frame of the React set, the shell's dialogs'
// and the products' own: a dialog over the component, which takes the
// pointer while it is open (a notice must be closed before the window can be
// used again) and keeps every key from the calculator; Escape is onClose.
// Opened, it takes the focus unless a child took it (autoFocus): the
// default button (`calcu-shell-default`), else the dialog itself, so that
// Escape always reaches it. And ShellDialogs, which shows
// the settings dialog, About, the notice and "Loading…" of useCalcuShell.
//
//   <ShellModal name="settings-dialog" title="Settings" onClose={fn} buttons={…}>…</ShellModal>
//   <ShellDialogs shell={s} savedMedia={Component?} />
//
// savedMedia: the library's saved media dialog (CalcuSavedMediaDialog), shown for
// File ▸ Saved Media… with {shell: s.shell, onClose}.

function ShellModal({ name, title, onClose, children, buttons }) {
  const ref = React.useRef(null);
  React.useEffect(() => {
    const el = ref.current;
    if (!el || el.contains(document.activeElement)) return;
    const button = el.querySelector("button.calcu-shell-default:not(:disabled)");
    (button || el).focus({ preventScroll: true });
  }, []);
  return (
    <div className="calcu-shell-modal-backdrop">
      <div className="calcu-shell-modal" role="dialog" aria-modal="true" aria-label={title || undefined} data-calcu={name}
        ref={ref} tabIndex={-1}
        onKeyDown={(e) => {
          e.stopPropagation();
          if (e.key === "Escape" && onClose) onClose();
        }}>
        {title && <div className="calcu-shell-modal-title">{title}</div>}
        <div className="calcu-shell-modal-body">{children}</div>
        {buttons && <div className="calcu-shell-modal-buttons">{buttons}</div>}
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
