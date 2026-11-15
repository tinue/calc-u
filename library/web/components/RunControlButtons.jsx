// CalcuRunControlButtons -- the run-control buttons of the React component
// set (LIB-10 design §3.10, D-9; spec AC-18; debugger.md DB-6): pause, step
// and resume for one level, fed from the session's `run-state` topic, the
// same rule as the C++ ButtonRule (library/debugger/ButtonRule.hpp) and the
// Qt6 widget; the table of its cases is library/debugger/tests/button_cases.json.
// The labels and the place are the product's.
//
//   <CalcuRunControlButtons shell={shell} level="cpu" | "program"
//                           labels={{pause, step, resume}?} send={fn?} />
//
// Pause can be used while the machine runs; step and resume while it is
// paused and this level owns the pause. Step fires on press and then every
// 250 ms while held, until release or until step cannot be used. send(command,
// args) defaults to shell.send. Elements carry data-calcu names for the
// tests: run-control, run-control-pause, run-control-step, run-control-resume.

const CALCU_STEP_REPEAT_MS = 250;   // four times a second (Calc-U-59 HoldRepeatButton.swift)

// The rule: which of the three buttons of `level` can be used in `state`
// (the run-state payload, or null before the first one).
function calcuButtonUse(level, state) {
  if (!state) return { pause: false, step: false, resume: false };
  const owns = !!state.paused && state.owner === level;
  return { pause: !state.paused, step: owns, resume: owns };
}

function CalcuRunControlButtons({ shell, level, labels, send }) {
  const state = useTopic(shell, "run-state");
  const use = calcuButtonUse(level, state || null);
  const sendTo = send || ((n, a) => (shell ? shell.send(n, a) : Promise.resolve({ done: false })));
  const words = Object.assign({ pause: "Pause", step: "Step", resume: "Resume" }, labels || {});
  const useRef = React.useRef(use);
  useRef.current = use;
  const timer = React.useRef(null);
  const held = React.useRef(false);

  const stopRepeat = () => {
    held.current = false;
    if (timer.current) clearInterval(timer.current);
    timer.current = null;
  };
  React.useEffect(() => stopRepeat, []);

  const step = () => sendTo("step", { level, count: 1 });
  const press = (e) => {
    if (!useRef.current.step) return;
    if (e && e.currentTarget && e.currentTarget.setPointerCapture && e.pointerId !== undefined) {
      try { e.currentTarget.setPointerCapture(e.pointerId); } catch (_) { /* synthetic events */ }
    }
    held.current = true;
    step();
    timer.current = setInterval(() => {
      // Until release, or until step cannot be used.
      if (!held.current || !useRef.current.step) { stopRepeat(); return; }
      step();
    }, CALCU_STEP_REPEAT_MS);
  };
  // A held step stays usable while its step runs (the run state says running then).
  const stepEnabled = use.step || held.current;

  return (
    <div className="calcu-run-control" data-calcu="run-control" data-level={level}>
      <button type="button" data-calcu="run-control-pause" disabled={!use.pause}
        onClick={() => { if (useRef.current.pause) sendTo("pause", { level }); }}>{words.pause}</button>
      <button type="button" data-calcu="run-control-step" disabled={!stepEnabled}
        onPointerDown={press} onPointerUp={stopRepeat} onPointerCancel={stopRepeat}
        onLostPointerCapture={stopRepeat}>{words.step}</button>
      <button type="button" data-calcu="run-control-resume" disabled={!use.resume}
        onClick={() => { if (useRef.current.resume) sendTo("resume", {}); }}>{words.resume}</button>
    </div>
  );
}
