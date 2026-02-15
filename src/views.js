/**
 * @module views
 * Declarative view engine for git-mind.
 * Views are filtered, computed projections of the graph.
 */

import { extractPrefix, isLowConfidence } from './validators.js';
import { composeLenses } from './lens.js';
import { buildAdjacency, topoSort, detectCycles, walkChain, findRoots } from './dag.js';

// ── Status classification ───────────────────────────────────────

const SYNONYMS = new Map([
  ['in_progress', 'in-progress'],
  ['inprogress', 'in-progress'],
  ['wip', 'in-progress'],
  ['complete', 'done'],
  ['completed', 'done'],
  ['finished', 'done'],
]);
const VALID_STATUSES = new Set(['done', 'in-progress', 'todo', 'blocked']);

/**
 * Normalize a status value to a canonical form.
 * Trims, lowercases, maps synonyms, returns 'unknown' for unrecognized.
 *
 * @param {unknown} value
 * @returns {'done' | 'in-progress' | 'todo' | 'blocked' | 'unknown'}
 */
export function classifyStatus(value) {
  if (typeof value !== 'string') return 'unknown';
  const normalized = value.trim().toLowerCase();
  const mapped = SYNONYMS.get(normalized) ?? normalized;
  return VALID_STATUSES.has(mapped) ? mapped : 'unknown';
}

/** @type {Map<string, ViewDefinition>} */
const registry = new Map();

/**
 * @typedef {object} ViewDefinition
 * @property {string} name
 * @property {string} [description]
 * @property {(nodes: string[], edges: Edge[], nodeProps?: Map<string, Record<string, unknown>>, options?: Record<string, unknown>) => ViewResult} filterFn
 * @property {boolean} [needsProperties]
 */

/**
 * @typedef {object} Edge
 * @property {string} from
 * @property {string} to
 * @property {string} label
 * @property {Record<string, unknown>} [props]
 */

/**
 * @typedef {object} ViewResult
 * @property {string[]} nodes
 * @property {Edge[]} edges
 * @property {Record<string, unknown>} [meta] - Optional computed metadata
 */

/**
 * @typedef {object} ViewConfig
 * @property {string[]} prefixes - Node prefix filter
 * @property {string[]} [edgeTypes] - Edge type filter (default: all)
 * @property {boolean} [requireBothEndpoints=false] - If true, both edge endpoints must be in filtered nodes
 * @property {string} [description]
 */

/**
 * Register a named view with a filter function.
 *
 * @param {string} name
 * @param {ViewDefinition['filterFn']} filterFn
 * @param {{ needsProperties?: boolean }} [opts]
 */
export function defineView(name, filterFn, opts = {}) {
  registry.set(name, { name, filterFn, needsProperties: opts.needsProperties ?? false });
}

/**
 * Register a declarative view from a config object.
 * Compiles the config into a filter function.
 *
 * @param {string} name
 * @param {ViewConfig} config
 */
export function declareView(name, config) {
  if (!Array.isArray(config.prefixes) || config.prefixes.length === 0) {
    throw new Error(`declareView("${name}"): config.prefixes must be a non-empty array`);
  }
  const prefixSet = new Set(config.prefixes);
  const typeSet = config.edgeTypes ? new Set(config.edgeTypes) : null;
  const bothEndpoints = config.requireBothEndpoints ?? false;

  const filterFn = (nodes, edges) => {
    const matchedNodes = nodes.filter(n => {
      const prefix = extractPrefix(n);
      return prefix && prefixSet.has(prefix);
    });
    const nodeSet = new Set(matchedNodes);

    const matchedEdges = edges.filter(e => {
      if (typeSet && !typeSet.has(e.label)) return false;
      if (bothEndpoints) return nodeSet.has(e.from) && nodeSet.has(e.to);
      return nodeSet.has(e.from) || nodeSet.has(e.to);
    });

    return { nodes: matchedNodes, edges: matchedEdges };
  };

  registry.set(name, { name, description: config.description, filterFn });
}

