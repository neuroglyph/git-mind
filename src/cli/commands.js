/**
 * @module cli/commands
 * Command implementations for the git-mind CLI.
 */

import { initGraph, loadGraph } from '../graph.js';
import { createEdge, queryEdges, removeEdge, EDGE_TYPES } from '../edges.js';
import { renderView, listViews } from '../views.js';
import { success, error, info, formatEdge, formatView } from './format.js';

/**
 * Initialize a git-mind graph in the current repo.
 * @param {string} cwd
 */
export async function init(cwd) {
  try {
    const graph = await initGraph(cwd);
    console.log(success('Initialized git-mind graph'));
  } catch (err) {
    console.error(error(`Failed to initialize: ${err.message}`));
    process.exitCode = 1;
  }
}

/**
 * Create a link (edge) between two nodes.
 * @param {string} cwd
 * @param {string} source
 * @param {string} target
 * @param {{ type?: string, confidence?: number }} opts
 */
export async function link(cwd, source, target, opts = {}) {
  const type = opts.type ?? 'relates-to';

  try {
    const graph = await loadGraph(cwd);
    await createEdge(graph, { source, target, type, confidence: opts.confidence });
    console.log(success(`${source} --[${type}]--> ${target}`));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Render a named view.
 * @param {string} cwd
 * @param {string} viewName
 */
export async function view(cwd, viewName) {
  if (!viewName) {
    console.log(info(`Available views: ${listViews().join(', ')}`));
    return;
  }

  try {
    const graph = await loadGraph(cwd);
    const result = await renderView(graph, viewName);
    console.log(formatView(viewName, result));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * List edges in the graph, optionally filtered.
 * @param {string} cwd
 * @param {{ type?: string, source?: string, target?: string }} [filter={}]
 */
export async function list(cwd, filter = {}) {
  try {
    const graph = await loadGraph(cwd);
    const edges = await queryEdges(graph, filter);

    if (edges.length === 0) {
      console.log(info('No edges found'));
      return;
    }

    console.log(info(`${edges.length} edge(s):`));
    for (const edge of edges) {
      console.log(formatEdge(edge));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Remove an edge between two nodes.
 * @param {string} cwd
 * @param {string} source
 * @param {string} target
 * @param {{ type?: string }} opts
 */
export async function remove(cwd, source, target, opts = {}) {
  const type = opts.type ?? 'relates-to';

  try {
    const graph = await loadGraph(cwd);
    await removeEdge(graph, source, target, type);
    console.log(success(`Removed: ${source} --[${type}]--> ${target}`));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Stub: AI suggestions.
 */
export async function suggest() {
  console.log(info('AI suggestions not yet implemented'));
}

/**
 * Stub: review edges.
 */
export async function review() {
  console.log(info('Edge review not yet implemented'));
}
