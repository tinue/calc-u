// The stream topics of library/web/src/shell.js and calcuStreamAfter
// (src/embed.js) under `node --test` (CTest LibraryWebNodeTests, Frequent):
// a place in a stream kept by its last item taken holds over the cap of
// CALCU_STREAM_KEEP items and over a reset. Both files are run as the page
// runs them, classic scripts, with a worker that only records and replays.
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const SRC = path.join(__dirname, "..", "src");

function page() {
  let worker = null;
  const ctx = vm.createContext({
    Worker: function Worker() { worker = this; this.postMessage = () => {}; this.terminate = () => {}; },
    calcuWebHostServices: () => ({ names: [], answer: () => ({ done: false }) }),
    calcuCreateAudio: () => ({ message: () => {}, unlock: () => {}, close: () => {} }),
    CALCU_SENTENCES: { startFailed: () => "failed" },
    CALC_BASE: "",
  });
  for (const f of ["appLog.js", "shell.js", "embed.js"]) vm.runInContext(fs.readFileSync(path.join(SRC, f), "utf8"), ctx);
  const shell = ctx.createShell({ product: "Test", start: {} });
  const stream = (topic, value, reset) => worker.onmessage({ data: { kind: "stream", topic, value, reset } });
  const items = (topic) => shell.getState().streams[topic] || [];
  return { ctx, shell, stream, items, keep: vm.runInContext("CALCU_STREAM_KEEP", ctx) };
}
// Items as they come over postMessage: a fresh object each.
const lines = (from, n) => Array.from({ length: n }, (_, i) => ({ text: `${from + i}` }));
const texts = (items) => Array.from(items, (i) => i.text);   // an array of this realm

test("stream_after_nothing_taken_is_all", () => {
  const p = page();
  p.stream("t", lines(0, 3));
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), null)), ["0", "1", "2"]);
});

test("stream_after_the_marker", () => {
  const p = page();
  p.stream("t", lines(0, 3));
  const marker = p.items("t")[2];   // Clear: the last item cleared
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), []);
  p.stream("t", lines(3, 2));
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), ["3", "4"]);
});

// The stream keeps its last CALCU_STREAM_KEEP items: a count would point past
// them for good; the marker still finds its place, then (dropped off the
// front) leaves every item kept, all of them newer.
test("stream_after_the_marker_over_the_cap", () => {
  const p = page();
  p.stream("t", lines(0, p.keep));
  assert.equal(p.items("t").length, p.keep);
  const marker = p.items("t")[p.keep - 1];
  p.stream("t", lines(p.keep, 2));
  assert.equal(p.items("t").length, p.keep);
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), [`${p.keep}`, `${p.keep + 1}`]);
  p.stream("t", lines(p.keep + 2, p.keep));
  const after = p.ctx.calcuStreamAfter(p.items("t"), marker);
  assert.equal(after.length, p.keep);
  assert.equal(after[0].text, `${p.keep + 2}`);
});

// One item taken at a time (the opened programs): every item is taken once,
// over the cap too.
test("stream_taken_one_by_one_over_the_cap", () => {
  const p = page();
  let taken = null;
  const seen = [];
  for (let n = 0; n < p.keep + 20; ++n) {
    p.stream("t", lines(n, 1));
    const fresh = p.ctx.calcuStreamAfter(p.items("t"), taken);
    assert.equal(fresh.length, 1);
    taken = fresh[0];
    seen.push(taken.text);
  }
  assert.equal(seen.length, p.keep + 20);
  assert.equal(seen[seen.length - 1], `${p.keep + 19}`);
});

// A reset starts the stream anew: what comes after it is shown, the marker
// is gone with the items it was among.
test("stream_after_a_reset", () => {
  const p = page();
  p.stream("t", lines(0, 3));
  const marker = p.items("t")[2];
  p.stream("t", [], true);
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), []);
  p.stream("t", lines(0, 2));
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), ["0", "1"]);
  p.stream("t", lines(10, 1), true);
  assert.deepEqual(texts(p.ctx.calcuStreamAfter(p.items("t"), marker)), ["10"]);
});