/**
 * Render a named view against the graph, optionally applying lenses.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} viewName
 * @param {Record<string, unknown> & { lenses?: string[] }} [options] - Options forwarded to the view's filterFn; `lenses` applies post-processing
 * @returns {Promise<ViewResult>}
 */
export async function renderView(graph, viewName, options) {
  const view = registry.get(viewName);
  if (!view) {
    const available = [...registry.keys()].join(', ');
    throw new Error(`Unknown view: "${viewName}". Available views: ${available}`);
  }

  // Resolve lens composition (may throw on unknown lens)
  const lensNames = options?.lenses ?? [];
  const { composedFn, needsProperties: lensNeedsProps } = composeLenses(lensNames);

  const nodes = await graph.getNodes();
  const edges = await graph.getEdges();

  // Fetch node properties when the view OR any lens needs them
  let nodeProps;
  if (view.needsProperties || lensNeedsProps) {
    nodeProps = new Map();
    // TODO: batch API at scale — O(N) getNodeProps calls is fine at ~50 nodes
    for (const id of nodes) {
      const propsMap = await graph.getNodeProps(id);
      if (propsMap && propsMap.size > 0) {
        const props = {};
        for (const [k, v] of propsMap) props[k] = v;
        nodeProps.set(id, props);
      }
    }
  }

  let result = view.filterFn(nodes, edges, nodeProps, options);

  // Apply composed lenses
  if (lensNames.length > 0) {
    result = composedFn(result, nodeProps, options);
  }

  return result;
}

/**
 * List all registered view names.
 * @returns {string[]}
 */
export function listViews() {
  return [...registry.keys()];
}

/** @type {Set<string>} Built-in view names, captured after registration */
let builtInNames = new Set();

/**
 * Remove all views that were not registered at module load time.
 * Intended for test cleanup.
 */
export function resetViews() {
  // Safe: ES Map iterators handle deletion of not-yet-visited entries
  for (const name of registry.keys()) {
    if (!builtInNames.has(name)) registry.delete(name);
  }
}

// ── Built-in declarative views ──────────────────────────────────

declareView('roadmap', {
  description: 'Phase and task nodes with all related edges',
  prefixes: ['phase', 'task'],
});

declareView('architecture', {
  description: 'Module nodes with depends-on edges',
  prefixes: ['crate', 'module', 'pkg'],
  edgeTypes: ['depends-on'],
  requireBothEndpoints: true,
});

declareView('backlog', {
  description: 'Task nodes with all related edges',
  prefixes: ['task'],
});

// ── Built-in imperative views ───────────────────────────────────

defineView('suggestions', (nodes, edges) => {
  // Edges with low confidence (AI-suggested, not yet reviewed)
  const lowConfEdges = edges.filter(isLowConfidence);
  const involvedNodes = new Set();
  for (const e of lowConfEdges) {
    involvedNodes.add(e.from);
    involvedNodes.add(e.to);
  }
  return {
    nodes: nodes.filter(n => involvedNodes.has(n)),
    edges: lowConfEdges,
  };
});

// ── PRISM views ─────────────────────────────────────────────────

