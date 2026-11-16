// useCalcuShell -- the shell's state for a product component of the React
// set (LIB-14 design §2.3, §3.10): the models of shellModel.js drawn from the
// topics, and the senders. (The design's `useShell`: that name is LIB-15's
// hook that creates the shell, src/embed.js, so this one carries the set's
// prefix, D-17.)
//
//   const s = useCalcuShell(shell, product, {hasQuit: false, shortcuts: false, savedMedia: true});
//   s.menus, s.bar, s.panels, s.dialog (the settings rows), s.loading, s.notice,
//   s.started, s.romsMissing, s.open ("settings" | "about" | "saved-media" | null)
//   s.choose(item), s.setSetting(section, key, value), s.closeNotice(yes), s.close()
//
// `product` is the JS form of ShellProduct (shellModel.js); `product.loadPreset`
// (optional) replaces the shell's Load Preset… (a product that opens the picked
// files through LIB-13's `open`); `product.settingsSpec`
// (optional) is the dialog's spec, else the family sections of the rows the
// `settings` topic has; `product.keysShown` limits them.

const CALCU_SHELL_WEB_TRAITS = { hasQuit: false, shortcuts: false, savedMedia: true };

function calcuShellNow() {
  return typeof performance !== "undefined" ? performance.now() : Date.now();
}

function useCalcuShell(shell, product, traits) {
  const t = traits || CALCU_SHELL_WEB_TRAITS;
  const controlState = useTopic(shell, "control-state");
  const busy = useTopic(shell, "busy");
  const runState = useTopic(shell, "run-state");
  const romsMissing = useTopic(shell, "roms-missing");
  const settings = useTopic(shell, "settings");
  const media = useTopic(shell, "media");
  const started = useShellState(shell, (st) => st.start);
  const [presetList, setPresetList] = React.useState(null);
  const [open, setOpen] = React.useState(null);
  const [, setVersion] = React.useState(0);
  const rerender = React.useCallback(() => setVersion((v) => v + 1), []);
  const notices = React.useRef(null);
  if (!notices.current) notices.current = calcuShellNotices();
  const loading = React.useRef(null);
  if (!loading.current) loading.current = calcuShellLoading();

  const askPresets = React.useCallback(() => {
    if (!shell) return;
    shell.request("preset-list").then((r) => setPresetList(r.done ? r.value : null));
  }, [shell]);
  React.useEffect(() => { if (started === "running") askPresets(); }, [started, askPresets]);

  // AC-17: each notice, in order; none lost.
  React.useEffect(() => {
    if (!shell || !shell.listenStream) return undefined;
    return shell.listenStream((m) => {
      if (m.topic !== "notices") return;
      for (const n of m.value || []) notices.current.push((n && n.sentence) || "");
      if ((m.value || []).length) rerender();
    });
  }, [shell, rerender]);

  // L1: the rule on the page's clock; a timer looks again when it is due.
  const runs = !!(busy && busy.operationRuns);
  const full = !!(busy && busy.speed === "full");
  React.useEffect(() => {
    loading.current.busy(runs, full, calcuShellNow());
    rerender();
    const due = loading.current.due();
    if (due === null) return undefined;
    const id = setTimeout(rerender, Math.max(0, due - calcuShellNow()) + 5);
    return () => clearTimeout(id);
  }, [runs, full, rerender]);

  const inputs = { controlState: controlState || null, busy: busy || null, presetList, runState: runState || null,
    romsMissing: romsMissing || null, settings: settings || null, media: media || null };
  const menus = React.useMemo(() => calcuShellMenus(inputs, product, t),
    [controlState, presetList, runState, media, product, t]);
  const bar = React.useMemo(() => calcuShellBar(inputs, product), [controlState, product]);
  const panels = React.useMemo(() => calcuShellPanels(inputs), [controlState]);
  const spec = product.settingsSpec || calcuShellFamilySections((settings && settings.rows) || [], product.keysShown);
  const dialog = calcuShellDialog(settings || null, presetList, spec);

  const sendChecked = (name, args) => {
    if (!shell) return Promise.resolve({ done: false });
    return shell.send(name, args).then((r) => {
      if (!r.done) {
        notices.current.refusal(r.reason, { name, args });   // D-7, N4
        rerender();
      }
      return r;
    });
  };
  const loadPreset = () => {   // the page has no files of its own: the preset's text (LIB-6 `content`)
    calcuPickFiles(product.presetAccept || "", false).then(async (files) => {
      if (!files.length) return;
      const text = await files[0].text();
      sendChecked("load-preset", { content: text, name: files[0].name });
    });
  };
  const choose = (item) => {
    if (!item || item.enabled === false) return;
    if (item.command) { sendChecked(item.command, item.args); return; }
    const a = CALCU_SHELL_ACTIONS;
    if (item.id === a.loadPreset) (product.loadPreset || loadPreset)();   // a product may open the files itself (LIB-13)
    else if (item.id === a.settings) { askPresets(); setOpen("settings"); }
    else if (item.id === a.about) setOpen("about");
    else if (item.id === a.savedMedia) setOpen("saved-media");
    else if (item.id === a.help) calcuWebHostServices().answer("open-url", { url: product.helpUrl });   // EC-6: its result is ignored
    else if (item.id === a.copyScreen) { if (product.copyScreen) product.copyScreen(); }
    else if (item.id === a.pasteText) {
      const read = typeof navigator !== "undefined" && navigator.clipboard ? navigator.clipboard.readText() : Promise.resolve("");
      read.then((text) => sendChecked("paste", { text }), () => {});
    } else {
      const own = (product.items || []).find((p) => p.id === item.id);   // AC-7
      if (own && own.act) own.act();
    }
  };
  const current = notices.current.current();
  return {
    shell, product, traits: t, inputs, menus, bar, panels, dialog, open,
    started: !!controlState, romsMissing: romsMissing || null,
    loading: loading.current.shown(calcuShellNow()),
    notice: current,
    choose, sendChecked,
    setSetting: (section, key, value) => sendChecked("set-setting", { section, key, value }),
    closeNotice: (yes) => {
      const again = notices.current.answer(!!yes);
      if (again) sendChecked(again.name, again.args);
      rerender();
    },
    close: () => setOpen(null),
  };
}
