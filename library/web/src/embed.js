// The embedding props and the hooks a product's component uses. React 18 is
// a global of the host page.
//
//   embedProps(props, {name, layouts, layout}) → {width, keyboard (default
//                        true), layout, preset, height, model, workerUrl,
//                        rest}: the shared props in their order, then the
//                        product's own in rest; layout is
//                        one of the product's layouts, else its default (a
//                        value not among them is ignored with a console
//                        warning naming the component `name`)
//   bundlePathOf(preset) the preset prop as its bundle path: "sum.ti59" and
//                        "examples:sum.ti59" both give "examples:sum.ti59"
//                        (as the session's `start {model, preset}` takes it); empty
//                        stays empty
//   useFillParent(ref, {width, height}) → {width, height}: the props when
//                        both are given, else the size of ref's parent
//   useShell(product, embedded, services) → the shell (null until the effect
//                        ran); model, preset and workerUrl are read once,
//                        and so are the host services (createShell's
//                        `services`; default the page's, calcuWebHostServices)
//   useShellState(shell, select)   a slice of the shell's state
//   useTopic(shell, name, select)  the last state of a topic
//   useStream(shell, name)         what a stream topic appended since its last reset
//   calcuStreamAfter(items, marker)  a stream's items after an item taken before
//
// embedProps only names and defaults the props; whether a model or a
// preset names something is the session's start.

const CALCU_LAYOUT_WARNED = new Set();

function embedProps(props, product) {
  const { width, keyboard = true, layout, preset, height, model, workerUrl, ...rest } = props || {};
  const { name, layouts, layout: byDefault } = product;
  let taken = byDefault;
  if (layout !== undefined && layouts.includes(layout)) taken = layout;
  else if (layout !== undefined && !CALCU_LAYOUT_WARNED.has(`${name} ${layout}`)) {
    CALCU_LAYOUT_WARNED.add(`${name} ${layout}`);   // once, not at every render
    console.warn(`${name}: layout ${JSON.stringify(layout)} is not one of its layouts ` +
      `(${layouts.map((l) => JSON.stringify(l)).join(", ")}); it is ignored.`);
  }
  return { width, keyboard: keyboard !== false, layout: taken, preset, height, model, workerUrl, rest };
}

function bundlePathOf(preset) {
  if (!preset) return "";
  return preset.startsWith("examples:") ? preset : `examples:${preset}`;
}

function useFillParent(ref, { width, height }) {
  const fixed = typeof width === "number" && typeof height === "number";
  const [size, setSize] = React.useState({ width: fixed ? width : 0, height: fixed ? height : 0 });
  React.useEffect(() => {
    if (fixed) return undefined;
    const parent = ref.current && ref.current.parentElement;
    if (!parent || typeof ResizeObserver === "undefined") return undefined;
    const update = () => setSize((old) => {
      const w = Math.floor(parent.clientWidth);
      const h = Math.floor(parent.clientHeight);
      return old.width === w && old.height === h ? old : { width: w, height: h };
    });
    update();
    const observer = new ResizeObserver(update);
    observer.observe(parent);
    return () => observer.disconnect();
  }, [fixed]);
  return fixed ? { width, height } : size;
}

function useShell(product, embedded, services) {
  const [shell, setShell] = React.useState(null);
  const once = React.useRef({ model: embedded.model, preset: embedded.preset, workerUrl: embedded.workerUrl, services });
  React.useEffect(() => {
    const s = createShell({
      product,
      workerUrl: once.current.workerUrl,
      start: { model: once.current.model, preset: once.current.preset },
      services: once.current.services,
    });
    setShell(s);
    return () => s.destroy();
  }, []);
  return shell;
}

const CALCU_NO_SHELL = { getState: () => CALCU_NO_SHELL_STATE, subscribe: () => () => {} };
const CALCU_NO_ITEMS = [];

function useShellState(shell, select) {
  const s = shell || CALCU_NO_SHELL;
  return React.useSyncExternalStore(s.subscribe, () => select(s.getState()), () => select(CALCU_NO_SHELL_STATE));
}

function useTopic(shell, name, select) {
  return useShellState(shell, (st) => (select ? select(st.topics[name]) : st.topics[name]));
}

function useStream(shell, name) {
  return useShellState(shell, (st) => st.streams[name] || CALCU_NO_ITEMS);
}

// The items of a stream after `marker`, an item of it taken before. Items
// are told apart by identity: each comes once over postMessage, so no item of
// a reset or a later one is the marker. A marker no longer among the items
// (a reset, or more than CALCU_STREAM_KEEP items since) leaves them all.
function calcuStreamAfter(items, marker) {
  if (!marker) return items;
  const i = items.lastIndexOf(marker);
  return i < 0 ? items : items.slice(i + 1);
}
