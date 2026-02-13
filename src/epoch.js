/**
 * @module epoch
 * Epoch markers for time-travel — correlate git commits to CRDT Lamport ticks.
 *
 * Epoch nodes (`epoch:<sha8>`) are stored in the CRDT graph itself, so they
 * travel with push/pull/merge like any other graph data.
 */

import { execFileSync } from 'node:child_process';

/**
 * @typedef {object} EpochInfo
 * @property {number} tick - Lamport tick at the time of recording
 * @property {string} fullSha - Full commit SHA
 * @property {string} recordedAt - ISO timestamp
 * @property {boolean} [nearest] - True if this epoch was found via ancestor walk
 */

/**
 * Get the current maximum Lamport tick from the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<number>}
 */
export async function getCurrentTick(graph) {
  const { maxTick } = await graph.discoverTicks();
  return maxTick;
}

/**
 * Record an epoch marker correlating a git commit SHA to a Lamport tick.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} commitSha - Full or abbreviated commit SHA
 * @param {number} tick - Lamport tick to record
 * @returns {Promise<void>}
 */
export async function recordEpoch(graph, commitSha, tick) {
  const sha8 = commitSha.slice(0, 8);
  const nodeId = `epoch:${sha8}`;

  const patch = await graph.createPatch();

  if (!(await graph.hasNode(nodeId))) {
    patch.addNode(nodeId);
  }

  patch.setProperty(nodeId, 'tick', tick);
  patch.setProperty(nodeId, 'fullSha', commitSha);
  patch.setProperty(nodeId, 'recordedAt', new Date().toISOString());

  await patch.commit();
}

/**
 * Look up an epoch marker by commit SHA.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} commitSha - Full or abbreviated commit SHA
 * @returns {Promise<EpochInfo|null>}
 */
export async function lookupEpoch(graph, commitSha) {
  const sha8 = commitSha.slice(0, 8);
  const nodeId = `epoch:${sha8}`;

  const exists = await graph.hasNode(nodeId);
  if (!exists) return null;

  const propsMap = await graph.getNodeProps(nodeId);
  if (!propsMap) return null;

  return {
    tick: propsMap.get('tick'),
    fullSha: propsMap.get('fullSha'),
    recordedAt: propsMap.get('recordedAt'),
  };
}

/**
 * Walk up to `maxWalk` ancestor commits looking for an epoch marker.
 * Returns the nearest ancestor's epoch, or null if none found.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} cwd - Repository working directory
 * @param {string} targetSha - Starting commit SHA
 * @param {number} [maxWalk=100] - Maximum ancestors to check
 * @returns {Promise<EpochInfo|null>}
 */
export async function lookupNearestEpoch(graph, cwd, targetSha, maxWalk = 100) {
  let shas;
  try {
    const output = execFileSync(
      'git', ['rev-list', `--max-count=${maxWalk}`, targetSha],
      { cwd, encoding: 'utf-8' },
    ).trim();
    shas = output ? output.split('\n') : [];
  } catch {
    return null;
  }

  // Skip first entry (targetSha itself) — caller should have already checked direct lookup
  for (let i = 1; i < shas.length; i++) {
    const epoch = await lookupEpoch(graph, shas[i]);
    if (epoch) {
      return { ...epoch, nearest: true };
    }
  }

  return null;
}

/**
 * Resolve a git ref to a commit SHA, then find its epoch marker.
 * Falls back to nearest ancestor epoch if no exact match.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} cwd - Repository working directory
 * @param {string} ref - Git ref (branch, tag, SHA, HEAD~N, etc.)
 * @returns {Promise<{sha: string, epoch: EpochInfo}|null>}
 */
export async function getEpochForRef(graph, cwd, ref) {
  let sha;
  try {
    sha = execFileSync('git', ['rev-parse', ref], { cwd, encoding: 'utf-8' }).trim();
  } catch {
    return null;
  }

  // Direct lookup
  const direct = await lookupEpoch(graph, sha);
  if (direct) {
    return { sha, epoch: direct };
  }

  // Ancestor fallback
  const nearest = await lookupNearestEpoch(graph, cwd, sha);
  if (nearest) {
    return { sha, epoch: nearest };
  }

  return null;
}
