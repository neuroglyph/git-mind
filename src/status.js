/**
 * @module status
 * Graph status computation for git-mind.
 */

import { extractPrefix, isLowConfidence } from './validators.js';

/**
 * @typedef {object} GraphStatus
 * @property {{ total: number, byPrefix: Record<string, number> }} nodes
 * @property {{ total: number, byType: Record<string, number> }} edges
 * @property {{ blockedItems: number, lowConfidence: number, orphanNodes: number }} health
 */

/**
 * Compute a summary of the graph's current state.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<GraphStatus>}
 */
export async function computeStatus(graph) {
  const nodes = await graph.getNodes();
  const edges = await graph.getEdges();

  // Nodes grouped by prefix
  const byPrefix = {};
  for (const n of nodes) {
    const prefix = extractPrefix(n) ?? '(none)';
    byPrefix[prefix] = (byPrefix[prefix] ?? 0) + 1;
  }

  // Edges grouped by type
  const byType = {};
  for (const e of edges) {
    byType[e.label] = (byType[e.label] ?? 0) + 1;
  }

  // Health: blocked items (distinct targets of 'blocks' edges)
  const blockedTargets = new Set(
    edges.filter(e => e.label === 'blocks').map(e => e.to)
  );
  const blockedItems = blockedTargets.size;

  // Health: low-confidence edges
  const lowConfidence = edges.filter(isLowConfidence).length;

  // Health: orphan nodes (not referenced by any edge)
  const connected = new Set();
  for (const e of edges) {
    connected.add(e.from);
    connected.add(e.to);
  }
  const orphanNodes = nodes.filter(n => !connected.has(n)).length;

  return {
    nodes: { total: nodes.length, byPrefix },
    edges: { total: edges.length, byType },
    health: { blockedItems, lowConfidence, orphanNodes },
  };
}
