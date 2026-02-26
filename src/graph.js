/**
 * @module graph
 * Wraps @git-stunts/git-warp graph operations for git-mind.
 */

import WarpGraph, { GitGraphAdapter } from '@git-stunts/git-warp';
import GitPlumbing, { ShellRunnerFactory } from '@git-stunts/plumbing';
import { createHash, createHmac, timingSafeEqual } from 'node:crypto';
import { resolve } from 'node:path';

/** Minimal CryptoPort adapter using node:crypto. */
const crypto = {
  async hash(algo, data) { return createHash(algo).update(data).digest('hex'); },
  async hmac(algo, key, data) { return createHmac(algo, key).update(data).digest(); },
  timingSafeEqual(a, b) { return timingSafeEqual(a, b); },
};

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
    crypto,
  });

  return graph;
}

/**
 * Load an existing git-mind graph from a repository.
 * @deprecated Use initGraph — WarpGraph.open is idempotent (init and load are the same call).
 * @param {string} repoPath - Path to the Git repository
 * @param {{ writerId?: string }} [opts]
 * @returns {Promise<import('@git-stunts/git-warp').default>}
 */
export async function loadGraph(repoPath, opts = {}) {
  return initGraph(repoPath, opts);
}
