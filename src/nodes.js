/**
 * @module nodes
 * Node query and inspection for git-mind.
 */

import { validateNodeId, extractPrefix, classifyPrefix } from './validators.js';

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
