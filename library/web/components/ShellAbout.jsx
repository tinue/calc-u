// ShellAbout -- the About dialog of the React set, as the worker serves it (useCalcuShell's
// `about`, shell::about): the product's name, its version and build, the texts and links the
// product gives, the Support Information (build time, the page's address and what the browser
// tells of where it runs), Copy (about-copy: the model's text, shell::aboutText) and OK.
// Name: about-dialog.

function ShellAbout({ shell }) {
  const s = shell;
  const a = s.about;
  if (!a) return null;   // the About item comes with the models
  const copy = () => {
    const refused = (e) => calcuAppLog("warn", `About: Copy failed: ${calcuErrorText(e)}`);
    try {
      navigator.clipboard.writeText(a.text).catch(refused);
    } catch (e) {
      refused(e);
    }
  };
  return (
    <ShellModal name="about-dialog" title={a.title} onClose={s.close}
      buttons={<React.Fragment>
        <button type="button" data-calcu="about-copy" onClick={copy}>Copy</button>
        <button type="button" className="calcu-shell-default" data-calcu="about-close" onClick={s.close}>OK</button>
      </React.Fragment>}>
      <div className="calcu-shell-about-name" data-calcu="about-name">{a.name}</div>
      <div className="calcu-shell-about-version" data-calcu="about-version">{a.version}</div>
      {a.lines.map((l, i) => <p key={i}>{l}</p>)}
      {a.links.map(([label, url]) => (
        <p key={url}><a href={url} target="_blank" rel="noopener noreferrer">{label}</a></p>
      ))}
      {a.support.length > 0 && (
        <div className="calcu-shell-about-support" data-calcu="about-support">
          <div><b>{a.supportHeading}</b></div>
          {a.support.map(([label, value]) => <div key={label}>{label}: {value}</div>)}
        </div>
      )}
    </ShellModal>
  );
}
