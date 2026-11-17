// What the page's components share (M-57, LIB-14 59): the library's shell
// (LIB-15's createShell, its topics through useTopic) and the few things the
// page keeps itself. There is no translation layer any more (src/engine.js
// went in M-57): a component sends the library's commands and the
// product's (`c59.*`) by name and reads the topics as the session sends them.
//
//   createC59Shell({model, preset, workerUrl}) → {shell, traces}
//       the shell, started with LIB-18's `start {model, preset}` (the preset
//       as its bundle path `examples:<path>`, LIB-15 N3), and the traces the
//       session handed over with keep-file (LIB-10 AC-26, D-25: the LOG tab
//       lists them; the worker keeps no copy)
//   C59_PAGE        the context: {shell, s (useCalcuShell), traces, openFiles}
//   useLogView(shell) → {lines, clear}: the LOG tab's view of LIB-3's `log`,
//       its last 2000 lines (C59-4 AC-9); clear empties this view only

const PRODUCT = "Calc-U-59";
const C59_PAGE = React.createContext(null);
const C59_LOG_KEEP = 2000;

// "sum.ti59" → "examples:sum.ti59" (a path of bundle.yaml as its bundle path); empty stays empty.
function bundlePathOf(preset) {
  if (!preset) return "";
  return preset.startsWith("examples:") ? preset : `examples:${preset}`;
}

function createC59Shell({ model, preset, workerUrl }) {
  const kept = new Map();
  const listeners = new Set();
  let list = [];
  const base = calcuWebHostServices();
  const services = {
    names: base.names,
    answer: (name, args) => {
      if (name === "keep-file" && args) {   // the session hands over traces only
        const bytes = args.bytes || new Uint8Array(0);
        kept.set(args.name, bytes);
        list = list.filter((t) => t.name !== args.name).concat([{ name: args.name, size: bytes.length }]);
        listeners.forEach((l) => l());
      }
      return base.answer(name, args);
    },
  };
  const shell = createShell({ product: PRODUCT, workerUrl, start: { model: model || "", preset: bundlePathOf(preset) }, services });
  const traces = {
    list: () => list,
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
    download: (name) => { const b = kept.get(name); if (b) calcuDownload(name, b, "application/octet-stream"); },
  };
  return { shell, traces };
}

function useTraces(traces) {
  const t = traces || { list: () => [], subscribe: () => () => {} };
  return React.useSyncExternalStore(t.subscribe, t.list, t.list);
}

function useLogView(shell) {
  const [lines, setLines] = React.useState([]);
  React.useEffect(() => {
    if (!shell) return undefined;
    return shell.listenStream((m) => {
      if (m.topic !== "log") return;
      setLines((old) => {
        let out = m.reset ? [] : old;
        const add = [];
        for (const item of m.value || []) for (const line of String((item && item.text) || "").split("\n")) add.push(line);
        if (add.length) out = out.concat(add).slice(-C59_LOG_KEEP);
        return out;
      });
    });
  }, [shell]);
  return { lines, clear: () => setLines([]) };
}

// The card reader's place of the `media` topic, or null (a TI-58, TI-58C).
function cardReaderOf(media) {
  return ((media && media.places) || []).find((p) => p.place === "card-reader") || null;
}
