// ShellStorageBanner -- the storage sentence of the React set (LIB-14 design
// §10.3.5, D-39; LIB-15 AC-8, EC-2, EC-3; ledger 1600-48, 59-55): LIB-15's
// state topic `web.storage` ({mode, sentence}, posted by the library's
// worker) shown as a status line while what is saved is not kept (`mode`
// other than "kept"): another tab, unreadable or unavailable storage. The
// sentence is the library's (src/sentences.js). Name: storage-banner.
//
//   <ShellStorageBanner shell={shell} />   (shell: LIB-15's createShell)

function ShellStorageBanner({ shell }) {
  const storage = useTopic(shell, "web.storage");
  if (!storage || !storage.mode || storage.mode === "kept") return null;
  return (
    <div className="calcu-shell-storage-banner" data-calcu="storage-banner" role="status">
      {storage.sentence || ""}
    </div>
  );
}
