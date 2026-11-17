// The embedding props and the hooks a product's component uses (LIB-15
// design §3.1; spec AC-16, EC-4, EC-5). React 18 is a global of the host
// page (AC-15).
//
//   embedProps(props) → {width, keyboard (default true), layout, preset,
//                        height, model, workerUrl, rest}: the shared props
//                        in their order (AC-16, AC-29), then the product's
//                        own in rest; layout has the product's default
//   useFillParent(ref, {width, height}) → {width, height}: the props when
//                        both are given, else the size of ref's parent
//   useShell(product, embedded, services) → the shell (null until the effect
//                        ran); model, preset and workerUrl are read once (EC-5),
//                        and so are the host services (createShell's
//                        `services`; default the page's, calcuWebHostServices;
//                        Part E simplification E-5)
//   useShellState(shell, select)   a slice of the shell's state
//   useTopic(shell, name, select)  the last state of a topic
//   useStream(shell, name)         what a stream topic appended since its last reset
//
// embedProps only names and defaults the props; whether a model or a
// preset names something is the session's start (EC-4).

function embedProps(props) {
  const { width, keyboard = true, layout, preset, height, model, workerUrl, ...rest } = props || {};
  return { width, keyboard: keyboard !== false, layout, preset, height, model, workerUrl, rest };
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
