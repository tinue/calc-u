// ShellAbout -- the About dialog of the React set: the product's name, its version and build, the texts and
// links the product gives, the Support Information (build time and what the browser tells of where the page
// runs, calcuShellHostRows), Copy (about-copy: calcuShellAboutText, the visible texts as plain text) and OK.
// Name: about-dialog.

function ShellAbout({ shell }) {
  const s = shell;
  const host = useShellHostRows();
  const a = calcuShellAbout(s.product, host);
  const copy = () => {
    const text = calcuShellAboutText(a);
    const refused = (e) => calcuAppLog("warn", `About: Copy failed: ${calcuErrorText(e)}`);
    try {
      navigator.clipboard.writeText(text).catch(refused);
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
          <div><b>{CALCU_SHELL_SUPPORT_HEADING}</b></div>
          {a.support.map(([label, value]) => <div key={label}>{label}: {value}</div>)}
        </div>
      )}
    </ShellModal>
  );
}

// The browser's rows: at once from the user agent string, then from
// Chromium's client hints when they come (the rows are the last part of the
// dialog, so nothing above them moves).
function useShellHostRows() {
  const [rows, setRows] = React.useState(() => calcuShellHostRows(window.navigator, null));
  React.useEffect(() => {
    const hints = window.navigator.userAgentData;
    if (!hints || typeof hints.getHighEntropyValues !== "function") return undefined;
    let live = true;
    hints.getHighEntropyValues(["platformVersion", "architecture", "bitness", "model", "fullVersionList"])
      .then((h) => { if (live) setRows(calcuShellHostRows(window.navigator, h)); })
      .catch(() => {});
    return () => { live = false; };
  }, []);
  return rows;
}
