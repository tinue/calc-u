// The page's host services:
// what a Web frontend states to its session when it starts to listen, and
// the one answer to each ask. No folder picker, no haptics.
//
//   file-picker      {accept, multiple} → [{name, bytes}] (the glue writes
//                    them to /uploads and gives the session the paths);
//                    nothing picked: not given, "cancelled"
//   keep-file        {name, bytes, mediaType, kind?}: a download;
//                    kind "trace" for an ended trace
//   open-url         {url}: a new tab
//   clipboard-text   {text}: copied; without text: the clipboard's text, {text}
//   clipboard-image  {bytes}: PNG bytes copied;
//                    {bytes, text, name, dotsPerInch}: one
//                    clipboard entry with the picture and, with `text`, the
//                    text. When the page armed the clipboard in the user's
//                    gesture (armClipboard), the entry written then is
//                    completed; else it is written now. If the browser
//                    refuses the picture: with text, the text alone is
//                    copied (given, {textOnly: true}); without, the PNG is
//                    offered as a download named `name` (not given)
//   audio-sink       the sound messages (audio.js); never asked

const CALCU_WEB_SERVICES = ["audio-sink", "clipboard-text", "clipboard-image", "file-picker", "keep-file", "open-url"];

// <input type=file> once; resolves with the picked Files ([] if cancelled).
function calcuPickFiles(accept, multiple) {
  return new Promise((resolve) => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = accept || "";
    input.multiple = !!multiple;
    input.style.display = "none";
    let settled = false;
    const finish = (files) => {
      if (settled) return;
      settled = true;
      input.remove();
      resolve(files);
    };
    input.addEventListener("change", () => finish([...input.files]));
    input.addEventListener("cancel", () => finish([]));
    document.body.appendChild(input);
    input.click();
  });
}

// Files (picked or dropped) as the session takes them: [{name, bytes}].
function calcuNamedFiles(files) {
  return Promise.all(Array.from(files).map(async (f) => ({ name: f.name, bytes: new Uint8Array(await f.arrayBuffer()) })));
}

// Both products' downloadBytes.
function calcuDownload(name, bytes, type) {
  const url = URL.createObjectURL(new Blob([bytes], { type: type || "application/octet-stream" }));
  const a = document.createElement("a");
  a.href = url;
  a.download = name;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

// The clipboard written inside the user's gesture (a click on Copy, Cut
// or Copy Screen), with promises calcuCopyPicture resolves, because the
// picture comes from the worker after the gesture (WebKit allows a write
// only within one). {text}: whether a text entry comes too. A later arm,
// or calcuDisarmClipboard (the copy was refused), rejects the one before.
let calcuArmedClipboard = null;

function calcuDisarmClipboard() {
  const armed = calcuArmedClipboard;
  calcuArmedClipboard = null;
  if (armed) armed.reject(new Error("The copy was not made."));
}

// {view}: the window of the gesture (the paper's own window), whose
// clipboard takes the entry; the page's by default.
function armClipboard({ text = false, view = null } = {}) {
  calcuDisarmClipboard();
  const w = view || (typeof window !== "undefined" ? window : null);
  const nav = w ? w.navigator : (typeof navigator !== "undefined" ? navigator : null);
  const Item = (w && w.ClipboardItem) || (typeof ClipboardItem !== "undefined" ? ClipboardItem : undefined);
  const clip = nav ? nav.clipboard : null;
  if (!clip || !clip.write || typeof Item === "undefined") return null;
  const deferred = () => {
    const d = {};
    d.promise = new Promise((resolve, reject) => { d.resolve = resolve; d.reject = reject; });
    d.promise.catch(() => {});
    return d;
  };
  const png = deferred();
  const txt = text ? deferred() : null;
  const items = { "image/png": png.promise };
  if (txt) items["text/plain"] = txt.promise;
  let written;
  try {
    written = Promise.resolve(clip.write([new Item(items)]));
  } catch (e) {
    written = Promise.reject(e);
  }
  written.catch(() => {});
  const armed = {
    written,
    text: !!txt,
    resolve: (pngBlob, textBlob) => { png.resolve(pngBlob); if (txt) txt.resolve(textBlob); },
    reject: (e) => { png.reject(e); if (txt) txt.reject(e); },
  };
  calcuArmedClipboard = armed;
  return armed;
}

// clipboard-image with text, name, dotsPerInch; also the page's own copy
// of a picture (Copy Screen), with `whose` for the app log ("The
// screen's"); given(value) and refused(reason) make the outcome.
async function calcuCopyPicture(a, given, refused, whose = "The paper's") {
  const pngBlob = new Blob([a.bytes || new Uint8Array(0)], { type: "image/png" });
  const hasText = typeof a.text === "string";
  const armed = calcuArmedClipboard;
  calcuArmedClipboard = null;
  try {
    if (armed) {
      armed.resolve(pngBlob, new Blob([hasText ? a.text : ""], { type: "text/plain" }));
      await armed.written;
    } else {
      const items = { "image/png": pngBlob };
      if (hasText) items["text/plain"] = new Blob([a.text], { type: "text/plain" });
      await navigator.clipboard.write([new ClipboardItem(items)]);
    }
    return given();
  } catch (e) {
    // The browser's own error is told here only: the refusal's reason stays
    // short, for the user.
    const why = `${whose} picture was not copied: ${calcuErrorText(e)}`;
    let textError = null;
    if (hasText) {
      try {
        await navigator.clipboard.writeText(a.text);
        calcuAppLog("warn", `${why}; its text was copied alone.`);
        return given({ textOnly: true });
      } catch (t) {
        textError = t;
      }
    }
    calcuAppLog("warn", `${why}; ${textError ? `its text was not copied either: ${calcuErrorText(textError)}; ` : ""}` +
      "offered as a download.");
    calcuDownload(a.name || "paper.png", a.bytes || new Uint8Array(0), "image/png");
    return refused(`The browser did not take the picture (${(e && e.name) || "Error"}); it is offered as a download.`);
  }
}

function calcuWebHostServices() {
  const given = (value) => ({ done: true, reason: "", value: value === undefined ? null : value });
  const refused = (reason) => ({ done: false, reason, value: null });
  const answer = async (name, args) => {
    const a = args || {};
    switch (name) {
      case "file-picker": {
        const files = await calcuPickFiles(a.accept, a.multiple);
        if (!files.length) return refused("cancelled");
        return given(await calcuNamedFiles(files));
      }
      case "keep-file":
        calcuDownload(a.name || "download", a.bytes || new Uint8Array(0), a.mediaType);
        return given();
      case "open-url":
        window.open(a.url, "_blank", "noopener");
        return given();
      case "clipboard-text":
        if (typeof a.text === "string") {
          await navigator.clipboard.writeText(a.text);
          return given();
        }
        return given({ text: await navigator.clipboard.readText() });
      case "clipboard-image":
        if (typeof a.text === "string" || typeof a.name === "string") return calcuCopyPicture(a, given, refused);
        await navigator.clipboard.write([new ClipboardItem({ "image/png": new Blob([a.bytes], { type: "image/png" }) })]);
        return given();
      default:
        return refused(`This page does not offer ${name}.`);
    }
  };
  return { names: CALCU_WEB_SERVICES.slice(), answer };
}
