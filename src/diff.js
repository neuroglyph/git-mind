/**
 * @module diff
 * Graph diff computation between two historical points in time.
 *
 * Compares two materialized graph snapshots and reports node/edge changes.
 *
 * ## Edge identity invariant
 * Edge uniqueness is `(source, target, type)` — guaranteed by git-warp's
 * `addEdge` semantics (re-adding the same triple updates props, doesn't
 * duplicate). The `edgeKey()` helper relies on this invariant.
 *
 * ## System prefix exclusion
 * System-generated prefixes (`decision`, `commit`, `epoch`) are excluded
 * from diff output, matching `src/export.js` behavior.
 *
 * ## Prefix filter edge inclusion rule
 * When `prefix` option is specified, an edge is included **only if both
 * endpoints pass the prefix filter**. No partial cross-prefix edges.
 */

import { loadGraph } from './graph.js';
import { getEpochForRef } from './epoch.js';
import { extractPrefix } from './validators.js';

/** Node prefixes excluded from diff (system-generated). */
const EXCLUDED_PREFIXES = new Set(['decision', 'commit', 'epoch']);

/**
 * @typedef {object} EdgeDiffEntry
 * @property {string} source
 * @property {string} target
 * @property {string} type
 */

/**
 * @typedef {object} DiffEndpoint
 * @property {string} ref - Original git ref
 * @property {string} sha - Resolved commit SHA (short)
 * @property {number} tick - Lamport tick used for materialization
 * @property {boolean} nearest - Whether a nearest-epoch fallback was used
 */

/**
 * @typedef {object} DiffResult
 * @property {number} schemaVersion
 * @property {DiffEndpoint} from
 * @property {DiffEndpoint} to
 * @property {{ added: string[], removed: string[], total: { before: number, after: number } }} nodes
 * @property {{ added: EdgeDiffEntry[], removed: EdgeDiffEntry[], total: { before: number, after: number } }} edges
 * @property {{ nodesByPrefix: Record<string, {before: number, after: number}>, edgesByType: Record<string, {before: number, after: number}> }} summary
 * @property {{ materializeMs: { a: number, b: number }, diffMs: number, nodeCount: { a: number, b: number }, edgeCount: { a: number, b: number } }} stats
 */

/**
 * Compute a unique key for an edge based on (source, target, type).
 * Uses null byte separator to avoid collisions.
 *
 * @param {{ from: string, to: string, label: string }} edge
 * @returns {string}
 */
export function edgeKey(edge) {
  return `${edge.from}\0${edge.to}\0${edge.label}`;
}

/**
 * Stable comparator for edges: sort by (type, source, target).
 *
 * @param {EdgeDiffEntry} a
 * @param {EdgeDiffEntry} b
 * @returns {number}
 */
export function compareEdge(a, b) {
  return a.type.localeCompare(b.type)
    || a.source.localeCompare(b.source)
    || a.target.localeCompare(b.target);
}

/**
 * Compute summary counts for nodes by prefix and edges by type.
 *
 * @param {string[]} nodesA
 * @param {string[]} nodesB
 * @param {Array<{from: string, to: string, label: string}>} edgesA
 * @param {Array<{from: string, to: string, label: string}>} edgesB
 * @returns {{ nodesByPrefix: Record<string, {before: number, after: number}>, edgesByType: Record<string, {before: number, after: number}> }}
 */
export function computeSummary(nodesA, nodesB, edgesA, edgesB) {
  // Nodes by prefix
  const prefixCountA = {};
  const prefixCountB = {};

  for (const id of nodesA) {
    const p = extractPrefix(id) ?? '(none)';
    prefixCountA[p] = (prefixCountA[p] ?? 0) + 1;
  }
  for (const id of nodesB) {
    const p = extractPrefix(id) ?? '(none)';
    prefixCountB[p] = (prefixCountB[p] ?? 0) + 1;
  }

  const allPrefixes = new Set([...Object.keys(prefixCountA), ...Object.keys(prefixCountB)]);
  const nodesByPrefix = {};
  for (const p of [...allPrefixes].sort()) {
    nodesByPrefix[p] = { before: prefixCountA[p] ?? 0, after: prefixCountB[p] ?? 0 };
  }

  // Edges by type
  const typeCountA = {};
  const typeCountB = {};

  for (const e of edgesA) {
    typeCountA[e.label] = (typeCountA[e.label] ?? 0) + 1;
  }
  for (const e of edgesB) {
    typeCountB[e.label] = (typeCountB[e.label] ?? 0) + 1;
  }

  const allTypes = new Set([...Object.keys(typeCountA), ...Object.keys(typeCountB)]);
  const edgesByType = {};
  for (const t of [...allTypes].sort()) {
    edgesByType[t] = { before: typeCountA[t] ?? 0, after: typeCountB[t] ?? 0 };
  }

  return { nodesByPrefix, edgesByType };
}

