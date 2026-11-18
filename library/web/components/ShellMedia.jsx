// The library's media components placed in the shell, for both products'
// pages.
//
//   <ShellDialogs shell={s} savedMedia={ShellSavedMedia} />
//     File ▸ Saved Media…: the library's list (CalcuSavedMediaDialog) in the
//     shell's modal (data-calcu="saved-media")
//   <ShellControlBar slotPicker={Component} />, where the component draws
//   <ShellSlotPicker shell={s} entry={entry} label={text?} nameAndSave={fn?} icon={element?} />
//     the library's module picker of a slot (the shell's slot picker),
//     disabled with the place's reason while it cannot be used (`empty`);
//     `label` names the fieldset; with `nameAndSave(word)` a Name & Save
//     button (`icon` its glyph; data-calcu="name-and-save-<place>"), usable
//     while the place's medium can be named and saved

function ShellSavedMedia({ shell, onClose }) {
  const media = useTopic(shell, "media");
  return (
    <ShellModal name="saved-media" title="Saved Media" onClose={onClose}>
      <CalcuSavedMediaDialog media={media} send={(c, a) => shell.send(c, a)} onClose={onClose} />
    </ShellModal>
  );
}

function ShellSlotPicker({ shell, entry, label, nameAndSave, icon }) {
  const word = calcuShellText(entry, "place");
  const place = calcuMediaPlace(shell.inputs.media, word);
  const ok = calcuShellAvailability(entry, "empty");
  return (
    <fieldset className="calcu-bar-fieldset" disabled={!ok.now} title={ok.now ? undefined : ok.reason} aria-label={label}>
      <CalcuMediaPicker place={place} send={(c, a) => shell.shell.send(c, a)} />
      {nameAndSave && (
        <button type="button" className="calcu-iconbutton" disabled={!place || !place.nameAndSave} title="Name & Save"
          data-calcu={`name-and-save-${word}`} aria-label={`Name & Save ${word.replace("-", " ")}`}
          onClick={() => nameAndSave(word)}>
          {icon}
        </button>
      )}
    </fieldset>
  );
}
