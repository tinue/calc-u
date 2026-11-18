// Host keys: library/web/components/hostKeys.js under
// `node --test` (CTest LibraryWebNodeTests, Frequent); the Qt6 twin's cases
// are library/qt6/tests/host_keys_tests.cpp.
const test = require("node:test");
const assert = require("node:assert/strict");
const k = require("../components/hostKeys.js");

const ev = (code, key, extra) => ({ code, key, shiftKey: false, altKey: false, ctrlKey: false, metaKey: false, ...extra });

test("web_key_event_produced_and_carried", () => {
  assert.deepEqual(k.calcuHostKeyEvent(ev("KeyA", "A", { shiftKey: true }), false), {
    physical: "KeyA", named: "", produced: 65, carried: 97, shift: true, alt: false, "mac-host": false,
    command: false, control: false, meta: false,
  });
  const digit = k.calcuHostKeyEvent(ev("Digit1", "1"), false);
  assert.equal(digit.produced, 49);
  assert.equal(digit.carried, 0);
  const esc = k.calcuHostKeyEvent(ev("Escape", "Escape"), false);
  assert.equal(esc.named, "escape");
  assert.equal(esc.produced, 0);
  assert.equal(k.calcuHostKeyEvent(ev("NumpadEnter", "Enter"), false).named, "enter");
  assert.equal(k.calcuHostKeyEvent(ev("Enter", "Enter"), false).named, "return");
});

test("web_modifiers_by_what_was_pressed", () => {
  const mac = k.calcuHostKeyEvent(ev("KeyC", "c", { metaKey: true, ctrlKey: true }), true);
  assert.equal(mac.command, true);
  assert.equal(mac.control, true);
  assert.equal(mac.meta, false);
  assert.equal(mac["mac-host"], true);
  const pc = k.calcuHostKeyEvent(ev("KeyC", "c", { metaKey: true }), false);
  assert.equal(pc.command, false);
  assert.equal(pc.meta, true);
});

test("web_host_keys_pair_down_and_up", () => {
  const keys = k.calcuHostKeys(false);
  assert.equal(keys.down(ev("KeyA", "a")).physical, "KeyA");
  assert.equal(keys.down(ev("KeyA", "a")), null);   // held already
  assert.deepEqual(keys.up(ev("KeyA", "A", { shiftKey: true })), { physical: "KeyA" });
  assert.equal(keys.up(ev("KeyA", "a")), null);     // once
});

test("web_host_keys_forgotten_on_focus_lost", () => {
  const keys = k.calcuHostKeys(false);
  assert.equal(keys.focusLost(), false);
  keys.down(ev("KeyB", "b"));
  assert.equal(keys.focusLost(), true);
  assert.equal(keys.up(ev("KeyB", "b")), null);
  assert.equal(keys.focusLost(), false);
});
