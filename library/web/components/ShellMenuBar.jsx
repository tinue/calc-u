// ShellMenuBar -- the menu bar of the React set (LIB-14 design §3.3, §3.10;
// spec AC-1 to AC-7, EC-4): the menus of useCalcuShell's model, a menu
// opened by a click, submenus beside their item. A Web frontend binds no
// shortcut (AC-4) and has no Quit (AC-6). A disabled item carries the
// reason as its `title` and `aria-disabled` (AC-5). Elements are named by
// `data-calcu` (§3.10): menu-<id>, item-<id>.
//
//   <ShellMenuBar shell={s} />

// `prefix`: the items' names ("item-"; the More menu's "more-item-");
// `onChoose`: in place of s.choose (the More menu asks first, M15).
function ShellMenuItems({ s, items, onDone, prefix = "item-", onChoose }) {
  const [sub, setSub] = React.useState(null);
  return (
    <ul className="calcu-shell-menu-items" role="menu">
      {items.map((item, i) => {
        if (item.kind === "separator") return <li key={`sep-${i}`} className="calcu-shell-separator" role="separator" />;
        const disabled = item.enabled === false;
        const isSub = item.kind === "submenu";
        return (
          <li key={item.id} className={"calcu-shell-item" + (disabled ? " calcu-shell-disabled" : "")} role="menuitem"
            data-calcu={`${prefix}${item.id}`} aria-disabled={disabled ? "true" : undefined}
            aria-checked={item.checkable ? String(!!item.checked) : undefined}
            title={disabled && item.reason ? item.reason : undefined}
            onMouseEnter={() => setSub(isSub ? item.id : null)}
            onClick={(e) => {
              e.stopPropagation();
              if (disabled) return;
              if (isSub) { setSub(item.id); return; }   // opens it (a hover may have opened it already)
              onDone();
              (onChoose || s.choose)(item);
            }}>
            <span className="calcu-shell-check">{item.checkable && item.checked ? "✓" : ""}</span>
            <span className="calcu-shell-label">{item.label}</span>
            {isSub && <span className="calcu-shell-arrow">▸</span>}
            {isSub && sub === item.id && <ShellMenuItems s={s} items={item.children} onDone={onDone} prefix={prefix} onChoose={onChoose} />}
          </li>
        );
      })}
    </ul>
  );
}

function ShellMenuBar({ shell }) {
  const s = shell;
  const [open, setOpen] = React.useState(null);
  const ref = React.useRef(null);
  React.useEffect(() => {
    if (!open) return undefined;
    const away = (e) => { if (ref.current && !ref.current.contains(e.target)) setOpen(null); };
    document.addEventListener("pointerdown", away);
    return () => document.removeEventListener("pointerdown", away);
  }, [open]);
  return (
    <nav className="calcu-shell-menubar" data-calcu="menu-bar" ref={ref} role="menubar">
      {s.menus.map((m) => (
        <div key={m.id} className={"calcu-shell-menu" + (open === m.id ? " calcu-shell-open" : "")}>
          <button type="button" className="calcu-shell-menu-title" data-calcu={`menu-${m.id}`} aria-haspopup="true"
            aria-expanded={open === m.id} onClick={() => setOpen(open === m.id ? null : m.id)}
            onMouseEnter={() => { if (open && open !== m.id) setOpen(m.id); }}>
            {m.label}
          </button>
          {open === m.id && <ShellMenuItems s={s} items={m.items} onDone={() => setOpen(null)} />}
        </div>
      ))}
    </nav>
  );
}
