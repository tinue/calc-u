// What the page's components share: the library's shell (its useShell,
// its topics through useTopic) and the few things the page keeps itself.
// There is no translation layer: a component sends the library's commands and the
// product's (`c59.*`) by name and reads the topics as the session sends them.
//
//   createC59Traces() → {services, list, subscribe, download}
//       the page's host services for useShell (the library's, with the
//       traces the session hands over with keep-file kept: the RUN LOG tab
//       lists them; the worker keeps no copy). Every
//       keep-file is a download; only those of kind "trace" are listed (a
//       preset's screenshot and an exported medium are not)
//   C59_PAGE        the context: {shell, s (useCalcuShell), traces, log}
//   useLogView(shell) → {lines, clear}: the RUN LOG tab's view of the session's
//       `log`, its last 2000 lines; clear empties this view only

const PRODUCT = "Calc-U-59";
const C59_PAGE = React.createContext(null);
const C59_LOG_KEEP = 2000;

function createC59Traces() {
  const kept = new Map();
  const listeners = new Set();
  let list = [];
  const base = calcuWebHostServices();
  const services = {
    names: base.names,
    answer: (name, args) => {
      if (name === "keep-file" && args && args.kind === "trace") {
        const bytes = args.bytes || new Uint8Array(0);
        kept.set(args.name, bytes);
        list = list.filter((t) => t.name !== args.name).concat([{ name: args.name, size: bytes.length }]);
        listeners.forEach((l) => l());
      }
      return base.answer(name, args);
    },
  };
  return {
    services,
    list: () => list,
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
    download: (name) => { const b = kept.get(name); if (b) calcuDownload(name, b, "application/octet-stream"); },
  };
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
const cardReaderOf = (media) => calcuMediaPlace(media, "card-reader");
