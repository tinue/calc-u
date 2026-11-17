// A product page's glue to its session (Part E simplification E-5; LIB-14
// D-32, D-31, LIB-13): what both products' pages wrote each.
//
//   useShellProduct(shell, {request, args, fallback, version, build,
//                   copyScreen, presetAccept, openFiles, acts}) → {product, ready}
//     the product's shell data: `request` (`<prefix>.shell-product`) with
//     `args`, once per distinct `args` while the session runs, its answer
//     {product, settingsSpec} kept; `fallback` (the shell's frame with
//     nothing of the product's) until it came. The product is the answer's
//     with the page's `version` and `build`, Copy Screen (`copyScreen`, or
//     null: none), the settings spec, Load Preset… (the picked files,
//     `presetAccept`, through `openFiles(files, "dialog")`) and each item's
//     act from `acts` by its id. copyScreen, openFiles and acts are read
//     when used, so they may be new closures at each render.
//   useFaceplateMaps(shell, request, model) → {[model]: map}
//     the faceplate maps, one `request` {model} per model (D-31)
//   calcuOpenFiles(send, files, way) → the reply, or null without files
//     LIB-13's `open {files: [{name, bytes}], way, upload: true}` (several
//     files are one folder in /uploads, AC-8); `send` is the page's sender
//     (the shell's, or useCalcuShell's sendChecked to show a refusal)
//   calcuFileDrop(open, rule) → {onDragOver, onDrop}
//     a drop of files: `open(files, "drop")` (null: none yet) by the
//     product's rule, which stays its own (each is what its users know):
//     CALCU_DROP_ONE_FILE (a drop of exactly one file; a drop never
//     reaches the browser) or CALCU_DROP_FIRST_FILE (the first file of a
//     drop; one without a file or before the session is the browser's);
//     which product has which: LIB-13's design, Part E simplification E-5

const CALCU_DROP_ONE_FILE = "one-file";
const CALCU_DROP_FIRST_FILE = "first-file";

function useShellProduct(shell, options) {
  const start = useShellState(shell, (st) => st.start);
  const args = options.args === undefined ? null : options.args;
  const key = JSON.stringify(args);
  const [answers, setAnswers] = React.useState({});
  React.useEffect(() => {
    if (!shell || start !== "running" || answers[key]) return;
    shell.request(options.request, args).then((r) => {
      if (r.done) setAnswers((a) => ({ ...a, [key]: r.value }));
    });
  }, [shell, start, key]);
  const latest = React.useRef(options);
  latest.current = options;
  const data = answers[key];
  const hasCopyScreen = !!options.copyScreen;
  const product = React.useMemo(() => {
    if (!data) return options.fallback;
    const p = data.product;
    return {
      ...p,
      version: options.version,
      build: options.build,
      copyScreen: hasCopyScreen ? () => latest.current.copyScreen() : null,
      settingsSpec: data.settingsSpec,
      loadPreset: async () => {
        const files = await calcuPickFiles(latest.current.presetAccept || "", true);
        if (files.length) latest.current.openFiles(files, "dialog");
      },
      items: p.items.map((i) => ({ ...i, act: () => { const act = (latest.current.acts || {})[i.id]; if (act) act(); } })),
    };
  }, [data, hasCopyScreen]);
  return { product, ready: !!data };
}

function useFaceplateMaps(shell, request, model) {
  const start = useShellState(shell, (st) => st.start);
  const [maps, setMaps] = React.useState({});
  React.useEffect(() => {
    if (!shell || start !== "running" || maps[model]) return;
    shell.request(request, { model }).then((r) => {
      if (r.done) setMaps((m) => ({ ...m, [model]: r.value }));
    });
  }, [shell, start, model]);
  return maps;
}

async function calcuOpenFiles(send, files, way) {
  const list = Array.from(files || []);
  if (!list.length) return null;
  const named = await Promise.all(list.map((f) => f.arrayBuffer().then((b) => ({ name: f.name, bytes: new Uint8Array(b) }))));
  return send("open", { files: named, way, upload: true });
}

function calcuFileDrop(open, rule) {
  return {
    onDragOver: (e) => { if (e.dataTransfer.types.includes("Files")) e.preventDefault(); },
    onDrop: (e) => {
      const files = [...e.dataTransfer.files];
      if (rule === CALCU_DROP_ONE_FILE) {
        e.preventDefault();
        if (files.length === 1 && open) open(files, "drop");
      } else if (files.length && open) {
        e.preventDefault();
        open([files[0]], "drop");
      }
    },
  };
}
