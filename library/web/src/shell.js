// The page side of a Web frontend: starts the worker, keeps the last state of each state topic
// and what each stream topic appended, sends suspend and continue on
// visibilitychange, answers the host services and keeps the start state.
// Created in an effect only, never during a render (a server-side render
// starts no worker).
//
//   createShell({product, workerUrl, start: {model, preset}, services}) → {
//     getState(), subscribe(listener),         for useShellState() / useTopic()
//     send(command, args) → Promise<{done, reason, value}>,   a session command
//     request(name, args) → Promise<{done, reason, value}>,   a session request
//     frames,                                  the display topic, outside React
//     listenStream(listener) → unsubscribe,    every stream message as it comes,
//                                              {topic, reset, value: [item, ...]}
//                                              (a component that keeps more than
//                                              CALCU_STREAM_KEEP items: the paper)
//     unlockAudio(), destroy(),
//   }
//   state: {start: "starting" | "running" | "failed", failure (the sentence:
//           could not start, or stopped once running),
//           topics: {name → last state}, streams: {name → appended since the last reset}}
//
// Everything that crosses postMessage is a session message in its JS form
// (glue/MessageCodec.hpp); the start parameters go in the first command,
// `start`.

const CALCU_STREAM_KEEP = 500;   // items kept per stream topic
const CALCU_NO_SHELL_STATE = { start: "starting", failure: "", topics: {}, streams: {} };

