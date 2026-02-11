/**
 * @module views
 * Declarative view engine for git-mind.
 * Views are filtered, computed projections of the graph.
 */

import { extractPrefix, isLowConfidence } from './validators.js';

/** @type {Map<string, ViewDefinition>} */
const registry = new Map();

/**
 * @typedef {object} ViewDefinition
 * @property {string} name
 * @property {string} [description]
 * @property {(nodes: string[], edges: Edge[]) => ViewResult} filterFn
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
 */
export function defineView(name, filterFn) {
  registry.set(name, { name, filterFn });
}

/**
 * Register a declarative view from a config object.
 * Compiles the config into a filter function.
 *
 * @param {string} name
 * @param {ViewConfig} config
 */
export function declareView(name, config) {
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
 * Render a named view against the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} viewName
 * @returns {Promise<ViewResult>}
 */
export async function renderView(graph, viewName) {
  const view = registry.get(viewName);
  if (!view) {
    const available = [...registry.keys()].join(', ');
    throw new Error(`Unknown view: "${viewName}". Available views: ${available}`);
  }

  const nodes = await graph.getNodes();
  const edges = await graph.getEdges();

  return view.filterFn(nodes, edges);
}

/**
 * List all registered view names.
 * @returns {string[]}
 */
export function listViews() {
  return [...registry.keys()];
}

/** @type {Set<string>} Built-in view names, captured after registration */
let builtInNames;

/**
 * Remove all views that were not registered at module load time.
 * Intended for test cleanup.
 */
export function resetViews() {
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
    e => relevant.has(e.from) || relevant.has(e.to)
  );

  // Pre-compute set of nodes that have an outgoing 'implements' edge
  const hasImplements = new Set(
    edges.filter(e => e.label === 'implements').map(e => e.from)
  );

  // Compute per-milestone stats
  const milestoneStats = {};
  for (const m of milestones) {
    // Tasks and features that belong-to this milestone
    const children = edges
      .filter(e => e.label === 'belongs-to' && e.to === m
        && (e.from.startsWith('task:') || e.from.startsWith('feature:')))
      .map(e => e.from);

    // A child is "done" if it has at least one 'implements' edge pointing from it
    const done = children.filter(child => hasImplements.has(child));

    // Blockers: tasks that block children of this milestone
    const childSet = new Set(children);
    const blockers = edges
      .filter(e => e.label === 'blocks' && childSet.has(e.to))
      .map(e => e.from);

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

  // Build adjacency list: blocker -> [blocked]
  const adj = new Map();
  for (const e of blockEdges) {
    if (!adj.has(e.from)) adj.set(e.from, []);
    adj.get(e.from).push(e.to);
  }

  // Find all transitive chains from each root blocker
  const chains = [];
  const cycles = [];
  const allInvolved = new Set();

  // DFS from each node that has outgoing blocks edges
  const visited = new Set();
  for (const root of adj.keys()) {
    const path = [];
    const onStack = new Set();

    const dfs = (node) => {
      if (onStack.has(node)) {
        // Cycle detected
        const cycleStart = path.indexOf(node);
        cycles.push([...path.slice(cycleStart), node]);
        return;
      }
      if (visited.has(node)) return;
      visited.add(node);
      path.push(node);
      onStack.add(node);
      allInvolved.add(node);

      const targets = adj.get(node) || [];
      for (const t of targets) {
        allInvolved.add(t);
        dfs(t);
      }
      path.pop();
      onStack.delete(node);
    };

    dfs(root);
  }

  // Build chains: root blockers (nodes that block others but aren't blocked themselves)
  const blocked = new Set(blockEdges.map(e => e.to));
  const roots = [...adj.keys()].filter(n => !blocked.has(n));

  for (const root of roots) {
    const chain = [];
    const seen = new Set();
    const buildChain = (node, depth) => {
      if (seen.has(node)) return;
      seen.add(node);
      chain.push({ node, depth });
      for (const t of (adj.get(node) || [])) {
        buildChain(t, depth + 1);
      }
    };
    buildChain(root, 0);
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
  // or pointing to doc nodes
  const relevantTypes = new Set(['depends-on', 'documents', 'implements']);
  const docEdges = edges.filter(e =>
    relevantTypes.has(e.label) && (docSet.has(e.from) || docSet.has(e.to))
  );

  // Build dependency graph for topological sort
  // An edge A --[depends-on]--> B means read B before A
  // An edge A --[documents]--> B means read A alongside B (no strict order)
  const inDegree = new Map();
  const adj = new Map();
  for (const n of docNodes) {
    inDegree.set(n, 0);
    adj.set(n, []);
  }

  for (const e of edges) {
    // Only consider depends-on between doc nodes for ordering
    if (e.label === 'depends-on' && docSet.has(e.from) && docSet.has(e.to)) {
      adj.get(e.to).push(e.from); // B should come before A
      inDegree.set(e.from, (inDegree.get(e.from) || 0) + 1);
    }
  }

  // Kahn's algorithm
  const queue = [];
  for (const [node, deg] of inDegree) {
    if (deg === 0) queue.push(node);
  }
  queue.sort(); // deterministic tie-break: alphabetical

  const sorted = [];
  const sortedSet = new Set();
  while (queue.length > 0) {
    const node = queue.shift();
    sorted.push(node);
    sortedSet.add(node);
    for (const next of (adj.get(node) || [])) {
      const newDeg = inDegree.get(next) - 1;
      inDegree.set(next, newDeg);
      if (newDeg === 0) {
        // Insert sorted to maintain deterministic order
        const idx = queue.findIndex(q => q > next);
        if (idx === -1) queue.push(next);
        else queue.splice(idx, 0, next);
      }
    }
  }

  // Nodes not reached (part of a cycle) go at the end
  const remaining = docNodes.filter(n => !sortedSet.has(n)).sort();

  return {
    nodes: [...sorted, ...remaining],
    edges: docEdges,
    meta: {
      readingOrder: [...sorted, ...remaining],
      hasCycles: remaining.length > 0,
    },
  };
});

// Capture built-in names after all registrations
builtInNames = new Set(registry.keys());
