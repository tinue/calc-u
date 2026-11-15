// ShellLoading -- "Loading…" of the React set (LIB-14 design §3.6 L2, D-6;
// spec AC-15, AC-16): the words with a sign that work is going on, no
// button, no title, transparent to the pointer, so that the menus, the bar
// and the keys stay usable. Placed over the component's upper part (the
// faceplate below its display); a product may place it itself. Name: loading.

function ShellLoading() {
  return (
    <div className="calcu-shell-loading" data-calcu="loading" role="status" aria-live="polite">
      <span className="calcu-shell-spinner" aria-hidden="true" />
      <span>Loading…</span>
    </div>
  );
}
