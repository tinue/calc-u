// The audio sink on the page side: the session sends sound-start {value: rate}, then
// sound {value: Int16Array} as the emulation makes it, and sound-stop. An
// AudioWorklet (audio-worklet.js, beside calc.js) plays it from a ring
// buffer. Browsers start audio only from a user gesture, so the context is
// made on the first key or click (unlock()); until then the samples are
// dropped. What is heard is the session's decision; this only
// plays what comes.
function calcuCreateAudio() {
  let context = null;
  let node = null;
  let rate = 48000;
  let starting = null;
  let warned = false;   // once: every key would try again
  const warnOnce = (what, e) => {
    if (warned) return;
    warned = true;
    calcuAppLog("warn", `audio: the sound could not be ${what}: ${calcuErrorText(e)}`);
  };

  const unlock = () => {
    if (context || starting || typeof AudioContext === "undefined") {
      if (context && context.state === "suspended") context.resume().catch((e) => warnOnce("resumed", e));
      return;
    }
    const ctx = new AudioContext({ latencyHint: "interactive" });
    starting = ctx.audioWorklet.addModule(`${CALC_BASE}audio-worklet.js`).then(() => {
      node = new AudioWorkletNode(ctx, "calcu-buzzer", { outputChannelCount: [1] });
      node.connect(ctx.destination);
      node.port.postMessage({ rate });
      context = ctx;
    }).catch((e) => warnOnce("started", e)).finally(() => { starting = null; });
  };

  const message = (m) => {
    if (m.kind === "sound-start") {
      rate = m.value;
      if (node) node.port.postMessage({ rate });
    } else if (m.kind === "sound" && node && m.value) {
      node.port.postMessage({ pcm: m.value }, [m.value.buffer]);
    }
  };

  const close = () => {
    if (context) context.close();
    context = null;
    node = null;
  };

  return { unlock, message, close };
}
