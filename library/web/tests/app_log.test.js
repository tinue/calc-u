// The Web's app log, library/web/src/appLog.js, and its route through
// shell.js under `node --test` (CTest LibraryWebNodeTests, Frequent): the
// page's own lines filtered by `?app-log-level=`, the worker's taken as they
// come, the kept lines and their form, a failure's one error line. The files
// are run as the page runs them, classic scripts, with a worker that only
// records and replays.
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const SRC = path.join(__dirname, "..", "src");
const read = (f) => fs.readFileSync(path.join(SRC, f), "utf8");

// A page at `search` with the app log (and, with `shell`, shell.js and its
// worker); `printed` holds what reached the console, by level.
function page(search, { shell = false } = {}) {
  const printed = [];
  const out = (level) => (text) => printed.push({ level, text });
  let worker = null;
  const ctx = vm.createContext({
    location: { search: search || "" },
    console: { error: out("error"), warn: out("warn"), info: out("info"), debug: out("debug"), log: out("log") },
    Worker: function Worker(url) { worker = this; this.url = url; this.posted = []; this.postMessage = (m) => this.posted.push(m); this.terminate = () => {}; },
    calcuWebHostServices: () => ({ names: [], answer: () => ({ done: false }) }),
    calcuCreateAudio: () => ({ message: () => {}, unlock: () => {}, close: () => {} }),
    CALC_BASE: "",
  });
  for (const f of shell ? ["sentences.js", "appLog.js", "shell.js"] : ["appLog.js"]) vm.runInContext(read(f), ctx);
  const s = shell ? ctx.createShell({ product: "Test", start: {} }) : null;
  const fromWorker = (data) => worker.onmessage({ data });
  return { ctx, printed, shell: s, fromWorker, worker: () => worker };
}
const kept = (p) => p.ctx.calcuAppLogText().split("\n").filter(Boolean);

test("page_lines_are_warn_and_error_by_default", () => {
  const p = page("");
  for (const level of ["error", "warn", "info", "debug"]) p.ctx.calcuAppLog(level, `a ${level} line`);
  assert.deepEqual(p.printed.map((x) => x.level), ["error", "warn"]);
  assert.equal(kept(p).length, 2);
});

test("page_lines_follow_the_urls_level", () => {
  const p = page("?x=1&app-log-level=debug");
  for (const level of ["error", "warn", "info", "debug"]) p.ctx.calcuAppLog(level, `a ${level} line`);
  assert.deepEqual(p.printed.map((x) => x.level), ["error", "warn", "info", "debug"]);
  const e = page("?app-log-level=error");
  e.ctx.calcuAppLog("warn", "not kept");
  e.ctx.calcuAppLog("error", "kept");
  assert.deepEqual(e.printed.map((x) => x.text), ["[Calc-U] kept"]);
  // Another value is no level: the default holds.
  const bad = page("?app-log-level=loud");
  bad.ctx.calcuAppLog("info", "not kept");
  assert.equal(bad.printed.length, 0);
});

test("worker_lines_are_taken_as_they_come", () => {
  const p = page("");
  p.ctx.calcuAppLog("debug", "filtered in the worker", true);
  assert.deepEqual(p.printed, [{ level: "debug", text: "[Calc-U] filtered in the worker" }]);
});

test("kept_lines_have_stamp_level_and_text", () => {
  const p = page("?app-log-level=info");
  p.ctx.calcuAppLog("info", "one");
  p.ctx.calcuAppLog("nonsense", "an unknown level is a warning");
  const [a, b] = kept(p);
  assert.match(a, /^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\.\d{3} INFO  one$/);
  assert.match(b, /^\S+ WARN  an unknown level is a warning$/);
  assert.equal(p.ctx.calcuAppLogText().endsWith("\n"), true);
  assert.equal(page("").ctx.calcuAppLogText(), "");
});

