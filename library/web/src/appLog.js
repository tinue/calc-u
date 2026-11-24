// The page's app log: how the app ran (library/applog). A page cannot write a
// file, so each line goes to the browser's console at its level and the last
// CALCU_APP_LOG_KEEP lines are kept for Help ▸ Show App Log (a download).
// The worker's lines arrive filtered at the session's level (the setting
// `[app-log] level`, or `?app-log-level=` for this run); the page's own
// lines are warn and error, plus info and debug when the URL asks for them.
//
//   calcuAppLog(level, text)   level: "error" | "warn" | "info" | "debug"
//   calcuAppLogText()          the kept lines, one per line, for the download
//   calcuAppLogLevelOfPage()   the `?app-log-level=` of the page's URL, or ""

const CALCU_APP_LOG_KEEP = 2000;
const CALCU_APP_LOG_LEVELS = ["error", "warn", "info", "debug"];
const calcuAppLogLines = [];

function calcuAppLogLevelOfPage() {
  if (typeof location === "undefined") return "";
  const m = /[?&]app-log-level=([a-z]+)/.exec(location.search);
  return m && CALCU_APP_LOG_LEVELS.includes(m[1]) ? m[1] : "";
}

function calcuAppLogStamp(d) {
  const p = (n, w) => String(n).padStart(w || 2, "0");
  return `${d.getFullYear()}-${p(d.getMonth() + 1)}-${p(d.getDate())}T${p(d.getHours())}:${p(d.getMinutes())}:` +
    `${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`;
}

// `fromWorker`: the line was filtered in the worker already.
function calcuAppLog(level, text, fromWorker) {
  const at = CALCU_APP_LOG_LEVELS.indexOf(level);
  const lvl = at < 0 ? "warn" : level;
  if (!fromWorker) {
    const page = CALCU_APP_LOG_LEVELS.indexOf(calcuAppLogLevelOfPage() || "warn");
    if (CALCU_APP_LOG_LEVELS.indexOf(lvl) > page) return;
  }
  const body = String(text).replace(/\r/g, "").replace(/\n/g, "\n  ");
  calcuAppLogLines.push(`${calcuAppLogStamp(new Date())} ${lvl.toUpperCase().padEnd(5)} ${body}`);
  if (calcuAppLogLines.length > CALCU_APP_LOG_KEEP) calcuAppLogLines.splice(0, calcuAppLogLines.length - CALCU_APP_LOG_KEEP);
  if (typeof console !== "undefined") {
    const out = console[lvl] || console.log;
    out.call(console, `[Calc-U] ${body}`);
  }
}

function calcuAppLogText() {
  return calcuAppLogLines.length ? `${calcuAppLogLines.join("\n")}\n` : "";
}
