// The sentences a Web frontend shows about its start and its storage: the same for every
// product, with the product's name. Concatenated into a product's calc.js
// (the page) and into dist/library-worker.js (the worker).

const CALCU_SENTENCES = {
  // The page shows it instead of going on with "starting".
  startFailed: (product, why) => `${product} could not start${why ? `: ${why}` : "."}`,
  starting: (product) => `Starting ${product}…`,
  // Another copy of the product holds the store.
  secondCopy: (product) => `${product} is open in another tab or window: what you save here is not kept.`,
  // What could not be read is left as it is.
  unreadable: (product) =>
    `${product} could not read its saved files: they are left as they are, and what you save here is not kept.`,
  // Private mode or a full store.
  unavailable: (product) => `This browser keeps no files for ${product} here: what you save is not kept.`,
};