function createShell({ product, workerUrl, start, services }) {
  let state = CALCU_NO_SHELL_STATE;
  const listeners = new Set();
  const streamListeners = new Set();
  const frames = { display: null, onFrame: null };
  let framePending = false;
  const waiters = new Map();
  let lastCall = 0;
  const offered = services || calcuWebHostServices();
  const audio = calcuCreateAudio();
  let worker = null;
  let destroyed = false;

  const set = (next) => {
    state = next;
    listeners.forEach((l) => l());
  };
  // The reason of a call that cannot be carried out: a failed start, or a stop once running.
  let unanswered = "The product could not start.";
  // `detail` goes to the app log only. A failure after the start is a stop.
  const failWith = (why, detail) => {
    if (state.start === "failed") return;
    const stop = state.start === "running";
    const failure = (stop ? CALCU_SENTENCES.stopped : CALCU_SENTENCES.startFailed)(product, why);
    calcuAppLog("error", `${failure}${detail ? `\n${detail}` : ""}`);
    if (stop) unanswered = "The product stopped.";
    set({ ...state, start: "failed", failure });
    waiters.forEach((w) => w({ done: false, reason: unanswered, value: null }));
    waiters.clear();
  };
  const post = (message, transfer) => {
    if (worker && !destroyed) worker.postMessage(message, transfer || []);
  };
  const call = (kind, name, args) => new Promise((resolve) => {
    if (state.start === "failed" || destroyed) {
      resolve({ done: false, reason: unanswered, value: null });
      return;
    }
    const id = ++lastCall;
    waiters.set(id, resolve);
    post({ kind, name, args: args === undefined ? null : args, call: id });
  });

  const onState = (m) => {
    if (m.topic === "display") {   // outside the store: a canvas repaints from frames
      frames.display = m.value;
      if (!framePending && typeof requestAnimationFrame !== "undefined") {
        framePending = true;
        requestAnimationFrame(() => {
          framePending = false;
          if (frames.onFrame) frames.onFrame();
        });
      }
      return;
    }
    if (m.topic === "web.pace") {   // the worker's pace, shown with ?pace=1 only
      calcuPaceOverlay(m.value, state.topics.busy);
      return;
    }
    if (m.topic === "web.start") {
      if (m.value && m.value.state === "failed") failWith(m.value.failure, m.value.detail);
      else if (state.start === "starting") set({ ...state, start: "running", topics: { ...state.topics, [m.topic]: m.value } });
      return;
    }
    set({ ...state, topics: { ...state.topics, [m.topic]: m.value } });
  };
  const onStream = (m) => {
    const before = m.reset ? [] : state.streams[m.topic] || [];
    set({ ...state, streams: { ...state.streams, [m.topic]: before.concat(m.value || []).slice(-CALCU_STREAM_KEEP) } });
    streamListeners.forEach((l) => l({ topic: m.topic, reset: !!m.reset, value: m.value || [] }));
  };
  const onAsk = (m) => {
    Promise.resolve()
      .then(() => offered.answer(m.name, m.value))
      .catch((e) => {
        const reason = String((e && e.message) || e);
        calcuAppLog("warn", `The host service ${m.name} failed: ${(e && e.name) || "Error"}: ${reason}`);
        return { done: false, reason, value: null };
      })
      .then((a) => {
        const value = a.value === undefined ? null : a.value;
        post({ kind: "answer", call: m.call, done: !!a.done, reason: a.reason || "", value });
      });
  };

  try {
    // The page's `?app-log-level=` holds for the worker's app log too.
    const level = calcuAppLogLevelOfPage();
    const url = workerUrl || `${CALC_BASE}calc-engine-worker.js`;
    worker = new Worker(level ? `${url}${url.includes("?") ? "&" : "?"}app-log-level=${level}` : url);
  } catch (e) {
    failWith(String((e && e.message) || e));
  }
  if (worker) {
    worker.onmessage = (e) => {
      const m = e.data;
      if (!m || destroyed) return;
      switch (m.kind) {
        case "state": onState(m); break;
        case "stream": onStream(m); break;
        case "reply": {
          const w = waiters.get(m.call);
          waiters.delete(m.call);
          if (w) w({ done: !!m.done, reason: m.reason || "", value: m.value === undefined ? null : m.value });
          break;
        }
        case "ask": onAsk(m); break;
        case "app-log": calcuAppLog(m.level, m.text, true); break;
        case "sound-start": case "sound": case "sound-stop": audio.message(m); break;
        default: break;
      }
    };
    // A script that cannot be fetched or throws, a message that cannot be read.
    worker.onerror = (e) => {
      if (e && e.preventDefault) e.preventDefault();
      const where = e && e.filename ? ` (${e.filename}:${e.lineno || 0}:${e.colno || 0})` : "";
      failWith(`${(e && e.message) || "the worker could not be started"}${where}`);
    };
    worker.onmessageerror = () => failWith("a message from the worker could not be read");
    post({ kind: "listen", args: { services: offered.names } });
    if (typeof location !== "undefined" && /[?&]pace=1\b/.test(location.search)) post({ kind: "pace" });   // the worker measures
    call("command", "start", { model: (start && start.model) || "", preset: (start && start.preset) || "" });
  }

  // Hidden → suspend, shown → continue.
  const onVisibility = () => post({ kind: "command", name: document.hidden ? "suspend" : "continue", args: null, call: 0 });
  if (typeof document !== "undefined") document.addEventListener("visibilitychange", onVisibility);

  return {
    frames,
    getState: () => state,
    subscribe: (l) => { listeners.add(l); return () => listeners.delete(l); },
    listenStream: (l) => { streamListeners.add(l); return () => streamListeners.delete(l); },
    send: (name, args) => call("command", name, args),
    request: (name, args) => call("request", name, args),
    // The page's traits and the product's args of its shell: the worker
    // posts the shell's models as the topic `web.shell` from then on.
    setShell: (args) => post({ kind: "shell", args }),
    unlockAudio: audio.unlock,
    destroy: () => {
      destroyed = true;
      if (typeof document !== "undefined") document.removeEventListener("visibilitychange", onVisibility);
      if (worker) worker.terminate();
      audio.close();
    },
  };
}

// `?pace=1`: a line at the top of the page with the worker's pace, which it
// measures only when the page asks (the message `pace`): how often its
// timer fires, the share of time it computes, the longest gap, and the
// speed, to see what a browser grants the worker.
function calcuPaceOverlay(p, busy) {
  if (typeof document === "undefined" || !/[?&]pace=1\b/.test(location.search)) return;
  let el = document.getElementById("calcu-pace");
  if (!el) {
    el = document.createElement("div");
    el.id = "calcu-pace";
    el.style.cssText = "position:fixed;left:0;top:0;z-index:99;padding:2px 6px;font:11px ui-monospace,monospace;" +
      "background:rgba(0,0,0,0.7);color:#fff;pointer-events:none";
    document.body.appendChild(el);
  }
  el.textContent = `ticks/s ${p.ticksPerSecond}  busy ${p.busyPercent}%  ` +
    `longest gap ${p.longestGapMs} ms  speed ${(busy && busy.speed) || "?"}`;
}