defineView('milestone', (nodes, edges) => {
  // Milestone progress: find milestones and their children (tasks + features
  // linked via belongs-to), then compute completion stats per milestone.
  const milestones = nodes.filter(n => n.startsWith('milestone:'));
  const tasks = nodes.filter(n => n.startsWith('task:'));
  const features = nodes.filter(n => n.startsWith('feature:'));
  const relevant = new Set([...milestones, ...tasks, ...features]);

  const relevantEdges = edges.filter(
    e => relevant.has(e.from) && relevant.has(e.to)
  );

  // Pre-compute set of nodes that have an outgoing 'implements' edge
  const hasImplements = new Set(
    edges.filter(e => e.label === 'implements').map(e => e.from)
  );

  // Pre-index belongs-to and blocks edges by target for O(E + M) lookups
  const belongsToByTarget = new Map();
  const blocksByTarget = new Map();
  for (const e of edges) {
    if (e.label === 'belongs-to') {
      if (!belongsToByTarget.has(e.to)) belongsToByTarget.set(e.to, []);
      belongsToByTarget.get(e.to).push(e);
    } else if (e.label === 'blocks') {
      if (!blocksByTarget.has(e.to)) blocksByTarget.set(e.to, []);
      blocksByTarget.get(e.to).push(e);
    }
  }

  // Compute per-milestone stats
  const milestoneStats = {};
  for (const m of milestones) {
    // Tasks and features that belong-to this milestone
    const children = (belongsToByTarget.get(m) || [])
      .filter(e => e.from.startsWith('task:') || e.from.startsWith('feature:'))
      .map(e => e.from);

    // A child is "done" if it has at least one 'implements' edge pointing from it
    const done = children.filter(child => hasImplements.has(child));

    // Blockers: tasks that block children of this milestone
    const blockers = [];
    for (const child of children) {
      for (const e of (blocksByTarget.get(child) || [])) {
        blockers.push(e.from);
      }
    }

    milestoneStats[m] = {
      total: children.length,
      done: done.length,
      pct: children.length > 0 ? Math.round((done.length / children.length) * 100) : 0,
      blockers: [...new Set(blockers)],
    };
  }

  return {
    nodes: [...relevant],
    edges: relevantEdges,
    meta: { milestoneStats },
  };
});

defineView('traceability', (nodes, edges) => {
  // Spec-to-implementation gap analysis.
  // Find specs, then check which have 'implements' edges pointing at them.
  const specs = nodes.filter(n => n.startsWith('spec:') || n.startsWith('adr:'));
  const specSet = new Set(specs);
  const implementsEdges = edges.filter(e => e.label === 'implements');

  const implemented = new Set(implementsEdges.map(e => e.to));
  const gaps = specs.filter(s => !implemented.has(s));
  const covered = specs.filter(s => implemented.has(s));

  // Include all implements edges + the spec/impl nodes
  const implNodes = new Set(specs);
  for (const e of implementsEdges) {
    if (specSet.has(e.to)) {
      implNodes.add(e.from);
    }
  }

  const relevantEdges = implementsEdges.filter(e => specSet.has(e.to));

  return {
    nodes: [...implNodes],
    edges: relevantEdges,
    meta: {
      gaps,
      covered,
      coveragePct: specs.length > 0 ? Math.round((covered.length / specs.length) * 100) : 100,
    },
  };
});

defineView('blockers', (nodes, edges) => {
  // Transitive blocking chain resolution with cycle detection.
  const blockEdges = edges.filter(e => e.label === 'blocks');
  const { forward } = buildAdjacency(blockEdges);

  const cycles = detectCycles(forward);
  const roots = findRoots(forward);

  // Collect all involved nodes
  const allInvolved = new Set();
  for (const e of blockEdges) {
    allInvolved.add(e.from);
    allInvolved.add(e.to);
  }

  // Build chains from each root blocker
  const chains = [];
  for (const root of roots) {
    const chain = walkChain(root, forward);
    if (chain.length > 1) chains.push(chain);
  }

  return {
    nodes: [...allInvolved],
    edges: blockEdges,
    meta: { chains, cycles, rootBlockers: roots },
  };
});

