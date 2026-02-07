/**
 * @module edges
 * Edge creation, querying, and removal for git-mind.
 */

/** @type {string[]} */
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
  if (!EDGE_TYPES.includes(type)) {
    throw new Error(`Unknown edge type: "${type}". Valid types: ${EDGE_TYPES.join(', ')}`);
  }
  if (confidence < 0 || confidence > 1) {
    throw new Error(`Confidence must be between 0.0 and 1.0, got ${confidence}`);
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
 * @typedef {object} EdgeQuery
 * @property {string} [source] - Filter by source node
 * @property {string} [target] - Filter by target node
 * @property {string} [type] - Filter by edge type
 */

/**
 * Query edges from the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {EdgeQuery} [filter={}]
 * @returns {Promise<Array<{from: string, to: string, label: string, props: object}>>}
 */
export async function queryEdges(graph, filter = {}) {
  const allEdges = await graph.getEdges();

  return allEdges.filter(edge => {
    if (filter.source && edge.from !== filter.source) return false;
    if (filter.target && edge.to !== filter.target) return false;
    if (filter.type && edge.label !== filter.type) return false;
    return true;
  });
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
