/**
 * @module lenses/core
 * Five core lenses for git-mind: incomplete, frontier, critical-path, blocked, parallel.
 */

import { defineLens } from '../lens.js';
import { buildAdjacency, topoSort } from '../dag.js';
import { classifyStatus } from '../views.js';

// ── incomplete ─────────────────────────────────────────────────
// Filter to nodes whose status is NOT 'done'.
// Requires node properties to check status.

defineLens('incomplete', (viewResult, nodeProps) => {
  const filtered = viewResult.nodes.filter(id => {
    const props = nodeProps?.get(id);
    const status = props?.status !== undefined ? classifyStatus(props.status) : 'unknown';
    return status !== 'done';
  });
  const nodeSet = new Set(filtered);
  const edges = viewResult.edges.filter(e => nodeSet.has(e.from) || nodeSet.has(e.to));
  return { nodes: filtered, edges, meta: { ...viewResult.meta, lens: 'incomplete' } };
}, { needsProperties: true });

// ── frontier ───────────────────────────────────────────────────
// Filter to leaf nodes: nodes with no outgoing edges in the filtered set.

defineLens('frontier', (viewResult) => {
  const nodeSet = new Set(viewResult.nodes);
  const hasOutgoing = new Set();
  for (const e of viewResult.edges) {
    if (nodeSet.has(e.from) && nodeSet.has(e.to)) {
      hasOutgoing.add(e.from);
    }
  }
  const filtered = viewResult.nodes.filter(n => !hasOutgoing.has(n));
  const resultSet = new Set(filtered);
  const edges = viewResult.edges.filter(e => resultSet.has(e.from) || resultSet.has(e.to));
  return { nodes: filtered, edges, meta: { ...viewResult.meta, lens: 'frontier' } };
});

// ── critical-path ──────────────────────────────────────────────
// Filter to the longest dependency chain via DP on the DAG.
// Uses depends-on + blocks edges.

defineLens('critical-path', (viewResult) => {
  const nodeSet = new Set(viewResult.nodes);
  const depEdges = viewResult.edges.filter(
    e => (e.label === 'depends-on' || e.label === 'blocks') && nodeSet.has(e.from) && nodeSet.has(e.to)
  );

  if (depEdges.length === 0) {
    return { nodes: [], edges: [], meta: { ...viewResult.meta, lens: 'critical-path', chain: [] } };
  }

  const { forward } = buildAdjacency(depEdges);

  // Collect all nodes participating in dependency edges
  const depNodes = new Set();
  for (const e of depEdges) {
    depNodes.add(e.from);
    depNodes.add(e.to);
  }

  // Topological sort for DP ordering
  const { sorted } = topoSort([...depNodes], forward);

  // DP: longest path ending at each node + predecessor tracking
  const dist = new Map();
  const pred = new Map();
  for (const n of sorted) dist.set(n, 0);

  for (const n of sorted) {
    for (const next of (forward.get(n) || [])) {
      if (dist.has(next) && dist.get(n) + 1 > dist.get(next)) {
        dist.set(next, dist.get(n) + 1);
        pred.set(next, n);
      }
    }
  }

  // Find the node with the maximum distance (end of longest path)
  let endNode = sorted[0];
  let maxDist = 0;
  for (const [n, d] of dist) {
    if (d > maxDist) {
      maxDist = d;
      endNode = n;
    }
  }

  // Reconstruct the path by following predecessors
  const path = [];
  let current = endNode;
  while (current !== undefined) {
    path.push(current);
    current = pred.get(current);
  }
  path.reverse();

  const chain = path.map((node, i) => ({ node, depth: i }));
  const chainSet = new Set(path);
  const edges = viewResult.edges.filter(e => chainSet.has(e.from) && chainSet.has(e.to));

  return {
    nodes: path,
    edges,
    meta: { ...viewResult.meta, lens: 'critical-path', chain },
  };
});

// ── blocked ────────────────────────────────────────────────────
// Filter to nodes that have unresolved incoming 'blocks' edges.

defineLens('blocked', (viewResult) => {
  const nodeSet = new Set(viewResult.nodes);
  const blockedNodes = new Set();
  for (const e of viewResult.edges) {
    if (e.label === 'blocks' && nodeSet.has(e.to)) {
      blockedNodes.add(e.to);
    }
  }
  const filtered = viewResult.nodes.filter(n => blockedNodes.has(n));
  const resultSet = new Set(filtered);
  const edges = viewResult.edges.filter(e => resultSet.has(e.from) || resultSet.has(e.to));
  return { nodes: filtered, edges, meta: { ...viewResult.meta, lens: 'blocked' } };
});

// ── parallel ───────────────────────────────────────────────────
// Filter to nodes with no mutual dependency — can execute concurrently.
// A node is "parallel" if it has no depends-on or blocks relationship
// with any other node in the filtered set.

defineLens('parallel', (viewResult) => {
  const nodeSet = new Set(viewResult.nodes);
  const depEdges = viewResult.edges.filter(
    e => (e.label === 'depends-on' || e.label === 'blocks') && nodeSet.has(e.from) && nodeSet.has(e.to)
  );

  // Nodes involved in any dependency relationship
  const dependent = new Set();
  for (const e of depEdges) {
    dependent.add(e.from);
    dependent.add(e.to);
  }

  const filtered = viewResult.nodes.filter(n => !dependent.has(n));
  const resultSet = new Set(filtered);
  const edges = viewResult.edges.filter(e => resultSet.has(e.from) || resultSet.has(e.to));
  return { nodes: filtered, edges, meta: { ...viewResult.meta, lens: 'parallel' } };
});
