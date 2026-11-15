// ShellPanelArea -- the area for panels of the React set (LIB-14 design §3.4
// B6, §3.10; spec AC-23, EC-5): each panel the control state lists, drawn by
// the component given for its kind (LIB-8's CalcuPaperPanel for "paper" by
// default, with the owner of the control state), always, with or without an
// owner; the product's own panels follow as children. Where the area is and
// how panels share it is the product's layout (ADR 0023). Name: panel-area.
//
//   <ShellPanelArea shell={s} panels={{paper: Component?}}>{product panels}</ShellPanelArea>
//
// A component of `panels` gets {shell: s.shell, owner, ownerPlace}.

function ShellPanelArea({ shell, panels, children }) {
  const s = shell;
  const given = panels || {};
  return (
    <div className="calcu-shell-panels" data-calcu="panel-area">
      {s.panels.map((p) => {
        const Panel = given[p.kind] || (p.kind === "paper" && typeof CalcuPaperPanel !== "undefined" ? CalcuPaperPanel : null);
        if (!Panel) return null;
        return <Panel key={p.kind} shell={s.shell} owner={p.owner || null} ownerPlace={p.ownerPlace || null} controls={[]} />;
      })}
      {children}
    </div>
  );
}
