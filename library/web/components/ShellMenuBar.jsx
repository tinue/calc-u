// ShellMenuBar -- the menu bar of the React set: the menus of useCalcuShell's model, a menu
// opened by a click, submenus beside their item. A Web frontend binds no
// shortcut and has no Quit. A disabled item carries the
// reason as its `title` and `aria-disabled`. Elements are named by
// `data-calcu`: menu-<id>, item-<id>.
//
//   <ShellMenuBar shell={s} />

// `prefix`: the items' names ("item-"; the More menu's "more-item-");
// `onChoose`: in place of s.choose (the More menu asks first).
// `inline`: the More popup's way. A submenu opens below its item and
// pushes the items after it down, so it opens and closes by a click or a
// tap only: opened by hovering, the items would move under the pointer.
// The menu bar's submenus fly out beside their item and open on hover.
// `sizer`: every submenu open, no names, no handlers; the More popup
// measures its width with it.
function ShellMenuItems({ s, items, onDone, prefix = "item-", onChoose, inline = false, sizer = false }) {
  const [sub, setSub] = React.useState(null);
  const ref = React.useRef(null);
  // A submenu that opens inline in the More popup comes into view (the
  // popup scrolls; a low window shows only part of it).
  React.useEffect(() => {
    const ul = ref.current;
    if (!sizer && inline && ul && ul.parentElement && ul.parentElement.tagName === "LI")
      ul.scrollIntoView({ block: "nearest" });
  }, []);
  return (
    <ul className="calcu-shell-menu-items" role={sizer ? undefined : "menu"} ref={ref}>
      {items.map((item, i) => {
        if (item.kind === "separator") return <li key={`sep-${i}`} className="calcu-shell-separator" role="separator" />;
        if (item.kind === "heading") return <li key={item.id} className="calcu-shell-heading" role="presentation" data-calcu={sizer ? undefined : `${prefix}${item.id}`}>{item.label}</li>;
        const disabled = item.enabled === false;
        const isSub = item.kind === "submenu";
        const open = isSub && (sizer || sub === item.id);
        const row = (
          <div className="calcu-shell-row">
            <span className="calcu-shell-check">{item.checkable && item.checked ? "✓" : ""}</span>
            <span className="calcu-shell-label">{item.label}</span>
            {isSub && <span className="calcu-shell-arrow">{inline && open ? "▾" : "▸"}</span>}
          </div>
        );
        if (sizer) return <li key={item.id} className="calcu-shell-item">{row}{open && <ShellMenuItems s={s} items={item.children} inline sizer />}</li>;
        return (
          <li key={item.id} className={"calcu-shell-item" + (disabled ? " calcu-shell-disabled" : "") + (open ? " calcu-shell-sub-open" : "")}
            role="menuitem" data-calcu={`${prefix}${item.id}`} aria-disabled={disabled ? "true" : undefined}
            aria-haspopup={isSub ? "true" : undefined} aria-expanded={isSub ? open : undefined}
            aria-checked={item.checkable ? String(!!item.checked) : undefined}
            title={disabled && item.reason ? item.reason : undefined}
            onMouseEnter={inline ? undefined : () => setSub(isSub ? item.id : null)}
            onClick={(e) => {
              e.stopPropagation();
              if (disabled) return;
              // Inline a click opens or closes it; beside, it opens it (a hover may have already).
              if (isSub) { setSub(inline && sub === item.id ? null : item.id); return; }
              onDone();
              (onChoose || s.choose)(item);
            }}>
            {row}
            {open && <ShellMenuItems s={s} items={item.children} onDone={onDone} prefix={prefix} onChoose={onChoose} inline={inline} />}
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