test("multi_line_text_is_indented", () => {
  const p = page("");
  p.ctx.calcuAppLog("error", "first\r\nsecond\nthird");
  assert.match(p.ctx.calcuAppLogText(), /ERROR first\n  second\n  third\n$/);
  assert.equal(p.printed[0].text, "[Calc-U] first\n  second\n  third");
  // As applog::formatLine: no trailing spaces or line breaks.
  p.ctx.calcuAppLog("error", "ends here \n\n");
  p.ctx.calcuAppLog("error", "");
  assert.match(p.ctx.calcuAppLogText(), /ERROR ends here\n\S+ ERROR\n$/);
});

test("the_ring_keeps_the_last_2000_lines", () => {
  const p = page("");
  for (let i = 0; i < 2005; i++) p.ctx.calcuAppLog("warn", `line ${i}`);
  const lines = kept(p);
  assert.equal(lines.length, 2000);
  assert.match(lines[0], / line 5$/);
  assert.match(lines[1999], / line 2004$/);
});

test("error_text_names_the_error", () => {
  const { ctx } = page("");
  assert.equal(ctx.calcuErrorText({ name: "NotAllowedError", message: "Write permission denied." }),
    "NotAllowedError: Write permission denied.");
  assert.equal(ctx.calcuErrorText({ name: "ErrnoError", message: "FS error", errno: 44 }), "ErrnoError: FS error (errno 44)");
  assert.equal(ctx.calcuErrorText("plain"), "plain");
});

test("shell_routes_the_workers_lines", () => {
  const p = page("", { shell: true });
  p.fromWorker({ kind: "app-log", level: "info", text: "the session's line" });
  assert.deepEqual(p.printed, [{ level: "info", text: "[Calc-U] the session's line" }]);
  assert.match(kept(p)[0], /INFO  the session's line$/);
});

test("shell_passes_the_level_to_the_worker", () => {
  assert.equal(page("?app-log-level=debug", { shell: true }).worker().url, "calc-engine-worker.js?app-log-level=debug");
  assert.equal(page("", { shell: true }).worker().url, "calc-engine-worker.js");
});

test("a_failure_is_one_error_line_with_its_detail", () => {
  const p = page("", { shell: true });
  p.fromWorker({ kind: "state", topic: "web.start", value: { state: "failed", failure: "files.json could not be fetched (404)",
    detail: "while starting\nError: at fetchBytes" } });
  assert.deepEqual(p.printed.map((x) => x.level), ["error"]);
  assert.equal(p.printed[0].text, "[Calc-U] Test could not start: files.json could not be fetched (404)\n  while starting\n  Error: at fetchBytes");
  assert.equal(p.shell.getState().failure, "Test could not start: files.json could not be fetched (404)");
  // A second failure adds nothing.
  p.fromWorker({ kind: "state", topic: "web.start", value: { state: "failed", failure: "again" } });
  assert.equal(p.printed.length, 1);
});

test("a_failure_once_running_is_a_stop", async () => {
  const p = page("", { shell: true });
  p.fromWorker({ kind: "state", topic: "web.start", value: { state: "running", failure: "" } });
  p.fromWorker({ kind: "state", topic: "web.start", value: { state: "failed", failure: "the program stopped (abort)" } });
  assert.equal(p.shell.getState().failure, "Test stopped: the program stopped (abort)");
  assert.equal(p.printed[p.printed.length - 1].text, "[Calc-U] Test stopped: the program stopped (abort)");
  const answer = await p.shell.send("reset");
  assert.equal(answer.reason, "The product stopped.");
});

// In the worker the line is filtered by the worker's URL and posted to the page.
test("worker_lines_are_posted_to_the_page", () => {
  const posted = [];
  function WorkerGlobalScope() {}
  const self = Object.create(WorkerGlobalScope.prototype);
  const ctx = vm.createContext({ WorkerGlobalScope, self, location: { search: "?app-log-level=info" },
    postMessage: (m) => posted.push(m), console: { warn: () => assert.fail("no console in the worker") } });
  vm.runInContext(read("appLog.js"), ctx);
  ctx.calcuAppLog("info", "storage: kept");
  ctx.calcuAppLog("debug", "not at info");
  assert.deepEqual(posted.map((m) => ({ ...m })), [{ kind: "app-log", level: "info", text: "storage: kept" }]);
});
