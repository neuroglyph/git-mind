/**
 * @module lens
 * Composable lens engine for git-mind.
 * Lenses post-process ViewResults: ViewResult → ViewResult.
 */

/**
 * @typedef {object} ViewResult
 * @property {string[]} nodes
 * @property {Array<{from: string, to: string, label: string, props?: Record<string, unknown>}>} edges
 * @property {Record<string, unknown>} [meta]
 */

/**
 * @typedef {object} LensDefinition
 * @property {string} name
 * @property {(viewResult: ViewResult, nodeProps: Map<string, Record<string, unknown>>, options?: Record<string, unknown>) => ViewResult} filterFn
 * @property {boolean} needsProperties
 */

/** @type {Map<string, LensDefinition>} */
const registry = new Map();

/**
 * Register a named lens.
 *
 * @param {string} name - Lens name (e.g. 'incomplete', 'frontier')
 * @param {LensDefinition['filterFn']} filterFn - Transform function
 * @param {{ needsProperties?: boolean }} [opts]
 */
export function defineLens(name, filterFn, opts = {}) {
  if (typeof name !== 'string' || name.length === 0) {
    throw new Error('defineLens: name must be a non-empty string');
  }
  if (typeof filterFn !== 'function') {
    throw new Error(`defineLens("${name}"): filterFn must be a function`);
  }
  registry.set(name, {
    name,
    filterFn,
    needsProperties: opts.needsProperties ?? false,
  });
}

/**
 * Get a lens by name.
 *
 * @param {string} name
 * @returns {LensDefinition | undefined}
 */
export function getLens(name) {
  return registry.get(name);
}

/**
 * List all registered lens names.
 * @returns {string[]}
 */
export function listLenses() {
  return [...registry.keys()];
}

/**
 * Compose an array of lens names into a single transform function.
 * Left-to-right application: first lens in array runs first.
 *
 * @param {string[]} lensNames
 * @returns {{ composedFn: LensDefinition['filterFn'], needsProperties: boolean }}
 */
export function composeLenses(lensNames) {
  if (!lensNames || lensNames.length === 0) {
    return {
      composedFn: (viewResult) => viewResult,
      needsProperties: false,
    };
  }

  const lenses = [];
  for (const name of lensNames) {
    const lens = registry.get(name);
    if (!lens) {
      const available = [...registry.keys()].join(', ');
      throw new Error(`Unknown lens: "${name}". Available lenses: ${available}`);
    }
    lenses.push(lens);
  }

  const needsProperties = lenses.some(l => l.needsProperties);

  const composedFn = (viewResult, nodeProps, options) => {
    let result = viewResult;
    for (const lens of lenses) {
      result = lens.filterFn(result, nodeProps, options);
    }
    return result;
  };

  return { composedFn, needsProperties };
}

/** @type {Set<string>} Built-in lens names, captured after registration */
let builtInNames = new Set();

/**
 * Remove all lenses that were not registered at module load time.
 * Intended for test cleanup.
 */
export function resetLenses() {
  for (const name of registry.keys()) {
    if (!builtInNames.has(name)) registry.delete(name);
  }
}

/**
 * Capture current registry as built-in set.
 * Called after core lenses are registered.
 */
export function captureBuiltIns() {
  builtInNames = new Set(registry.keys());
}
