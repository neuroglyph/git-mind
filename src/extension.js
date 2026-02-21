/**
 * @module extension
 * Extension runtime for git-mind.
 * Loads, validates, and registers extension manifests.
 * Mirrors the registry pattern of src/lens.js.
 */

import { readFileSync } from 'node:fs';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import Ajv from 'ajv/dist/2020.js';
import { declareView } from './views.js';
import { getLens } from './lens.js';

// ── Schema setup ─────────────────────────────────────────────────

const SCHEMA_PATH = new URL('../docs/contracts/extension-manifest.schema.json', import.meta.url);
const schema = JSON.parse(readFileSync(fileURLToPath(SCHEMA_PATH), 'utf-8'));

const ajv = new Ajv({ strict: false, allErrors: true });
const validate = ajv.compile(schema);

// ── Built-in manifest paths ───────────────────────────────────────

const BUILTIN_MANIFESTS = [
  new URL('../extensions/roadmap/extension.yaml', import.meta.url),
  new URL('../extensions/architecture/extension.yaml', import.meta.url),
];

// ── Registry ─────────────────────────────────────────────────────

/**
 * @typedef {object} ExtensionRecord
 * @property {string} name
 * @property {string} version
 * @property {string} [description]
 * @property {string} manifestPath
 * @property {{ prefixes: string[], edgeTypes?: string[], statusValues?: string[] }} domain
 * @property {Array<{name: string, prefixes: string[], edgeTypes?: string[], requireBothEndpoints?: boolean, description?: string}>} views
 * @property {string[]} lenses
 * @property {Array<{name: string, severity: string, description?: string}>} rules
 * @property {Array<{name: string, description?: string}>} syncAdapters
 * @property {Array<{name: string, formats: string[], description?: string}>} materializers
 * @property {boolean} builtin
 */

/** @type {Map<string, ExtensionRecord>} */
const registry = new Map();

/** @type {Map<string, ExtensionRecord>} Snapshot of built-in extensions */
let builtInDefs = new Map();

/** Whether registerBuiltinExtensions() has already been called. */
let builtInsLoaded = false;

// ── Core API ─────────────────────────────────────────────────────

/**
 * Load and validate an extension manifest file (YAML or JSON).
 * Does NOT register the extension — use registerExtension() for that.
 *
 * @param {string} manifestPath - Absolute or relative path to the manifest file
 * @returns {Promise<ExtensionRecord>}
 * @throws {Error} If file not found, parse error, or schema violation
 */
export async function loadExtension(manifestPath) {
  let raw;
  try {
    raw = await readFile(manifestPath, 'utf-8');
  } catch (err) {
    throw new Error(`Extension manifest not found: ${manifestPath} (${err.message})`);
  }

  let parsed;
  try {
    if (manifestPath.endsWith('.json')) {
      parsed = JSON.parse(raw);
    } else {
      parsed = yaml.load(raw);
    }
  } catch (err) {
    throw new Error(`Failed to parse extension manifest at ${manifestPath}: ${err.message}`);
  }

  const valid = validate(parsed);
  if (!valid) {
    const messages = validate.errors.map(e => `${e.instancePath || '(root)'} ${e.message}`).join('; ');
    throw new Error(`Extension manifest schema violation at ${manifestPath}: ${messages}`);
  }

  return _normalize(parsed, manifestPath, false);
}

/**
 * Register a loaded ExtensionRecord.
 * Calls declareView() for each view and verifies lenses exist.
 *
 * @param {ExtensionRecord} record
 * @param {{ skipViews?: boolean }} [opts]
 * @throws {Error} If a referenced lens is not registered
 */
export function registerExtension(record, opts = {}) {
  // Check for prefix collisions with other registered extensions
  const incoming = record.domain?.prefixes ?? [];
  if (incoming.length > 0) {
    for (const [existingName, existing] of registry) {
      if (existingName === record.name) continue; // allow idempotent re-register
      const overlap = incoming.filter(p => existing.domain.prefixes.includes(p));
      if (overlap.length > 0) {
        throw new Error(
          `Extension "${record.name}" declares prefix(es) [${overlap.join(', ')}] already owned by "${existingName}"`
        );
      }
    }
  }

  // Verify all referenced lenses exist
  for (const lensName of record.lenses) {
    if (!getLens(lensName)) {
      throw new Error(`Extension "${record.name}" references unknown lens: "${lensName}"`);
    }
  }

  // Register declarative views (skip if builtin=true since views.js already registered them)
  if (!record.builtin && !opts.skipViews) {
    for (const viewConfig of record.views) {
      declareView(viewConfig.name, viewConfig);
    }
  }

  registry.set(record.name, record);
}

/**
 * List all registered extensions.
 * @returns {ExtensionRecord[]}
 */
export function listExtensions() {
  return [...registry.values()];
}

/**
 * Get a registered extension by name.
 * @param {string} name
 * @returns {ExtensionRecord | undefined}
 */
export function getExtension(name) {
  return registry.get(name);
}

/**
 * Validate a manifest file without registering it.
 * Returns validation result + parsed record (or null on failure).
 *
 * @param {string} manifestPath
 * @returns {Promise<{ valid: boolean, errors: string[], record: ExtensionRecord | null }>}
 */
export async function validateExtension(manifestPath) {
  try {
    const record = await loadExtension(manifestPath);
    return { valid: true, errors: [], record };
  } catch (err) {
    return { valid: false, errors: [err.message], record: null };
  }
}

/**
 * Remove all non-builtin extensions from the registry.
 * Restores to built-in baseline. Intended for test cleanup.
 */
export function resetExtensions() {
  registry.clear();
  for (const [name, rec] of builtInDefs) {
    registry.set(name, rec);
  }
}

/**
 * Capture the current registry as the built-in baseline.
 * Called after built-in extensions are registered.
 * @internal
 */
export function captureBuiltIns() {
  builtInDefs = new Map(registry);
}

/**
 * Clear the built-in snapshot. Intended for test cleanup only.
 * @internal
 */
export function _resetBuiltInsForTest() {
  builtInDefs.clear();
  builtInsLoaded = false;
}

/**
 * Load and register all built-in extension manifests.
 * Built-in extensions are registered with builtin=true — views are skipped
 * because src/views.js already declares them.
 *
 * @returns {Promise<void>}
 */
export async function registerBuiltinExtensions() {
  if (builtInsLoaded) return;
  for (const url of BUILTIN_MANIFESTS) {
    const manifestPath = fileURLToPath(url);
    const record = await loadExtension(manifestPath);
    registerExtension({ ...record, builtin: true });
  }
  captureBuiltIns();
  builtInsLoaded = true;
}

// ── Internal helpers ─────────────────────────────────────────────

/**
 * Normalize a parsed manifest object into an ExtensionRecord.
 * Fills in optional arrays as empty arrays.
 *
 * @param {object} parsed
 * @param {string} manifestPath
 * @param {boolean} builtin
 * @returns {ExtensionRecord}
 */
function _normalize(parsed, manifestPath, builtin) {
  return {
    name: parsed.name,
    version: parsed.version,
    description: parsed.description,
    manifestPath,
    domain: {
      prefixes: parsed.domain?.prefixes ?? [],
      edgeTypes: parsed.domain?.edgeTypes ?? [],
      statusValues: parsed.domain?.statusValues ?? [],
    },
    views: parsed.views ?? [],
    lenses: parsed.lenses ?? [],
    rules: parsed.rules ?? [],
    syncAdapters: parsed.syncAdapters ?? [],
    materializers: parsed.materializers ?? [],
    builtin,
  };
}
