// ShellNotice -- a notice or a question of the React set: the sentence as it is, no title, OK; a
// question (`asks:`) with Yes and No. Modal: it must be closed before the
// component can be used again, while the machine and a running operation go
// on behind it. Each notice is a modal of its own, so the modal's focus rule
// gives each its default button. Names: notice, notice-ok, notice-yes, notice-no.

function ShellNotice({ shell }) {
  const s = shell;
  const n = s.notice;
  if (!n) return null;
  const buttons = n.question ? (
    <React.Fragment>
      <button type="button" data-calcu="notice-no" onClick={() => s.closeNotice(false)}>No</button>
      <button type="button" className="calcu-shell-default" data-calcu="notice-yes" onClick={() => s.closeNotice(true)}>Yes</button>
    </React.Fragment>
  ) : (
    <button type="button" className="calcu-shell-default" data-calcu="notice-ok" onClick={() => s.closeNotice(true)}>OK</button>
  );
  return (
    <ShellModal key={n.seq} name="notice" onClose={() => s.closeNotice(false)} buttons={buttons}>
      <p className="calcu-shell-notice-text">{n.text}</p>
    </ShellModal>
  );
}
