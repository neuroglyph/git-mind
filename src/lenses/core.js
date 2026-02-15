/**
 * @module lenses/core
 * Five core lenses for git-mind: incomplete, frontier, critical-path, blocked, parallel.
 */

import { defineLens } from '../lens.js';
import { buildAdjacency, topoSort, findRoots, walkChain } from '../dag.js';
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
// Filter to the longest dependency chain. Uses depends-on + blocks edges.

defineLens('critical-path', (viewResult) => {
  // Build adjacency from depends-on and blocks edges within the view
  const nodeSet = new Set(viewResult.nodes);
  const depEdges = viewResult.edges.filter(
    e => (e.label === 'depends-on' || e.label === 'blocks') && nodeSet.has(e.from) && nodeSet.has(e.to)
  );
  const { forward } = buildAdjacency(depEdges);

  // Find roots and walk chains from each, keep the longest
  const roots = findRoots(forward);
  let longestChain = [];

  for (const root of roots) {
    const chain = walkChain(root, forward);
    if (chain.length > longestChain.length) {
      longestChain = chain;
    }
  }

  // If no dependency edges, fall back to topo sort of all nodes
  if (longestChain.length === 0 && viewResult.nodes.length > 0) {
    return { nodes: [], edges: [], meta: { ...viewResult.meta, lens: 'critical-path', chain: [] } };
  }

  const chainNodes = longestChain.map(c => c.node);
  const chainSet = new Set(chainNodes);
  const edges = viewResult.edges.filter(e => chainSet.has(e.from) && chainSet.has(e.to));

  return {
    nodes: chainNodes,
    edges,
    meta: { ...viewResult.meta, lens: 'critical-path', chain: longestChain },
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
