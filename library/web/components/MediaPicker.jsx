// CalcuMediaPicker (and CalcuNameAndSaveDialog, CalcuSavedMediaDialog in the next two files) -- the
// media components of the React component set. They draw the topic `media`
// as it is; no rule lives here. The shell places them.
//
//   <CalcuMediaPicker place={placeState} send={fn} />
//     one control-bar picker from a place's entries (PlaceState of the topic
//     `media`): the empty place, the templates, the instances, the modules;
//     an entry's description is its tooltip; an entry that cannot be chosen
//     is disabled, "(outdated)" with its hint as the tooltip. Choosing
//     sends `media.choose {place, entry}`.
//   <CalcuNameAndSaveDialog place={placeState} prompt="Instance name:"
//                           send={fn} ask={fn?} onClose={fn} />
//     the name, and the write-protect switch where the medium has one still
//     off. A reply whose reason starts with "asks: " is a question:
//     on yes the same command goes again with confirmed: true.
//   <CalcuSavedMediaDialog media={mediaState} send={fn} ask={fn?} onClose={fn} />
//     the saved media by kind and name, a template and a medium in a place
//     marked; Download hands the file to the user (`media.export`,
//     the host's keep-file), Delete asks first (`media.delete`).
//
// send(command, args) returns a promise of the reply {done, reason}; ask(text)
// a promise of true or false (default: window.confirm). Elements carry
// data-calcu names for the tests: media-picker-<place>, media-name,
// media-protect, media-save, media-saved-<kind>-<name>, media-download,
// media-delete.
//
// The helpers (one prefix for every page):
//   CALCU_ASKS                   "asks: ", the start of a question's reason
//                                (C++ media::kAsks)
//   calcuAskedQuestion(reason)   the question of such a reason, else null
//   calcuSendAsking(send, ask, command, args)   the reply of a command that may
//                                ask first: on yes sent again with confirmed: true
//   calcuMediaPlace(media, word) the place of the topic `media` named `word`
//                                ("slot-1", "card-reader", "cassette"), or null

const CALCU_ASKS = "asks: ";
const CALCU_MEDIA_PROTECT = "Write-protect (switch to read-only, can't be undone)";

function calcuMediaAsk(ask, question) {
  if (ask) return Promise.resolve(ask(question));
  return Promise.resolve(typeof window !== "undefined" && window.confirm ? window.confirm(question) : false);
}

function calcuAskedQuestion(reason) {
  return typeof reason === "string" && reason.startsWith(CALCU_ASKS) ? reason.slice(CALCU_ASKS.length) : null;
}

// The reply of a command that may ask first: sent again confirmed on yes.
function calcuSendAsking(send, ask, command, args) {
  return Promise.resolve(send(command, args)).then((reply) => {
    const question = reply && !reply.done ? calcuAskedQuestion(reply.reason) : null;
    if (question !== null) {
      return calcuMediaAsk(ask, question).then((yes) =>
        yes ? send(command, Object.assign({}, args, { confirmed: true })) : { done: false, reason: "" });
    }
    return reply;
  });
}

function calcuMediaPlace(media, word) {
  return ((media && media.places) || []).find((p) => p.place === word) || null;
}

function calcuPickerLabel(entry) {
  if (entry.group === "empty") return "(empty)";
  const outdated = !entry.canChoose && entry.hint && entry.hint.indexOf("outdated") >= 0;
  return outdated ? entry.label + " (outdated)" : entry.label;
}

function CalcuMediaPicker({ place, send }) {
  if (!place) return null;
  const entries = place.picker || [];
  const held = place.holds ? place.holds.name : null;
  let current = 0;
  let listed = held === null;
  entries.forEach((e, i) => {
    if (!e.canChoose) return;
    if (held !== null ? e.group !== "empty" && e.label === held : e.group === "empty") {
      current = i;
      listed = true;
    }
  });
  // A medium the place holds that no entry lists (a tape started under a
  // name, its file written at the first motor stop): shown
  // as what the place holds.
  if (!listed) current = entries.length;
  return (
    <select className="calcu-media-picker" data-calcu={`media-picker-${place.place}`} value={String(current)}
      onChange={(ev) => {
        const e = entries[Number(ev.target.value)];
        if (e && e.canChoose && send) send("media.choose", { place: place.place, entry: e.id });
      }}>
      {entries.map((e, i) => (
        <option key={i} value={String(i)} disabled={!e.canChoose}
          title={(e.canChoose ? e.tooltip : e.hint) || undefined}>
          {calcuPickerLabel(e)}
        </option>
      ))}
      {listed ? null : <option key="held" value={String(entries.length)} disabled>{held}</option>}
    </select>
  );
}

