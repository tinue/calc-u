// useCalcuShell -- the shell's state for a product component of the React
// set: the models the worker serves (the library's C++ shell models, topic
// `web.shell`) and the senders.
// (Not `useShell`: that name is the hook that creates the shell, src/embed.js,
// so this one carries the set's prefix.)
//
//   const s = useCalcuShell(shell, product, {settings, about}, shellArgs);
//   (traits: `settings` false: no Settings…; `about` false: no About; the
//   page is mobile when CALCU_SHELL_MOBILE. shellArgs: the product's own
//   args of its shell, as its Web part reads them.)
//   s.menus, s.bar, s.panels, s.dialog (the settings rows), s.loading, s.notice,
//   s.started, s.romsMissing, s.open ("settings" | "about" | "saved-media" | null)
//   s.choose(item), s.setSetting(section, key, value), s.closeNotice(yes), s.close()
//
// `product` is the page's form of the product's data (`*.shell-product`):
// its name, help address and About, `product.loadPreset` (optional) in
// place of the shell's Load Preset… (a product that opens the picked files
// through the session's `open`), `copyScreen`, `nameAndSave(place)` and its
// items' acts by id.

// Whether the page is mobile, fixed while it is open: true for the
// installed app and for a browser that asks for the mobile site, else
// false (desktop). The browser's own choice decides (Safari's "Request
// Desktop Website", Chrome's "Desktop site" change what it reports), so a
// user who wants the desktop page on a phone gets it. `?mobile=1|0`
// overrides, for trying one on the other.
function calcuShellMobile() {
  // A server-side render (no browser around it) draws the desktop page.
  if (typeof window === "undefined" || typeof window.matchMedia !== "function" || !window.navigator) return false;
  const asked = /[?&]mobile=([01])\b/.exec((window.location && window.location.search) || "");
  if (asked) return asked[1] === "1";
  if (window.matchMedia("(display-mode: standalone)").matches) return true;
  if (window.navigator.standalone === true) return true;
  const hints = window.navigator.userAgentData;
  if (hints && typeof hints.mobile === "boolean") return hints.mobile;
  return /iPhone|iPod|Android.*Mobile|Mobi/.test(window.navigator.userAgent);
}
const CALCU_SHELL_MOBILE = calcuShellMobile();
const CALCU_SHELL_WEB_TRAITS = { mobile: CALCU_SHELL_MOBILE, settings: true, about: true };
const CALCU_SHELL_NO_MODELS = { menus: [], bar: [], panels: [], dialog: [] };

function calcuShellNow() {
  return typeof performance !== "undefined" ? performance.now() : Date.now();
}

function useCalcuShell(shell, product, traits, shellArgs) {
  const t = Object.assign({}, CALCU_SHELL_WEB_TRAITS, traits || {});
  const shellKey = JSON.stringify([t, shellArgs || {}]);
  const controlState = useTopic(shell, "control-state");
  const busy = useTopic(shell, "busy");
  const runState = useTopic(shell, "run-state");
  const romsMissing = useTopic(shell, "roms-missing");
  const settings = useTopic(shell, "settings");
  const media = useTopic(shell, "media");
  const models = useTopic(shell, "web.shell") || CALCU_SHELL_NO_MODELS;
  const [open, setOpen] = React.useState(null);
  const [, setVersion] = React.useState(0);
  const rerender = React.useCallback(() => setVersion((v) => v + 1), []);
  const notices = React.useRef(null);
  if (!notices.current) notices.current = calcuShellNotices();
  const loading = React.useRef(null);
  if (!loading.current) loading.current = calcuShellLoading();

  // The worker computes the models for this page's traits.
  React.useEffect(() => {
    if (shell && shell.setShell) shell.setShell(Object.assign({ mobile: !!t.mobile,
      settings: t.settings !== false, about: t.about !== false }, shellArgs || {}));
  }, [shell, shellKey]);

  // Each notice, in order; none lost.
  React.useEffect(() => {
    if (!shell || !shell.listenStream) return undefined;
    return shell.listenStream((m) => {
      if (m.topic !== "notices") return;
      for (const n of m.value || []) notices.current.push((n && n.sentence) || "");
      if ((m.value || []).length) rerender();
    });
  }, [shell, rerender]);

  // "Loading…": the rule on the page's clock; a timer looks again when it is due.
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

  const inputs = { controlState: controlState || null, busy: busy || null, runState: runState || null,
    romsMissing: romsMissing || null, settings: settings || null, media: media || null };
  const { menus, bar, panels, dialog } = models;

  const sendChecked = (name, args) => {
    if (!shell) return Promise.resolve({ done: false });
    return shell.send(name, args).then((r) => {
      if (!r.done) {
        notices.current.refusal(r.reason, { name, args });   // a refusal is a notice, or a question for "asks:"
        rerender();
      }
      return r;
    });
  };
  const loadPreset = () => {   // the page has no files of its own: the preset's text (`content`)
    calcuPickFiles(product.presetUploads || "", false).then(async (files) => {
      if (!files.length) return;
      const text = await files[0].text();
      sendChecked("load-preset", { content: text, name: files[0].name });
    });
  };
  const choose = (item) => {
    if (!item || item.enabled === false) return;
    if (item.command) { sendChecked(item.command, item.args); return; }
    const a = CALCU_SHELL_ACTIONS;
    if (item.id === a.loadPreset) (product.loadPreset || loadPreset)();   // a product may open the files itself
    else if (item.id === a.settings) setOpen("settings");
    else if (item.id === a.about) setOpen("about");
    else if (item.id === a.savedMedia) setOpen("saved-media");
    else if (item.id.startsWith("media-name-and-save-")) { if (product.nameAndSave) product.nameAndSave(item.args.place); }
    else if (item.id === a.help) calcuWebHostServices().answer("open-url", { url: product.helpUrl });   // its result is ignored
    else if (item.id === a.copyScreen) { if (product.copyScreen) product.copyScreen(); }
    else if (item.id === a.pasteText) {
      const read = typeof navigator !== "undefined" && navigator.clipboard ? navigator.clipboard.readText() : Promise.resolve("");
      read.then((text) => sendChecked("paste", { text }), () => {});
    } else {
      const act = (product.acts || {})[item.id];   // a product item acts
      if (act) act();
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
