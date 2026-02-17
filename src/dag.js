/**
 * @module dag
 * Pure DAG utilities for git-mind.
 * Extracted from views.js — composable, testable, no graph dependencies.
 */

/**
 * @typedef {object} Edge
 * @property {string} from
 * @property {string} to
 * @property {string} label
 * @property {Record<string, unknown>} [props]
 */

/**
 * Build forward and reverse adjacency maps from an edge list.
 *
 * @param {Edge[]} edges
 * @param {string} [edgeType] - If provided, only include edges of this type
 * @returns {{ forward: Map<string, string[]>, reverse: Map<string, string[]> }}
 */
export function buildAdjacency(edges, edgeType) {
  const forward = new Map();
  const reverse = new Map();

  for (const e of edges) {
    if (edgeType && e.label !== edgeType) continue;
    if (!forward.has(e.from)) forward.set(e.from, []);
    forward.get(e.from).push(e.to);
    if (!reverse.has(e.to)) reverse.set(e.to, []);
    reverse.get(e.to).push(e.from);
  }

  return { forward, reverse };
}

/**
 * Topological sort using Kahn's algorithm.
 * Returns sorted nodes. Nodes in cycles are appended at the end (sorted alphabetically).
 *
 * @param {string[]} nodes - All node IDs to sort
 * @param {Map<string, string[]>} forward - forward adjacency: node → [successors]
 * @returns {{ sorted: string[], remaining: string[] }}
 */
export function topoSort(nodes, forward) {
  const inDegree = new Map();
  for (const n of nodes) inDegree.set(n, 0);

  for (const [src, targets] of forward) {
    for (const t of targets) {
      if (inDegree.has(t)) {
        inDegree.set(t, inDegree.get(t) + 1);
      }
    }
  }

  // Seed queue with zero in-degree nodes, sorted for determinism
  const queue = [];
  for (const [node, deg] of inDegree) {
    if (deg === 0) queue.push(node);
  }
  queue.sort();

  const sorted = [];
  const sortedSet = new Set();

  while (queue.length > 0) {
    const node = queue.shift();
    sorted.push(node);
    sortedSet.add(node);

    for (const next of (forward.get(node) || [])) {
      if (!inDegree.has(next)) continue;
      const newDeg = inDegree.get(next) - 1;
      inDegree.set(next, newDeg);
      if (newDeg === 0) {
        // Insert sorted for deterministic order
        const idx = queue.findIndex(q => q > next);
        if (idx === -1) queue.push(next);
        else queue.splice(idx, 0, next);
      }
    }
  }

  // Nodes not reached are part of cycles
  const remaining = nodes.filter(n => !sortedSet.has(n)).sort();

  return { sorted, remaining };
}

/**
 * Detect cycles in a directed graph via DFS.
 *
 * @param {Map<string, string[]>} forward - forward adjacency
 * @returns {string[][]} Array of cycles, each cycle is an array of node IDs
 */
export function detectCycles(forward) {
  const cycles = [];
  const visited = new Set();
  const onStack = new Set();

  const dfs = (node, path) => {
    if (onStack.has(node)) {
      const cycleStart = path.indexOf(node);
      cycles.push([...path.slice(cycleStart), node]);
      return;
    }
    if (visited.has(node)) return;
    visited.add(node);
    onStack.add(node);
    path.push(node);

    for (const t of (forward.get(node) || [])) {
      dfs(t, path);
    }

    path.pop();
    onStack.delete(node);
  };

  for (const node of forward.keys()) {
    dfs(node, []);
  }

  return cycles;
}

/**
 * Walk a chain from a start node following the forward adjacency.
 * Returns a depth-annotated chain. Handles cycles by tracking visited nodes.
 *
 * @param {string} start
 * @param {Map<string, string[]>} forward
 * @returns {{ node: string, depth: number }[]}
 */
export function walkChain(start, forward) {
  const chain = [];
  const seen = new Set();

  const walk = (node, depth) => {
    if (seen.has(node)) return;
    seen.add(node);
    chain.push({ node, depth });
    for (const t of (forward.get(node) || [])) {
      walk(t, depth + 1);
    }
  };

  walk(start, 0);
  return chain;
}

/**
 * Find root nodes — nodes with no incoming edges (in the relevant edge set).
 *
 * @param {Map<string, string[]>} forward
 * @returns {string[]}
 */
export function findRoots(forward) {
  const hasIncoming = new Set();
  for (const targets of forward.values()) {
    for (const t of targets) hasIncoming.add(t);
  }
  return [...forward.keys()].filter(n => !hasIncoming.has(n));
}
