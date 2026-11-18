// Host keys to the typing engine (the family's rule; the Qt6 twin is
// library/qt6/HostKeys): a browser key event as the
// command `host-key-down`, and the keys a page holds down, so each key-up
// pairs with its key-down and a focus loss lifts them all. Nothing is mapped
// here: the product's keyboard map in the session decides what a key types.
// The page keeps its product's own keys (a SHIFT tap) and
// which events are the browser's. No React, no DOM: loadable by
// `node --test` (library/web/tests/host_keys.test.js).

// KeyboardEvent.code of the keys a keyboard map may name (calcu::typing::NamedKey).
const CALCU_HOST_NAMED_KEYS = {
  Escape: "escape", Backspace: "backspace", Enter: "return", NumpadEnter: "enter", Tab: "tab", Delete: "delete",
  ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right",
  F1: "f1", F2: "f2", F3: "f3", F4: "f4", F5: "f5", F6: "f6",
  ScrollLock: "scroll-lock", Insert: "insert", Home: "home", End: "end", PageUp: "page-up", PageDown: "page-down",
};
const CALCU_HOST_IS_MAC = typeof navigator !== "undefined" && /Mac|iPhone|iPad|iPod/.test(navigator.platform || navigator.userAgent);

// The arguments of `host-key-down` for a browser key event: which key, which modifiers, the one character it produced (0: none
// or more), and for a letter the letter the key carries.
function calcuHostKeyEvent(e, isMac = CALCU_HOST_IS_MAC) {
  const chars = [...(e.key || "")];
  const produced = chars.length === 1 ? chars[0].codePointAt(0) : 0;
  const letter = chars.length === 1 && chars[0].toLowerCase() !== chars[0].toUpperCase()
    ? chars[0].toLowerCase().codePointAt(0) : 0;
  return {
    physical: e.code, named: CALCU_HOST_NAMED_KEYS[e.code] || "", produced, carried: letter,
    shift: !!e.shiftKey, alt: !!e.altKey, "mac-host": isMac,
    // By what the user pressed: on a Mac the browser's metaKey is Command.
    command: isMac ? !!e.metaKey : false, control: !!e.ctrlKey, meta: isMac ? false : !!e.metaKey,
  };
}

// The keys a page holds down (by KeyboardEvent.code); the page sends what
// these return.
//   down(e): the arguments of `host-key-down`, or null while the key is held already
//   up(e):   the arguments of `host-key-up` ({physical}) for a held key, or null
//            (the engine keeps a quick tap down long enough to be scanned)
//   focusLost(): whether a key was held (then the page sends `focus-lost`);
//            forgets them all
function calcuHostKeys(isMac = CALCU_HOST_IS_MAC) {
  const held = new Set();
  return {
    down(e) {
      if (held.has(e.code)) return null;
      held.add(e.code);
      return calcuHostKeyEvent(e, isMac);
    },
    up(e) {
      if (!held.has(e.code)) return null;
      held.delete(e.code);
      return { physical: e.code };
    },
    focusLost() {
      const any = held.size > 0;
      held.clear();
      return any;
    },
  };
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { CALCU_HOST_NAMED_KEYS, calcuHostKeyEvent, calcuHostKeys };
}
