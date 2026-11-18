// ShellMoreMenu -- the More button of a window without a menu bar: the button
// (⋯, its label unless `compact`) opens a popup of the menu bar's items
// (ShellMenuItems) anchored to it, kept inside the client's width; an item
// with `confirm` asks first in a ShellModal with the item's label and
// Cancel. The items are the bar model's `choices`, computed again at
// every change of an input. Names: bar-more, more-menu,
// more-item-<id>, more-confirm, more-confirm-yes, more-confirm-no.
//
//   <ShellMoreMenu shell={s} items={control.choices} label="More" compact={bool} />

function ShellMoreMenu({ shell, items, label, compact }) {
  const s = shell;
  const [open, setOpen] = React.useState(false);
  const [asking, setAsking] = React.useState(null);
  const [place, setPlace] = React.useState(null);
  const ref = React.useRef(null);
  React.useEffect(() => {
    if (!open) return undefined;
    const away = (e) => { if (ref.current && !ref.current.contains(e.target)) setOpen(false); };
    document.addEventListener("pointerdown", away);
    return () => document.removeEventListener("pointerdown", away);
  }, [open]);
  // The popup opens below (or above) the button, its right edge at the button's, and
  // stays inside the client (the narrowest portrait is 280 px). Fixed to the viewport, so that neither it nor the question's
  // modal is held by the bar's box.
  React.useLayoutEffect(() => {
    if (!open || !ref.current) { setPlace(null); return; }
    const popup = ref.current.querySelector(".calcu-shell-more-popup");
    const client = ref.current.closest(".calcu-client");
    if (!popup || !client) return;
    const c = client.getBoundingClientRect();
    const b = ref.current.getBoundingClientRect();
    const p = popup.getBoundingClientRect();
    const width = Math.min(p.width, c.width - 8);
    const left = Math.max(c.left + 4, Math.min(b.right, c.right - 4) - width);
    // Below the button, or above it when the window has no room below (the
    // bar sits at the bottom of the calculator).
    const below = b.bottom + p.height <= window.innerHeight;
    const top = below ? b.bottom : Math.max(0, b.top - p.height);
    setPlace({ left, top, maxWidth: c.width - 8 });
  }, [open]);
  const choose = (item) => {
    if (item.confirm) { setAsking(item); return; }
    s.choose(item);
  };
  const style = place ? { left: place.left, top: place.top, maxWidth: place.maxWidth } : { visibility: "hidden" };
  return (
    <span className="calcu-shell-more" ref={ref}>
      <button type="button" className="calcu-shell-action" data-calcu="bar-more" aria-haspopup="true" aria-expanded={open}
        aria-label={label} title={label} onClick={() => setOpen(!open)}>
        <span className="calcu-shell-action-icon" aria-hidden="true">{CALCU_SHELL_ACTION_ICONS.more}</span>
        {!compact && <span className="calcu-shell-action-label">{label}</span>}
      </button>
      {open && (
        <div className="calcu-shell-more-popup" data-calcu="more-menu" style={style}>
          <ShellMenuItems s={s} items={items} prefix="more-item-" onChoose={choose} onDone={() => setOpen(false)} />
        </div>
      )}
      {asking && (
        <ShellModal name="more-confirm" onClose={() => setAsking(null)}
          buttons={<React.Fragment>
            <button type="button" data-calcu="more-confirm-no" onClick={() => setAsking(null)}>Cancel</button>
            <button type="button" data-calcu="more-confirm-yes"
              className={asking.destructive ? "calcu-shell-destructive" : undefined}
              onClick={() => { const item = asking; setAsking(null); s.choose(Object.assign({}, item, { confirm: "" })); }}>
              {asking.label}
            </button>
          </React.Fragment>}>
          <p className="calcu-shell-notice-text">{asking.confirm}</p>
        </ShellModal>
      )}
    </span>
  );
}
