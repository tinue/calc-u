// ShellAbout -- the About dialog of the React set: the product's name, its version and build, the texts and
// links the product gives, one button to close it. Name: about-dialog.

function ShellAbout({ shell }) {
  const s = shell;
  const a = calcuShellAbout(s.product);
  return (
    <ShellModal name="about-dialog" title={a.title} onClose={s.close}
      buttons={<button type="button" className="calcu-shell-default" data-calcu="about-close" onClick={s.close}>OK</button>}>
      <div className="calcu-shell-about-name" data-calcu="about-name">{a.name}</div>
      <div className="calcu-shell-about-version" data-calcu="about-version">{a.version}</div>
      {a.lines.map((l, i) => <p key={i}>{l}</p>)}
      {a.links.map(([label, url]) => (
        <p key={url}><a href={url} target="_blank" rel="noopener noreferrer">{label}</a></p>
      ))}
    </ShellModal>
  );
}
