// ShellRomsMissing -- in place of the faceplate when the product could not
// start (LIB-14 spec AC-19; LIB-12 AC-5): the text of the `roms-missing`
// topic. The product shows it where its faceplate would be; the shell then
// shows no control bar and no Machine menu. Name: roms-missing.
//
//   {s.romsMissing && !s.started ? <ShellRomsMissing shell={s} /> : <Faceplate/>}

function ShellRomsMissing({ shell }) {
  const s = shell;
  if (s.started || !s.romsMissing) return null;
  return (
    <div className="calcu-shell-roms-missing" data-calcu="roms-missing" role="alert">
      {calcuShellText(s.romsMissing, "text")}
    </div>
  );
}
