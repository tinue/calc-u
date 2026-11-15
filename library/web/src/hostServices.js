// The page's host services (LIB-15 design §3.1, §9 N4; spec AC-4, AC-9):
// what a Web frontend states to its session when it starts to listen, and
// the one answer to each ask. No folder picker, no haptics.
//
//   file-picker      {accept, multiple} → [{name, bytes}] (the glue writes
//                    them to /uploads and gives the session the paths);
//                    nothing picked: not given, "cancelled"
//   keep-file        {name, bytes, mediaType}: a download (AC-9)
//   open-url         {url}: a new tab
//   clipboard-text   {text}: copied; without text: the clipboard's text, {text}
//   clipboard-image  {bytes}: PNG bytes copied
//   audio-sink       the sound messages (audio.js); never asked

const CALCU_WEB_SERVICES = ["audio-sink", "clipboard-text", "clipboard-image", "file-picker", "keep-file", "open-url"];

// <input type=file> once; resolves with the picked Files ([] if cancelled).
// From Calc-U-1600 src/CalcU1600.jsx:19-38.
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

function calcuWebHostServices() {
  const given = (value) => ({ done: true, reason: "", value: value === undefined ? null : value });
  const refused = (reason) => ({ done: false, reason, value: null });
  const answer = async (name, args) => {
    const a = args || {};
    switch (name) {
      case "file-picker": {
        const files = await calcuPickFiles(a.accept, a.multiple);
        if (!files.length) return refused("cancelled");
        return given(await Promise.all(files.map(async (f) => ({ name: f.name, bytes: new Uint8Array(await f.arrayBuffer()) }))));
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
        await navigator.clipboard.write([new ClipboardItem({ "image/png": new Blob([a.bytes], { type: "image/png" }) })]);
        return given();
      default:
        return refused(`This page does not offer ${name}.`);
    }
  };
  return { names: CALCU_WEB_SERVICES.slice(), answer };
}