/**
 * Compare two materialized graph instances and return a diff.
 * This is a pure comparison — both graphs must already be materialized.
 *
 * @param {import('@git-stunts/git-warp').default} graphA - "before" graph
 * @param {import('@git-stunts/git-warp').default} graphB - "after" graph
 * @param {{ prefix?: string }} [opts]
 * @returns {Promise<{ nodes: { added: string[], removed: string[], total: { before: number, after: number } }, edges: { added: EdgeDiffEntry[], removed: EdgeDiffEntry[], total: { before: number, after: number } }, summary: object }>}
 */
export async function diffSnapshots(graphA, graphB, opts = {}) {
  const prefixFilter = opts.prefix ?? null;

  const allNodesA = await graphA.getNodes();
  const allNodesB = await graphB.getNodes();

  // Filter nodes: exclude system prefixes, apply prefix filter
  const filterNode = (id) => {
    const prefix = extractPrefix(id);
    if (prefix && EXCLUDED_PREFIXES.has(prefix)) return false;
    if (prefixFilter && prefix !== prefixFilter) return false;
    return true;
  };

  const nodesA = allNodesA.filter(filterNode);
  const nodesB = allNodesB.filter(filterNode);

  const setA = new Set(nodesA);
  const setB = new Set(nodesB);

  const addedNodes = nodesB.filter(id => !setA.has(id)).sort();
  const removedNodes = nodesA.filter(id => !setB.has(id)).sort();

  // Edges: only include edges where both endpoints are in the filtered node set
  const allEdgesA = await graphA.getEdges();
  const allEdgesB = await graphB.getEdges();

  const filterEdge = (edge, nodeSet) => {
    return nodeSet.has(edge.from) && nodeSet.has(edge.to);
  };

  const edgesA = allEdgesA.filter(e => filterEdge(e, setA));
  const edgesB = allEdgesB.filter(e => filterEdge(e, setB));

  const edgeMapA = new Set(edgesA.map(edgeKey));
  const edgeMapB = new Set(edgesB.map(edgeKey));

  const addedEdges = edgesB
    .filter(e => !edgeMapA.has(edgeKey(e)))
    .map(e => ({ source: e.from, target: e.to, type: e.label }))
    .sort(compareEdge);

  const removedEdges = edgesA
    .filter(e => !edgeMapB.has(edgeKey(e)))
    .map(e => ({ source: e.from, target: e.to, type: e.label }))
    .sort(compareEdge);

  const summary = computeSummary(nodesA, nodesB, edgesA, edgesB);

  return {
    nodes: {
      added: addedNodes,
      removed: removedNodes,
      total: { before: nodesA.length, after: nodesB.length },
    },
    edges: {
      added: addedEdges,
      removed: removedEdges,
      total: { before: edgesA.length, after: edgesB.length },
    },
    summary,
  };
}

/**
 * Parse diff ref arguments from CLI args.
 * Supports: `A..B`, `A B`, `A` (shorthand for `A..HEAD`).
 * Rejects: `A..B..C`, `..B`, `A..`, empty.
 *
 * @param {string[]} args - Non-flag arguments after the `diff` command
 * @returns {{ refA: string, refB: string }}
 */
export function parseDiffRefs(args) {
  if (!args || args.length === 0) {
    throw new Error('Usage: git mind diff <ref-a>..<ref-b>  or  git mind diff <ref-a> <ref-b>  or  git mind diff <ref>');
  }

  // Two-arg syntax: A B
  if (args.length >= 2) {
    return { refA: args[0], refB: args[1] };
  }

  // Single arg — might be range syntax or shorthand
  const arg = args[0];

  // Check for range syntax
  const parts = arg.split('..');
  if (parts.length > 2) {
    throw new Error(`Malformed range: "${arg}". Expected "ref-a..ref-b", got multiple ".." separators.`);
  }
  if (parts.length === 2) {
    if (!parts[0]) {
      throw new Error(`Malformed range: "${arg}". Left side of ".." is empty.`);
    }
    if (!parts[1]) {
      throw new Error(`Malformed range: "${arg}". Right side of ".." is empty.`);
    }
    return { refA: parts[0], refB: parts[1] };
  }

  // Single ref — shorthand for ref..HEAD
  return { refA: arg, refB: 'HEAD' };
}

