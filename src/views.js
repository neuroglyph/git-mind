/**
 * @module views
 * Observer view definitions and rendering for git-mind.
 * Views are filtered projections of the graph.
 */

/** @type {Map<string, ViewDefinition>} */
const registry = new Map();

/**
 * @typedef {object} ViewDefinition
 * @property {string} name
 * @property {(nodes: string[], edges: Array<{from: string, to: string, label: string, props: object}>) => {nodes: string[], edges: Array<{from: string, to: string, label: string, props: object}>}} filterFn
 */

/**
 * Register a named view.
 *
 * @param {string} name
 * @param {ViewDefinition['filterFn']} filterFn
 */
export function defineView(name, filterFn) {
  registry.set(name, { name, filterFn });
}

/**
 * Render a named view against the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} viewName
 * @returns {Promise<{nodes: string[], edges: Array<{from: string, to: string, label: string, props: object}>}>}
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

// --- Built-in views ---

defineView('roadmap', (nodes, edges) => {
  // Nodes that look like phases or tasks (by ID prefix convention)
  const phaseOrTask = nodes.filter(n => n.startsWith('phase:') || n.startsWith('task:'));
  const relevantEdges = edges.filter(
    e => phaseOrTask.includes(e.from) || phaseOrTask.includes(e.to)
  );
  return { nodes: phaseOrTask, edges: relevantEdges };
});

defineView('architecture', (nodes, edges) => {
  // Nodes that look like crates/modules and depends-on edges
  const modules = nodes.filter(n =>
    n.startsWith('crate:') || n.startsWith('module:') || n.startsWith('pkg:')
  );
  const depEdges = edges.filter(
    e => e.label === 'depends-on' && modules.includes(e.from) && modules.includes(e.to)
  );
  return { nodes: modules, edges: depEdges };
});

defineView('backlog', (nodes, edges) => {
  // All task nodes
  const tasks = nodes.filter(n => n.startsWith('task:'));
  const taskEdges = edges.filter(
    e => tasks.includes(e.from) || tasks.includes(e.to)
  );
  return { nodes: tasks, edges: taskEdges };
});

defineView('suggestions', (nodes, edges) => {
  // Edges with low confidence (AI-suggested, not yet reviewed)
  const lowConfEdges = edges.filter(e => {
    const conf = e.props?.confidence;
    return typeof conf === 'number' && conf < 0.5;
  });
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
