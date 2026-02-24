/**
 * @module nodes
 * Node query and inspection for git-mind.
 */

import { extractPrefix, classifyPrefix } from './validators.js';

/**
 * @typedef {object} NodeInfo
 * @property {string} id - Full node ID (prefix:identifier)
 * @property {string} prefix - Extracted prefix
 * @property {string} prefixClass - 'canonical' | 'system' | 'unknown'
 * @property {Record<string, unknown>} properties - Node properties
 */

/**
 * Get all node IDs from the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<string[]>}
 */
export async function getNodes(graph) {
  return graph.getNodes();
}

/**
 * Check if a node exists in the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} id - Node ID to check
 * @returns {Promise<boolean>}
 */
export async function hasNode(graph, id) {
  return graph.hasNode(id);
}

/**
 * Get a node by ID with prefix classification and properties.
 * Returns null if the node doesn't exist.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} id - Node ID
 * @returns {Promise<NodeInfo|null>}
 */
export async function getNode(graph, id) {
  const exists = await graph.hasNode(id);
  if (!exists) return null;

  const prefix = extractPrefix(id);
  const propsMap = await graph.getNodeProps(id);

  // Convert Map to plain object
  const properties = {};
  if (propsMap) {
    for (const [key, value] of propsMap) {
      properties[key] = value;
    }
  }

  return {
    id,
    prefix,
    prefixClass: prefix ? classifyPrefix(prefix) : 'unknown',
    properties,
  };
}

/**
 * Get all nodes matching a given prefix.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} prefix - Prefix to filter by (without colon)
 * @returns {Promise<string[]>}
 */
export async function getNodesByPrefix(graph, prefix) {
  const nodes = await graph.getNodes();
  const needle = prefix + ':';
  return nodes.filter(n => n.startsWith(needle));
}

/**
 * @typedef {object} SetPropertyResult
 * @property {string} id - Node ID
 * @property {string} key - Property key
 * @property {unknown} value - New property value
 * @property {unknown} previous - Previous value (null if unset)
 * @property {boolean} changed - Whether the value actually changed
 */

/**
 * Set a property on an existing node.
 * Idempotent — setting the same value returns { changed: false }.
 *
 * Note: idempotency uses strict equality (===). The CLI always passes string
 * values; callers using structured values (objects/arrays) will always see
 * changed: true since === compares by reference.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} id - Node ID
 * @param {string} key - Property key (non-empty)
 * @param {unknown} value - Property value (string in CLI usage)
 * @returns {Promise<SetPropertyResult>}
 */
export async function setNodeProperty(graph, id, key, value) {
  if (!key || typeof key !== 'string') {
    throw new Error('Property key must be a non-empty string');
  }

  const exists = await graph.hasNode(id);
  if (!exists) {
    throw new Error(`Node not found: ${id}`);
  }

  // Read current value
  const propsMap = await graph.getNodeProps(id);
  const previous = propsMap?.get(key) ?? null;
  const changed = previous !== value;

  if (changed) {
    const patch = await graph.createPatch();
    patch.setProperty(id, key, value);
    await patch.commit();
  }

  return { id, key, value, previous, changed };
}

/**
 * @typedef {object} UnsetPropertyResult
 * @property {string} id - Node ID
 * @property {string} key - Property key
 * @property {unknown} previous - Previous value (null if unset)
 * @property {boolean} removed - Whether a value was actually removed
 */

/**
 * Remove a property from an existing node.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} id - Node ID
 * @param {string} key - Property key
 * @returns {Promise<UnsetPropertyResult>}
 */
export async function unsetNodeProperty(graph, id, key) {
  if (!key || typeof key !== 'string') {
    throw new Error('Property key must be a non-empty string');
  }

  const exists = await graph.hasNode(id);
  if (!exists) {
    throw new Error(`Node not found: ${id}`);
  }

  const propsMap = await graph.getNodeProps(id);
  const previous = propsMap?.get(key) ?? null;
  const removed = previous != null;

  if (removed) {
    const patch = await graph.createPatch();
    patch.setProperty(id, key, null);
    await patch.commit();
  }

  return { id, key, previous, removed };
}
