/**
 * @module graph
 * Wraps @git-stunts/git-warp graph operations for git-mind.
 */

import WarpGraph, { GitGraphAdapter } from '@git-stunts/git-warp';
import GitPlumbing, { ShellRunnerFactory } from '@git-stunts/plumbing';
import { resolve } from 'node:path';

const GRAPH_NAME = 'gitmind';

/**
 * Initialize a new git-mind graph in a repository.
 * @param {string} repoPath - Path to the Git repository
 * @param {{ writerId?: string }} [opts]
 * @returns {Promise<import('@git-stunts/git-warp').default>}
 */
export async function initGraph(repoPath, opts = {}) {
  const cwd = resolve(repoPath);
  const writerId = opts.writerId ?? 'local';

  const runner = ShellRunnerFactory.create();
  const plumbing = new GitPlumbing({ cwd, runner });
  const persistence = new GitGraphAdapter({ plumbing });

  const graph = await WarpGraph.open({
    graphName: GRAPH_NAME,
    persistence,
    writerId,
    autoMaterialize: true,
  });

  return graph;
}

/**
 * Load an existing git-mind graph from a repository.
 * @param {string} repoPath - Path to the Git repository
 * @param {{ writerId?: string }} [opts]
 * @returns {Promise<import('@git-stunts/git-warp').default>}
 */
export async function loadGraph(repoPath, opts = {}) {
  // Same operation — WarpGraph.open is idempotent
  return initGraph(repoPath, opts);
}

/**
 * Save (checkpoint) the graph state.
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<string>} checkpoint SHA
 */
export async function saveGraph(graph) {
  return graph.createCheckpoint();
}
