/**
 * @module errors
 * Structured error taxonomy for git-mind (#207).
 *
 * Every user-facing error should be a GmindError with a code from the
 * catalog. The CLI boundary uses the code to select the right exit code
 * and, in --json mode, to emit a structured error envelope.
 */

// ── Exit codes ──────────────────────────────────────────────────

/** @enum {number} */
export const ExitCode = Object.freeze({
  SUCCESS:        0,
  GENERAL:        1,   // catch-all / internal
  USAGE:          2,   // bad flags, missing args, unknown command
  VALIDATION:     3,   // schema / data validation failure
  NOT_FOUND:      4,   // node, edge, epoch, observer, content
});

// ── Error codes ─────────────────────────────────────────────────

/**
 * Error catalog. Each entry maps a GMIND_E_* code to its default
 * exit code and a short human hint.
 *
 * @type {Record<string, { exit: number, hint: string }>}
 */
export const ERROR_CATALOG = Object.freeze({
  // Usage
  GMIND_E_USAGE:           { exit: ExitCode.USAGE,      hint: 'Check command usage with: git mind help' },
  GMIND_E_UNKNOWN_CMD:     { exit: ExitCode.USAGE,      hint: 'Run "git mind help" to see available commands' },

  // Lifecycle
  GMIND_E_NOT_INITIALIZED: { exit: ExitCode.GENERAL,    hint: 'Run "git mind init" first' },

  // Identity / validation
  GMIND_E_INVALID_ID:      { exit: ExitCode.VALIDATION,  hint: 'Node IDs must match prefix:identifier format' },
  GMIND_E_INVALID_EDGE:    { exit: ExitCode.VALIDATION,  hint: 'See GRAPH_SCHEMA.md for valid edge types' },
  GMIND_E_VALIDATION:      { exit: ExitCode.VALIDATION,  hint: 'Check input data against the expected schema' },

  // Not found
  GMIND_E_NODE_NOT_FOUND:  { exit: ExitCode.NOT_FOUND,   hint: 'Use "git mind nodes" to list existing nodes' },
  GMIND_E_NOT_FOUND:       { exit: ExitCode.NOT_FOUND,   hint: 'The requested resource does not exist' },

  // Domain
  GMIND_E_IMPORT:          { exit: ExitCode.GENERAL,     hint: 'Check file format and run with --dry-run first' },
  GMIND_E_EXPORT:          { exit: ExitCode.GENERAL,     hint: 'Check file permissions and disk space' },
  GMIND_E_EXTENSION:       { exit: ExitCode.GENERAL,     hint: 'Run "git mind extension validate <path>" to diagnose' },
  GMIND_E_CONTENT:         { exit: ExitCode.GENERAL,     hint: 'Check that the node exists and has content attached' },

  // Catch-all
  GMIND_E_INTERNAL:        { exit: ExitCode.GENERAL,     hint: 'This is a bug — please file an issue' },
});

// ── GmindError class ────────────────────────────────────────────

export class GmindError extends Error {
  /**
   * @param {string} code   - A GMIND_E_* constant from ERROR_CATALOG
   * @param {string} message - Human-readable error description
   * @param {object} [opts]
   * @param {string} [opts.hint]    - Override the catalog hint
   * @param {number} [opts.exitCode] - Override the catalog exit code
   * @param {Error}  [opts.cause]   - Root cause for chaining
   */
  constructor(code, message, opts = {}) {
    super(message, opts.cause ? { cause: opts.cause } : undefined);
    this.name = 'GmindError';

    const entry = ERROR_CATALOG[code];
    if (!entry) {
      // Unknown code — fall back to INTERNAL
      this.code = 'GMIND_E_INTERNAL';
      this.exitCode = ExitCode.GENERAL;
      this.hint = `Unknown error code: ${code}`;
    } else {
      this.code = code;
      this.exitCode = opts.exitCode ?? entry.exit;
      this.hint = opts.hint ?? entry.hint;
    }
  }

  /**
   * Structured envelope for --json error output.
   * @returns {{ error: string, errorCode: string, exitCode: number, hint: string }}
   */
  toJSON() {
    return {
      error: this.message,
      errorCode: this.code,
      exitCode: this.exitCode,
      hint: this.hint,
    };
  }
}
