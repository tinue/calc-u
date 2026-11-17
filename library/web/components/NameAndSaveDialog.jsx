// CalcuNameAndSaveDialog (LIB-7 design §2, D-6, D-7; spec AC-16, AC-17, AC-19):
// see MediaPicker.jsx, which is loaded before it and holds the helpers.

function CalcuNameAndSaveDialog({ place, prompt, send, ask, onClose }) {
  const [name, setName] = React.useState("");
  const [protect, setProtect] = React.useState(false);
  const [problem, setProblem] = React.useState("");
  if (!place) return null;
  const save = () => {
    const args = { place: place.place, name };
    if (place.askWriteProtect && protect) args.protect = true;
    calcuSendAsking(send, ask, "media.name-and-save", args).then((reply) => {
      if (reply && reply.done) {
        if (onClose) onClose(true);
      } else if (reply && reply.reason) {
        setProblem(reply.reason);
      }
    });
  };
  return (
    <div className="calcu-media-dialog" role="dialog" aria-label="Name & Save">
      <label className="calcu-media-row">
        <span>{prompt || "Name:"}</span>
        <input data-calcu="media-name" value={name} autoFocus onChange={(e) => setName(e.target.value)} />
      </label>
      {place.askWriteProtect ? (
        <label className="calcu-media-row">
          <input type="checkbox" data-calcu="media-protect" checked={protect} onChange={(e) => setProtect(e.target.checked)} />
          <span>{CALCU_MEDIA_PROTECT}</span>
        </label>
      ) : null}
      {problem ? <div className="calcu-media-problem">{problem}</div> : null}
      <div className="calcu-media-buttons">
        <button type="button" onClick={() => onClose && onClose(false)}>Cancel</button>
        <button type="button" data-calcu="media-save" disabled={!name.trim()} onClick={save}>Save</button>
      </div>
    </div>
  );
}