defineView('onboarding', (nodes, edges) => {
  // Topologically-sorted reading order for new engineers.
  // Uses doc/spec/adr nodes, ordered by dependency edges.
  const docNodes = nodes.filter(n =>
    n.startsWith('doc:') || n.startsWith('spec:') || n.startsWith('adr:')
  );
  const docSet = new Set(docNodes);

  // Relevant edges: depends-on, implements, documents between doc nodes
  const relevantTypes = new Set(['depends-on', 'documents', 'implements']);
  const docEdges = edges.filter(e =>
    relevantTypes.has(e.label) && docSet.has(e.from) && docSet.has(e.to)
  );

  // Build dependency graph for topological sort
  // An edge A --[depends-on]--> B means read B before A
  // So the "forward" direction for topoSort is: B → A (dependency target comes first)
  const forward = new Map();
  for (const n of docNodes) forward.set(n, []);
  for (const e of docEdges) {
    if (e.label === 'depends-on') {
      forward.get(e.to).push(e.from); // B should come before A
    }
  }

  const { sorted, remaining } = topoSort(docNodes, forward);

  return {
    nodes: [...sorted, ...remaining],
    edges: docEdges,
    meta: {
      readingOrder: [...sorted, ...remaining],
      hasCycles: remaining.length > 0,
    },
  };
});

defineView('coverage', (nodes, edges) => {
  // Code-to-spec gap analysis: which code nodes lack implements edges to specs?
  const codePrefixes = new Set(['crate', 'module', 'pkg']);
  const specPrefixes = new Set(['spec', 'adr']);

  const codeNodes = nodes.filter(n => {
    const p = extractPrefix(n);
    return p && codePrefixes.has(p);
  });
  const specNodes = new Set(
    nodes.filter(n => {
      const p = extractPrefix(n);
      return p && specPrefixes.has(p);
    })
  );

  // Code nodes that have at least one implements edge to a spec/adr
  const linkedSet = new Set(
    edges
      .filter(e => e.label === 'implements' && specNodes.has(e.to))
      .map(e => e.from)
  );

  const linked = codeNodes.filter(n => linkedSet.has(n));
  const unlinked = codeNodes.filter(n => !linkedSet.has(n));

  // Self-contained subgraph: edges where both endpoints are in the result
  const resultNodes = new Set([...codeNodes, ...specNodes]);
  const resultEdges = edges.filter(
    e => e.label === 'implements' && resultNodes.has(e.from) && resultNodes.has(e.to)
  );

  return {
    nodes: [...resultNodes],
    edges: resultEdges,
    meta: {
      linked,
      unlinked,
      coveragePct: codeNodes.length > 0
        ? Math.round((linked.length / codeNodes.length) * 100)
        : 100,
    },
  };
});

defineView('progress', (nodes, edges, nodeProps, options) => {
  // Group work-item nodes by their status property.
  // --scope narrows which prefixes count as work items (default: task + feature).
  const prefixes = options?.scope ?? ['task', 'feature'];
  const workItems = nodes.filter(n => {
    const p = extractPrefix(n);
    return p && prefixes.includes(p);
  });

  const byStatus = {
    'done': [],
    'in-progress': [],
    'todo': [],
    'blocked': [],
    'unknown': [],
  };

  for (const id of workItems) {
    const props = nodeProps?.get(id);
    const raw = props?.status;
    const status = raw !== undefined ? classifyStatus(raw) : 'unknown';
    byStatus[status].push(id);
  }

  // Deterministic ordering within each bucket
  for (const bucket of Object.values(byStatus)) bucket.sort();

  const total = workItems.length;
  const done = byStatus['done'].length;
  const pct = total > 0 ? Math.round((done / total) * 100) : 0;

  // Include edges between work items
  const workSet = new Set(workItems);
  const relevantEdges = edges.filter(e => workSet.has(e.from) || workSet.has(e.to));

  return {
    nodes: workItems,
    edges: relevantEdges,
    meta: {
      byStatus,
      summary: {
        total,
        done,
        'in-progress': byStatus['in-progress'].length,
        todo: byStatus['todo'].length,
        blocked: byStatus['blocked'].length,
        unknown: byStatus['unknown'].length,
        pct,
        ratio: `${done}/${total}`,
        remaining: total - done,
      },
    },
  };
}, { needsProperties: true });

// Capture built-in names after all registrations
builtInNames = new Set(registry.keys());

// ── Register core lenses (side-effect import) + capture ─────────
import './lenses/core.js';
import { captureBuiltIns as captureLensBuiltIns } from './lens.js';
captureLensBuiltIns();
