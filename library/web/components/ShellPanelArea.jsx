// ShellPanelArea -- the area for panels of the React set: each panel the
// control state lists, drawn by the component given for its kind
// (CalcuPaperPanel for "paper" by
// default, with the owner of the control state), always, with or without an
// owner; the product's own panels follow as children. Where the area is and
// how panels share it is the product's layout. Name: panel-area.
//
//   <ShellPanelArea shell={s} panels={{paper: Component?}}>{product panels}</ShellPanelArea>
//
// A component of `panels` gets {shell: s.shell, owner, ownerPlace, hint, places,
// onChoose} (places: the place controls on the paper, s.choose for them).

function ShellPanelArea({ shell, panels, children }) {
  const s = shell;
  const given = panels || {};
  return (
    <div className="calcu-shell-panels" data-calcu="panel-area">
      {s.panels.map((p) => {
        const Panel = given[p.kind] || (p.kind === "paper" && typeof CalcuPaperPanel !== "undefined" ? CalcuPaperPanel : null);
        if (!Panel) return null;
        return <Panel key={p.kind} shell={s.shell} owner={p.owner || null} ownerPlace={p.ownerPlace || null} hint={p.hint}
          places={p.places} onChoose={s.choose} />;
      })}
      {children}
    </div>
  );
}
