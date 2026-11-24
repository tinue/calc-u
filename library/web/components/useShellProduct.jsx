// A product page's glue to its session, the same for both products' pages.
//
//   useShellProduct(shell, {request, args, fallback, version, build, buildTime,
//                   copyScreen, openFiles, acts}) → {product, ready, chrome}
//     the product's data: `request` (`<prefix>.shell-product`) with
//     `args`, once per distinct `args` while the session runs, its answer
//     {product, chrome} kept; `fallback` (the shell's frame with nothing of
//     the product's) until it came. Either carries the page's `version`,
//     `build` and `buildTime`; the answer's also Copy Screen (`copyScreen`, or null:
//     none), Load Preset… (the files picked by the product's
//     `presetUploads`: the preset with the files it names, through
//     `openFiles(files, "dialog")`) and the items' acts (`acts`, by id); its
//     `nameAndSave` the shell carries out. copyScreen, openFiles and acts are read
//     when used, so they may be new closures at each render. `chrome`: the
//     answer's chrome (the product's app/ Chrome in shell/Chrome's form),
//     null until it came.
//   useColourScheme() → true while the system's appearance is dark
//   calcuChromeStyle(chrome, dark) → the style object of the client's root:
//     every colour of the set for the appearance as `--calcu-<name>`, the
//     sizes as `--calcu-panel-header-height`, `--calcu-panel-buttons-height`,
//     `--calcu-panel-inset`, `--calcu-pill-height`, `--calcu-pill-radius`,
//     `--calcu-pill-padding`, `--calcu-pill-font-size` and
//     `--calcu-pill-font-weight` (only when set), `--calcu-bar-height`,
//     `--calcu-bar-gap`, `--calcu-slot-picker-width` (only when set),
//     `--calcu-printer-min-width`, `--calcu-printer-max-width` (only when
//     set); {} without chrome (the CSS's neutral fallbacks then).
//   useFaceplateMaps(shell, request, model) → {[model]: map}
//     the faceplate maps, one `request` {model} per model
//   calcuOpenFiles(send, files, way) → the reply, or null without files
//     the session's `open {files: [{name, bytes}], way, upload: true}` (several
//     files are one folder in /uploads); `send` is the page's sender
//     (the shell's, or useCalcuShell's sendChecked to show a refusal)
//   calcuFileDrop(open) → {onDragOver, onDrop}
//     a drop of files: `open(files, "drop")` (null: none yet) for a drop
//     of exactly one file; a drop of more is ignored, and no drop reaches
//     the browser (one rule for both products)

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
    const stamp = { version: options.version, build: options.build, buildTime: options.buildTime || "" };
    if (!data) return { ...options.fallback, ...stamp };
    const p = data.product;
    return {
      ...p,
      ...stamp,
      copyScreen: hasCopyScreen ? () => latest.current.copyScreen() : null,
      loadPreset: async () => {
        const files = await calcuPickFiles(p.presetUploads || "", true);
        if (files.length) latest.current.openFiles(files, "dialog");
      },
      acts: Object.fromEntries(p.items.map((i) => [i.id, () => { const act = (latest.current.acts || {})[i.id]; if (act) act(); }])),
    };
  }, [data, hasCopyScreen]);
  return { product, ready: !!data, chrome: (data && data.chrome) || null };
}

function useColourScheme() {
  const query = React.useMemo(() => (window.matchMedia ? window.matchMedia("(prefers-color-scheme: dark)") : null), []);
  const [dark, setDark] = React.useState(!!(query && query.matches));
  React.useEffect(() => {
    if (!query) return undefined;
    const changed = (e) => setDark(e.matches);
    query.addEventListener("change", changed);
    return () => query.removeEventListener("change", changed);
  }, [query]);
  return dark;
}

function calcuChromeStyle(chrome, dark) {
  if (!chrome) return {};
  const style = {};
  const colours = dark && chrome.dark ? chrome.dark : chrome.light;
  for (const name of Object.keys(colours || {})) style[`--calcu-${name}`] = colours[name];
  const s = chrome.sizes || {};
  const px = (v) => `${v}px`;
  style["--calcu-panel-header-height"] = px(s.panelHeaderHeight);
  style["--calcu-panel-buttons-height"] = px(s.panelButtonsHeight);
  style["--calcu-panel-inset"] = px(s.panelInset);
  style["--calcu-pill-height"] = px(s.pillHeight);
  style["--calcu-pill-radius"] = px(s.pillRadius);
  style["--calcu-pill-padding"] = `${px(s.pillPaddingY)} ${px(s.pillPaddingX)}`;
  if (s.pillFontSize) style["--calcu-pill-font-size"] = px(s.pillFontSize);
  if (s.pillBold) style["--calcu-pill-font-weight"] = "700";
  style["--calcu-bar-height"] = px(s.barHeight);
  style["--calcu-bar-gap"] = px(s.barGap);
  if (s.slotPickerEm) style["--calcu-slot-picker-width"] = `${s.slotPickerEm}em`;
  style["--calcu-paper-row-compact-width"] = String(s.paperRowCompactWidth);
  style["--calcu-paper-row-gap"] = px(s.paperRowGap);
  style["--calcu-paper-row-compact-gap"] = px(s.paperRowCompactGap);
  if (s.printerMaxWidth) {
    style["--calcu-printer-min-width"] = px(s.printerMinWidth);
    style["--calcu-printer-max-width"] = px(s.printerMaxWidth);
  }
  return style;
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

function calcuFileDrop(open) {
  return {
    onDragOver: (e) => { if (e.dataTransfer.types.includes("Files")) e.preventDefault(); },
    onDrop: (e) => {
      e.preventDefault();
      const files = [...e.dataTransfer.files];
      if (files.length === 1 && open) open(files, "drop");
    },
  };
}
