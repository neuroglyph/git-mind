/**
 * @module context-envelope
 * Defines the ContextEnvelope — the explicit read context passed to all graph operations.
 *
 * A ContextEnvelope captures:
 * - `asOf`         — git ref or Lamport tick ceiling (time-travel / point-in-time reads)
 * - `observer`     — named observer for filtered/redacted views
 * - `trustPolicy`  — trust policy mode governing what facts are visible
 * - `extensionLock` — hash pinning the active extension set (reproducibility)
 */

/**
 * @typedef {object} ContextEnvelope
 * @property {string}      asOf          - Git ref or tick ceiling. Default: `'HEAD'`.
 * @property {string|null} observer      - Observer name for filtered views. Default: `null` (no filtering).
 * @property {string}      trustPolicy   - Trust policy mode. Default: `'open'`.
 * @property {string|null} extensionLock - SHA of pinned extension set. Default: `null`.
 */

/**
 * The default context — HEAD state, no observer, open trust, no extension lock.
 * @type {Readonly<ContextEnvelope>}
 */
export const DEFAULT_CONTEXT = Object.freeze({
  asOf: 'HEAD',
  observer: null,
  trustPolicy: 'open',
  extensionLock: null,
});

/**
 * Create a ContextEnvelope by merging overrides onto the defaults.
 * The returned object is frozen (immutable).
 *
 * @param {Partial<ContextEnvelope>} [overrides={}]
 * @returns {Readonly<ContextEnvelope>}
 *
 * @example
 * createContext()                        // → DEFAULT_CONTEXT
 * createContext({ asOf: 'main~5' })      // → { asOf: 'main~5', observer: null, ... }
 * createContext({ observer: 'alice' })   // → { asOf: 'HEAD', observer: 'alice', ... }
 */
export function createContext(overrides = {}) {
  return Object.freeze({ ...DEFAULT_CONTEXT, ...overrides });
}