/** Boolean flags that don't consume the next arg. */
const DIFF_BOOLEAN_FLAGS = new Set(['json']);

/**
 * Collect positional args from a diff command's arg list,
 * skipping --flag and their consumed values.
 *
 * @param {string[]} args - Args after the `diff` command word
 * @returns {string[]} Positional (non-flag) arguments
 */
export function collectDiffPositionals(args) {
  const positionals = [];
  for (let i = 0; i < args.length; i++) {
    if (args[i].startsWith('--')) {
      const name = args[i].slice(2);
      if (!DIFF_BOOLEAN_FLAGS.has(name)) i++; // skip the flag's value
    } else {
      positionals.push(args[i]);
    }
  }
  return positionals;
}

/**
 * Full diff orchestrator: resolve epochs, materialize graphs, compute diff.
 *
 * Opens three graph instances:
 * 1. Resolver — unmaterialized, used to resolve both refs to epoch ticks
 * 2. Graph A — materialized at tick A ("before")
 * 3. Graph B — materialized at tick B ("after")
 *
 * Three instances are needed because `materialize({ ceiling })` is
 * destructive (no unmaterialize).
 *
 * @param {string} cwd - Repository working directory
 * @param {string} refA - "before" git ref
 * @param {string} refB - "after" git ref
 * @param {{ prefix?: string }} [opts]
 * @returns {Promise<DiffResult>}
 */
export async function computeDiff(cwd, refA, refB, opts = {}) {
  // 1. Resolve both refs to epochs using a resolver graph
  const resolver = await loadGraph(cwd, { writerId: 'diff-resolver' });

  const resultA = await getEpochForRef(resolver, cwd, refA);
  if (!resultA) {
    throw new Error(`No epoch found for "${refA}" or its ancestors. Run "git mind process-commit" to record epoch markers.`);
  }

  const resultB = await getEpochForRef(resolver, cwd, refB);
  if (!resultB) {
    throw new Error(`No epoch found for "${refB}" or its ancestors. Run "git mind process-commit" to record epoch markers.`);
  }

  const tickA = resultA.epoch.tick;
  const tickB = resultB.epoch.tick;

  // Same tick → empty diff
  if (tickA === tickB) {
    return {
      schemaVersion: 1,
      from: {
        ref: refA,
        sha: resultA.sha.slice(0, 8),
        tick: tickA,
        nearest: resultA.epoch.nearest ?? false,
      },
      to: {
        ref: refB,
        sha: resultB.sha.slice(0, 8),
        tick: tickB,
        nearest: resultB.epoch.nearest ?? false,
      },
      nodes: { added: [], removed: [], total: { before: 0, after: 0 } },
      edges: { added: [], removed: [], total: { before: 0, after: 0 } },
      summary: { nodesByPrefix: {}, edgesByType: {} },
      stats: {
        sameTick: true,
        materializeMs: { a: 0, b: 0 },
        diffMs: 0,
        nodeCount: { a: 0, b: 0 },
        edgeCount: { a: 0, b: 0 },
      },
    };
  }

  // 2. Materialize two separate graph instances
  const startA = Date.now();
  const graphA = await loadGraph(cwd, { writerId: 'diff-a' });
  await graphA.materialize({ ceiling: tickA });
  const msA = Date.now() - startA;

  const startB = Date.now();
  const graphB = await loadGraph(cwd, { writerId: 'diff-b' });
  await graphB.materialize({ ceiling: tickB });
  const msB = Date.now() - startB;

  // 3. Compute diff
  const startDiff = Date.now();
  const diff = await diffSnapshots(graphA, graphB, { prefix: opts.prefix });
  const msDiff = Date.now() - startDiff;

  return {
    schemaVersion: 1,
    from: {
      ref: refA,
      sha: resultA.sha.slice(0, 8),
      tick: tickA,
      nearest: resultA.epoch.nearest ?? false,
    },
    to: {
      ref: refB,
      sha: resultB.sha.slice(0, 8),
      tick: tickB,
      nearest: resultB.epoch.nearest ?? false,
    },
    nodes: diff.nodes,
    edges: diff.edges,
    summary: diff.summary,
    stats: {
      materializeMs: { a: msA, b: msB },
      diffMs: msDiff,
      nodeCount: { a: diff.nodes.total.before, b: diff.nodes.total.after },
      edgeCount: { a: diff.edges.total.before, b: diff.edges.total.after },
    },
  };
}
