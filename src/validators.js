/**
 * @module validators
 * Runtime schema validators for git-mind's knowledge graph.
 * Implements constraints from GRAPH_SCHEMA.md (BDK-001).
 */

// ── Constants ────────────────────────────────────────────────────────

/** @type {RegExp} Canonical regex for node IDs (prefix:identifier) */
export const NODE_ID_REGEX = /^[a-z][a-z0-9-]*:[A-Za-z0-9._\/@-]+$/;

/** @type {number} Maximum total length of a node ID */
export const NODE_ID_MAX_LENGTH = 256;

/** @type {string[]} All canonical prefixes including system */
export const CANONICAL_PREFIXES = [
  'milestone', 'feature', 'task', 'issue', 'phase',
  'spec', 'adr', 'doc', 'concept', 'decision',
  'crate', 'module', 'pkg', 'file',
  'person', 'tool',
  'event', 'metric',
  'commit',
];

/** @type {string[]} System-generated prefixes (reserved, not user-writable) */
export const SYSTEM_PREFIXES = ['commit'];

/** @type {string[]} Valid edge types */
export const EDGE_TYPES = [
  'implements',
  'augments',
  'relates-to',
  'blocks',
  'belongs-to',
  'consumed-by',
  'depends-on',
  'documents',
];

/** @type {string[]} Edge types that forbid self-edges */
const SELF_EDGE_FORBIDDEN = ['blocks', 'depends-on'];

// ── Functions ────────────────────────────────────────────────────────

/**
 * Extract the prefix portion of a node ID (before the first colon).
 *
 * @param {string} nodeId
 * @returns {string|null} Prefix string, or null if no colon present
 */
export function extractPrefix(nodeId) {
  const idx = nodeId.indexOf(':');
  if (idx === -1) return null;
  return nodeId.slice(0, idx);
}

/**
 * Validate a node ID against the schema grammar.
 *
 * @param {string} nodeId
 * @returns {{ valid: boolean, error?: string }}
 */
export function validateNodeId(nodeId) {
  if (!nodeId || typeof nodeId !== 'string') {
    return { valid: false, error: 'Node ID must be a non-empty string' };
  }
  if (nodeId.length > NODE_ID_MAX_LENGTH) {
    return { valid: false, error: `Node ID exceeds max length of ${NODE_ID_MAX_LENGTH} characters (got ${nodeId.length})` };
  }
  if (!NODE_ID_REGEX.test(nodeId)) {
    return { valid: false, error: `Invalid node ID: "${nodeId}". Must match prefix:identifier (lowercase prefix, valid identifier chars)` };
  }
  return { valid: true };
}

/**
 * Classify a prefix string.
 *
 * @param {string} prefix
 * @returns {'canonical'|'system'|'unknown'}
 */
export function classifyPrefix(prefix) {
  if (SYSTEM_PREFIXES.includes(prefix)) return 'system';
  if (CANONICAL_PREFIXES.includes(prefix)) return 'canonical';
  return 'unknown';
}

/**
 * Validate an edge type against the known set.
 *
 * @param {string} type
 * @returns {{ valid: boolean, error?: string }}
 */
export function validateEdgeType(type) {
  if (!EDGE_TYPES.includes(type)) {
    return { valid: false, error: `Unknown edge type: "${type}". Valid types: ${EDGE_TYPES.join(', ')}` };
  }
  return { valid: true };
}

/**
 * Validate a confidence value.
 *
 * @param {*} value
 * @returns {{ valid: boolean, error?: string }}
 */
export function validateConfidence(value) {
  if (typeof value !== 'number') {
    return { valid: false, error: `Confidence must be a number, got ${typeof value}` };
  }
  if (!Number.isFinite(value)) {
    return { valid: false, error: `Confidence must be a finite number, got ${value}` };
  }
  if (value < 0 || value > 1) {
    return { valid: false, error: `Confidence must be between 0.0 and 1.0, got ${value}` };
  }
  return { valid: true };
}

/**
 * Composite validator for a full edge.
 * Validates source/target IDs, edge type, confidence, and self-edge constraint.
 *
 * @param {string} source - Source node ID
 * @param {string} target - Target node ID
 * @param {string} type - Edge type
 * @param {number} [confidence] - Optional confidence value
 * @returns {{ valid: boolean, errors: string[], warnings: string[] }}
 */
export function validateEdge(source, target, type, confidence) {
  const errors = [];
  const warnings = [];

  // Validate source node ID
  const srcResult = validateNodeId(source);
  if (!srcResult.valid) errors.push(`Source: ${srcResult.error}`);

  // Validate target node ID
  const tgtResult = validateNodeId(target);
  if (!tgtResult.valid) errors.push(`Target: ${tgtResult.error}`);

  // Validate edge type
  const typeResult = validateEdgeType(type);
  if (!typeResult.valid) errors.push(typeResult.error);

  // Validate confidence if provided
  if (confidence !== undefined) {
    const confResult = validateConfidence(confidence);
    if (!confResult.valid) errors.push(confResult.error);
  }

  // Self-edge check
  if (source === target && SELF_EDGE_FORBIDDEN.includes(type)) {
    errors.push(`Self-edge forbidden for "${type}": source and target are both "${source}"`);
  }

  // Unknown prefix warnings (only if IDs are structurally valid)
  if (srcResult.valid) {
    const srcPrefix = extractPrefix(source);
    if (srcPrefix && classifyPrefix(srcPrefix) === 'unknown') {
      warnings.push(`Source prefix "${srcPrefix}" is not a canonical prefix`);
    }
  }
  if (tgtResult.valid) {
    const tgtPrefix = extractPrefix(target);
    if (tgtPrefix && classifyPrefix(tgtPrefix) === 'unknown') {
      warnings.push(`Target prefix "${tgtPrefix}" is not a canonical prefix`);
    }
  }

  return { valid: errors.length === 0, errors, warnings };
}
