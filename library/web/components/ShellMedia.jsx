// The library's media components placed in the shell, for both products'
// pages.
//
//   <ShellDialogs shell={s} savedMedia={ShellSavedMedia} />
//     File ▸ Saved Media…: the library's list (CalcuSavedMediaDialog) in the
//     shell's modal (data-calcu="saved-media")
//   <ShellControlBar slotPicker={Component} />, where the component draws
//   <ShellSlotPicker shell={s} entry={entry} label={text?} icon={element?} short={bool?} />
//     the library's module picker of a slot (the shell's slot picker),
//     disabled with the place's reason while it cannot be used (`empty`);
//     `label` names the fieldset; where the product asks for it
//     (nameAndSave.slotButtons) a Name & Save button (`icon` its glyph;
//     data-calcu="name-and-save-<place>"), usable while the place's medium
//     can be named and saved, which opens the shell's dialog
//   <ShellNameAndSaveButton shell={s} place={word} label={text?} icon={element} />
//     the Name & Save button of a media place, for the slot pickers and a
//     product's own media controls: usable while the place's medium can be
//     named and saved, it opens the shell's dialog; `label` names the place
//     in the button's accessible name (default: the place's word);
//     data-calcu="name-and-save-<place>"

function ShellSavedMedia({ shell, onClose }) {
  const media = useTopic(shell, "media");
  return (
    <ShellModal name="saved-media" title="Saved Media" onClose={onClose}>
      <CalcuSavedMediaDialog media={media} send={(c, a) => shell.send(c, a)} onClose={onClose} />
    </ShellModal>
  );
}

function ShellSlotPicker({ shell, entry, label, icon, short }) {
  const word = calcuShellText(entry, "place");
  const button = !!(shell.product && shell.product.nameAndSave && shell.product.nameAndSave.slotButtons);
  const place = calcuMediaPlace(shell.inputs.media, word);
  const ok = calcuShellAvailability(entry, "empty");
  return (
    <fieldset className="calcu-bar-fieldset" disabled={!ok.now} title={ok.now ? undefined : ok.reason} aria-label={label}>
      <CalcuMediaPicker place={place} send={(c, a) => shell.shell.send(c, a)} short={short} />
      {button && <ShellNameAndSaveButton shell={shell} place={word} icon={icon} />}
    </fieldset>
  );
}

function ShellNameAndSaveButton({ shell, place, label, icon }) {
  const medium = calcuMediaPlace(shell.inputs.media, place);
  return (
    <button type="button" className="calcu-iconbutton" disabled={!medium || !medium.nameAndSave} title="Name & Save"
      data-calcu={`name-and-save-${place}`} aria-label={`Name & Save ${label || place.replace("-", " ")}`}
      onClick={() => shell.nameAndSave(place)}>
      {icon}
    </button>
  );
}
