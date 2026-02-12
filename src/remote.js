/**
 * @module remote
 * Cross-repo edge protocol — ID parsing and qualification.
 * Cross-repo IDs follow the format: repo:owner/name:prefix:identifier
 */

/** @type {RegExp} Regex for cross-repo node IDs. */
export const CROSS_REPO_ID_REGEX = /^repo:([A-Za-z0-9._-]+\/[A-Za-z0-9._-]+):([a-z][a-z0-9-]*):([A-Za-z0-9._\/@-]+)$/;

/**
 * @typedef {object} CrossRepoId
 * @property {string} repo - Repository identifier (owner/name)
 * @property {string} prefix - Inner prefix (e.g., "crate", "task")
 * @property {string} identifier - Node identifier within the prefix
 * @property {string} local - Local node ID (prefix:identifier)
 */

/**
 * Parse a cross-repo node ID.
 *
 * @param {string} nodeId
 * @returns {CrossRepoId|null} Parsed components, or null if not a cross-repo ID
 */
export function parseCrossRepoId(nodeId) {
  if (typeof nodeId !== 'string') return null;
  const match = CROSS_REPO_ID_REGEX.exec(nodeId);
  if (!match) return null;
  return {
    repo: match[1],
    prefix: match[2],
    identifier: match[3],
    local: `${match[2]}:${match[3]}`,
  };
}

/**
 * Build a cross-repo node ID from components.
 *
 * @param {string} repo - Repository identifier (owner/name)
 * @param {string} localId - Local node ID (prefix:identifier)
 * @returns {string} Cross-repo ID (repo:owner/name:prefix:identifier)
 */
export function buildCrossRepoId(repo, localId) {
  return `repo:${repo}:${localId}`;
}

/**
 * Check whether a node ID is a cross-repo ID.
 *
 * @param {string} nodeId
 * @returns {boolean}
 */
export function isCrossRepoId(nodeId) {
  return typeof nodeId === 'string' && CROSS_REPO_ID_REGEX.test(nodeId);
}

/**
 * Extract the repository identifier from a cross-repo ID.
 *
 * @param {string} nodeId
 * @returns {string|null} Repository identifier (owner/name), or null if not cross-repo
 */
export function extractRepo(nodeId) {
  const parsed = parseCrossRepoId(nodeId);
  return parsed ? parsed.repo : null;
}

/**
 * Qualify a local node ID with a repository identifier.
 * If the ID is already cross-repo, returns it unchanged.
 *
 * @param {string} nodeId - Local or cross-repo node ID
 * @param {string} repo - Repository identifier (owner/name)
 * @returns {string} Qualified cross-repo ID
 */
export function qualifyNodeId(nodeId, repo) {
  if (isCrossRepoId(nodeId)) return nodeId;
  return buildCrossRepoId(repo, nodeId);
}
