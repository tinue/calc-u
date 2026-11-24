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
//   s.menus, s.bar, s.panels, s.dialog (the settings rows), s.about (the About
//   dialog: shell::about with the page's rows and its Display facts, measured
//   when About opens, and its Copy `text`), s.loading, s.notice,
//   s.started, s.romsMissing, s.open ("settings" | "about" | "saved-media" |
//   "name-and-save" | null), s.naming (the place Name & Save is open for),
//   s.nameAndSavePrompts ({place: prompt}, the models' nameAndSave)
//   s.choose(item), s.setSetting(section, key, value), s.closeNotice(yes), s.close()
//   s.chooseStartupPreset(model): the settings row's "Choose…", the preset
//   picked with its files (`choose-startup-preset`: checked, then kept)
//   s.notify(text): a notice of the page's own, queued with the session's
//   s.nameAndSave(place): the Name & Save dialog of a media place (ShellDialogs),
//   for the place's submenu, the slot pickers' buttons and the product's own
//   media controls
//
// `product` is the page's form of the product's data (`*.shell-product`):
// its name, help address, the page's `version`, `build` and `buildTime`
// (the worker makes About of them), `product.loadPreset` (optional) in
// place of the shell's Load Preset… (a product that opens the picked files
// through the session's `open`), `copyScreen`, `nameAndSave` ({slotButtons}:
// ShellProduct::nameAndSave) and its items' acts by id. The Name & Save
// prompts by place are the models' (`s.nameAndSavePrompts`).

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
const CALCU_SHELL_NO_MODELS = { menus: [], bar: [], panels: [], dialog: [], about: null, nameAndSave: {} };

function calcuShellNow() {
  return typeof performance !== "undefined" ? performance.now() : Date.now();
}

function useCalcuShell(shell, product, traits, shellArgs) {
  const t = Object.assign({}, CALCU_SHELL_WEB_TRAITS, traits || {});
  const host = useShellHostRows();
  // About's Display section: the screen and the window as they are when
  // About opens (measured again at each open, not at each resize).
  const [display, setDisplay] = React.useState(null);
  const args = Object.assign({ mobile: !!t.mobile, settings: t.settings !== false, about: t.about !== false,
    version: product.version || "", build: product.build || "", buildTime: product.buildTime || "", host },
  display ? { display } : {}, shellArgs || {});
  const shellKey = JSON.stringify(args);
  const controlState = useTopic(shell, "control-state");
  const busy = useTopic(shell, "busy");
  const runState = useTopic(shell, "run-state");
  const romsMissing = useTopic(shell, "roms-missing");
  const settings = useTopic(shell, "settings");
  const media = useTopic(shell, "media");
  const models = useTopic(shell, "web.shell") || CALCU_SHELL_NO_MODELS;
  const [open, setOpen] = React.useState(null);
  const [naming, setNaming] = React.useState(null);
  const [, setVersion] = React.useState(0);
  const rerender = React.useCallback(() => setVersion((v) => v + 1), []);
  const notices = React.useRef(null);
  if (!notices.current) notices.current = calcuShellNotices();
  const loading = React.useRef(null);
  if (!loading.current) loading.current = calcuShellLoading();

  // The worker computes the models for this page's traits, build and rows
  // of where it runs (sent again when the browser's hints come).
  React.useEffect(() => {
    if (shell && shell.setShell) shell.setShell(args);
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
  const { menus, bar, panels, dialog, about } = models;
  const nameAndSavePrompts = models.nameAndSave || {};

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
  // The preset with every file it names, or alone when they were chosen
  // before (the session keeps them in its file cache).
  const chooseStartupPreset = (model) => {
    calcuPickFiles(product.presetUploads || "", true).then(async (files) => {
      if (!files.length) return;
      sendChecked("choose-startup-preset", { model, files: await calcuNamedFiles(files) });
    });
  };
  const nameAndSave = (place) => {
    setNaming(place);
    setOpen("name-and-save");
  };
  const choose = (item) => {
    if (!item || item.enabled === false) return;
    if (item.command) { sendChecked(item.command, item.args); return; }
    const a = CALCU_SHELL_ACTIONS;
    if (item.id === a.loadPreset) (product.loadPreset || loadPreset)();   // a product may open the files itself
    else if (item.id === a.settings) setOpen("settings");
    else if (item.id === a.about) { setDisplay(calcuShellDisplayFacts(window)); setOpen("about"); }
    else if (item.id === a.savedMedia) setOpen("saved-media");
    else if (item.id.startsWith(a.nameAndSavePrefix)) nameAndSave(item.args.place);
    else if (item.id === a.help) calcuWebHostServices().answer("open-url", { url: product.helpUrl });   // its result is ignored
    else if (item.id === a.showAppLog) {   // a page writes no file: the kept lines as a download
      calcuDownload(`${product.name || "Calc-U"}.log`, new TextEncoder().encode(calcuAppLogText()), "text/plain");
    }
    else if (item.id === a.copyScreen) { if (product.copyScreen) product.copyScreen(); }
    else if (item.id === a.pasteText) {
      const read = typeof navigator !== "undefined" && navigator.clipboard ? navigator.clipboard.readText() : Promise.resolve("");
      read.then((text) => sendChecked("paste", { text }),
        (e) => calcuAppLog("warn", `Paste: the clipboard could not be read: ${calcuErrorText(e)}`));
    } else {
      const act = (product.acts || {})[item.id];   // a product item acts
      if (act) act();
    }
  };
  const current = notices.current.current();
  return {
    shell, product, traits: t, inputs, menus, bar, panels, dialog, about, open, naming, nameAndSavePrompts,
    started: !!controlState, romsMissing: romsMissing || null,
    loading: loading.current.shown(calcuShellNow()),
    notice: current,
    choose, sendChecked, nameAndSave, chooseStartupPreset,
    setSetting: (section, key, value) => sendChecked("set-setting", { section, key, value }),
    closeNotice: (yes) => {
      const again = notices.current.answer(!!yes);
      if (again) sendChecked(again.name, again.args);
      rerender();
    },
    notify: (text) => {
      notices.current.push(text);
      rerender();
    },
    close: () => setOpen(null),
  };
}

// About's rows of where the page runs: at once from the user agent string,
// then from Chromium's client hints when they come (the rows are the last
// part of the dialog, so nothing above them moves).
function useShellHostRows() {
  const [rows, setRows] = React.useState(() => (typeof window === "undefined" ? [] : calcuShellHostRows(window.navigator, null, window.location)));
  React.useEffect(() => {
    const hints = typeof window === "undefined" ? null : window.navigator.userAgentData;
    if (!hints || typeof hints.getHighEntropyValues !== "function") return undefined;
    let live = true;
    hints.getHighEntropyValues(["platformVersion", "architecture", "bitness", "model", "fullVersionList"])
      .then((h) => { if (live) setRows(calcuShellHostRows(window.navigator, h, window.location)); })
      .catch(() => {});
    return () => { live = false; };
  }, []);
  return rows;
}
