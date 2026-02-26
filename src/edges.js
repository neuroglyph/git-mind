/**
 * @module edges
 * Edge creation, querying, and removal for git-mind.
 */

import { validateEdge } from './validators.js';

export { EDGE_TYPES } from './validators.js';

/**
 * @typedef {object} EdgeInput
 * @property {string} source - Source node ID
 * @property {string} target - Target node ID
 * @property {string} type - Edge type (one of EDGE_TYPES)
 * @property {number} [confidence=1.0] - Confidence 0.0–1.0
 * @property {string} [rationale] - Why this edge exists
 */

/**
 * Create an edge between two nodes in the graph.
 * Creates source/target nodes if they don't already exist.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {EdgeInput} input
 * @returns {Promise<void>}
 */
export async function createEdge(graph, { source, target, type, confidence = 1.0, rationale }) {
  const result = validateEdge(source, target, type, confidence);
  if (!result.valid) {
    throw new Error(result.errors.join('; '));
  }
  for (const warning of result.warnings) {
    console.warn(`[git-mind] ${warning}`);
  }

  const patch = await graph.createPatch();
  // Ensure both nodes exist
  if (!(await graph.hasNode(source))) {
    patch.addNode(source);
  }
  if (!(await graph.hasNode(target))) {
    patch.addNode(target);
  }

  patch.addEdge(source, target, type);
  patch.setEdgeProperty(source, target, type, 'confidence', confidence);
  patch.setEdgeProperty(source, target, type, 'createdAt', new Date().toISOString());

  if (rationale) {
    patch.setEdgeProperty(source, target, type, 'rationale', rationale);
  }

  await patch.commit();
}

/**
 * Remove an edge from the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} source
 * @param {string} target
 * @param {string} type
 * @returns {Promise<void>}
 */
export async function removeEdge(graph, source, target, type) {
  const patch = await graph.createPatch();
  patch.removeEdge(source, target, type);
  await patch.commit();
}
