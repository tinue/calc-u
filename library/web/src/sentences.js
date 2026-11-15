// The sentences a Web frontend shows about its start and its storage (LIB-15
// spec AC-3, AC-8, EC-2, EC-3, Technical Requirements): the same for every
// product, with the product's name. Concatenated into a product's calc.js
// (the page) and into dist/library-worker.js (the worker).

const CALCU_SENTENCES = {
  // AC-3: the page shows it instead of going on with "starting".
  startFailed: (product, why) => `${product} could not start${why ? `: ${why}` : "."}`,
  starting: (product) => `Starting ${product}…`,
  // AC-8 (Calc-U-1600 today: "Calc-U-1600 is open in another tab: cards,
  // disks and tapes saved here are not kept.").
  secondCopy: (product) => `${product} is open in another tab or window: what you save here is not kept.`,
  // EC-2: what could not be read is left as it is.
  unreadable: (product) =>
    `${product} could not read its saved files: they are left as they are, and what you save here is not kept.`,
  // EC-3: private mode or a full store.
  unavailable: (product) => `This browser keeps no files for ${product} here: what you save is not kept.`,
};
