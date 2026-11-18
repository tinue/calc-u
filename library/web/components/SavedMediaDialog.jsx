// CalcuSavedMediaDialog: see MediaPicker.jsx, which is loaded before it and holds the helpers.

function CalcuSavedMediaDialog({ media, send, ask, onClose }) {
  const [problem, setProblem] = React.useState("");
  React.useEffect(() => {   // the folders read again when the list opens
    if (send) send("media.refresh", {});
  }, []);
  const kinds = (media && media.kinds) || [];
  const rows = [];
  kinds.forEach((k) => {
    [...(k.templates || []), ...(k.instances || [])]
      .filter((i) => i.origin === "saved")
      .sort((a, b) => a.name.toLowerCase().localeCompare(b.name.toLowerCase()))
      .forEach((i) => rows.push({ kind: k, item: i }));
  });
  const run = (command, args) => calcuSendAsking(send, ask, command, args).then((reply) => {
    setProblem(reply && !reply.done && reply.reason ? reply.reason : "");
  });
  return (
    <div className="calcu-media-dialog calcu-media-saved" role="dialog" aria-label="Saved Media">
      {rows.length === 0 ? <div className="calcu-media-none">No saved media.</div> : null}
      <table className="calcu-media-table">
        <tbody>
          {rows.map(({ kind, item }) => (
            <tr key={kind.kind + "/" + item.fileName} data-calcu={`media-saved-${kind.kind}-${item.name}`}>
              <td>{kind.label}</td>
              <td title={item.description || undefined}>{item.name}</td>
              <td>{item.template ? "template" : ""}{item.inPlace ? (item.template ? ", " : "") + "in " + item.inPlace : ""}</td>
              <td>
                <button type="button" data-calcu="media-download"
                  onClick={() => run("media.export", { kind: kind.kind, name: item.name })}>Download</button>
                <button type="button" data-calcu="media-delete" disabled={!!item.inPlace}
                  onClick={() => run("media.delete", { kind: kind.kind, name: item.name })}>Delete</button>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
      {problem ? <div className="calcu-media-problem">{problem}</div> : null}
      <div className="calcu-media-buttons">
        <button type="button" onClick={() => onClose && onClose()}>Close</button>
      </div>
    </div>
  );
}
