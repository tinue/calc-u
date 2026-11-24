// The persistent file system, worker side. One IDBFS mount per product (its
// database is named after the mount point, so two products on one origin
// never share one), written by one copy only (Web Locks), never written over
// when it could not be read.
//
//   kept         the lock is held, or there is no lock API, and the
//                read succeeded: every write, rename and delete is kept
//   second-copy  another copy holds the lock: read once, kept in memory
//   unreadable   the read failed: in memory, nothing goes to the store
//   unavailable  IndexedDB cannot be opened, or a write was refused (a
//                full store): in memory
//
// The automatic persistence is IDBFS's autoPersist, with the library's own
// queue in place of IDBFS.queuePersist: it is shut until the read succeeded
// in mode `kept`, and it reports a refused write.

// Whether IndexedDB opens at all (private mode as such cannot be told).
function calcuCanOpenIndexedDb(name) {
  return new Promise((resolve) => {
    try {
      if (typeof indexedDB === "undefined" || !indexedDB) { resolve(false); return; }
      const request = indexedDB.open(name);
      request.onsuccess = () => { request.result.close(); resolve(true); };
      request.onerror = () => {
        calcuAppLog("warn", `storage: IndexedDB ${name} cannot be opened: ${calcuErrorText(request.error)}`);
        resolve(false);
      };
      request.onblocked = () => resolve(true);
    } catch (e) {
      calcuAppLog("warn", `storage: IndexedDB ${name} cannot be opened: ${calcuErrorText(e)}`);
      resolve(false);
    }
  });
}

// Whether this copy may write: it holds the lock for the rest of its life,
// or there is no lock API (plain HTTP).
function calcuAcquireStorageLock(name) {
  if (!(self.navigator && navigator.locks && navigator.locks.request)) return Promise.resolve(true);
  return new Promise((resolve) => {
    navigator.locks.request(name, { ifAvailable: true }, (lock) => {
      resolve(!!lock);
      return lock ? new Promise(() => {}) : undefined;  // held while the worker lives
    }).catch((e) => {
      calcuAppLog("warn", `storage: the lock ${name} could not be asked for: ${calcuErrorText(e)}; this copy writes.`);
      resolve(true);
    });
  });
}

// Mounts the product's store at `config.mount`; resolves with
// {mode, sentence}. `onRefused(state)` is told when a write is refused later.
async function calcuOpenStorage(Module, config, onRefused) {
  const FS = Module.FS;
  const IDBFS = Module.IDBFS;
  const product = config.name;
  const memory = (mode, sentence) => ({ mode, sentence: sentence(product) });
  FS.mkdir(config.mount);

  if (!(await calcuCanOpenIndexedDb(config.mount))) return memory("unavailable", CALCU_SENTENCES.unavailable);
  const writer = await calcuAcquireStorageLock(config.lock);

  let open = false;
  const persist = (mount) => {
    const done = (err) => {
      if (err && open) {
        open = false;
        calcuAppLog("warn", `storage: writing ${config.mount} failed: ${calcuErrorText(err)}; nothing more is kept.`);
        if (onRefused) onRefused(memory("unavailable", CALCU_SENTENCES.unavailable));
      }
      if (mount.calcuPersist === "again") start();
      else mount.calcuPersist = 0;
    };
    const start = () => {
      mount.calcuPersist = "running";
      try {
        IDBFS.syncfs(mount, false, done);
      } catch (e) {
        done(e);
      }
    };
    if (!mount.calcuPersist) mount.calcuPersist = setTimeout(start, 0);
    else if (mount.calcuPersist === "running") mount.calcuPersist = "again";
  };
  IDBFS.queuePersist = (mount) => { if (open) persist(mount); };

  FS.mount(IDBFS, { autoPersist: true }, config.mount);
  const err = await new Promise((resolve) => {
    try {
      FS.syncfs(true, (e) => resolve(e || null));
    } catch (e) {
      resolve(e);
    }
  });
  if (err) {
    calcuAppLog("warn", `storage: ${config.mount} could not be read: ${calcuErrorText(err)}; nothing is kept.`);
    return memory("unreadable", CALCU_SENTENCES.unreadable);
  }
  if (!writer) return memory("second-copy", CALCU_SENTENCES.secondCopy);
  open = true;
  return { mode: "kept", sentence: "" };
}
