/**
 * @module merge
 * Multi-repo graph merge — import another repo's graph with cross-repo qualification.
 */

import { execFileSync } from 'node:child_process';
import { initGraph } from './graph.js';
import { qualifyNodeId } from './remote.js';

/**
 * Detect the repository identifier from a Git remote URL.
 * Parses the "origin" remote to extract owner/name.
 *
 * @param {string} repoPath - Path to the Git repository
 * @returns {string|null} Repository identifier (owner/name), or null if not detectable
 */
export function detectRepoIdentifier(repoPath) {
  try {
    const url = execFileSync('git', ['remote', 'get-url', 'origin'], {
      cwd: repoPath,
      encoding: 'utf-8',
      stdio: ['pipe', 'pipe', 'pipe'],
    }).trim();

    // SSH format: git@github.com:owner/name.git
    const sshMatch = url.match(/:([^/]+)\/([^/.]+?)(?:\.git)?$/);
    if (sshMatch && url.includes('@')) return `${sshMatch[1]}/${sshMatch[2]}`;

    // HTTPS format: https://github.com/owner/name.git
    const httpsMatch = url.match(/\/([^/]+)\/([^/.]+?)(?:\.git)?$/);
    if (httpsMatch) return `${httpsMatch[1]}/${httpsMatch[2]}`;

    return null;
  } catch {
    return null;
  }
}

/** Edge properties to preserve during merge. */
const MERGE_EDGE_PROPS = new Set(['confidence', 'rationale']);

/**
 * @typedef {object} MergeResult
 * @property {number} nodes - Number of nodes merged
 * @property {number} edges - Number of edges merged
 * @property {string} repoName - Repository identifier used for qualification
 * @property {boolean} dryRun - Whether this was a dry-run
 */

/**
 * Merge a remote repository's graph into the local graph.
 * All remote nodes are qualified with cross-repo IDs.
 *
 * @param {import('@git-stunts/git-warp').default} localGraph
 * @param {string} remoteRepoPath - Path to the remote Git repository
 * @param {{ repoName?: string, dryRun?: boolean }} [opts]
 * @returns {Promise<MergeResult>}
 */
export async function mergeFromRepo(localGraph, remoteRepoPath, opts = {}) {
  const repoName = opts.repoName ?? detectRepoIdentifier(remoteRepoPath);
  if (!repoName) {
    throw new Error('Could not detect repository identifier. Use --repo-name to specify it.');
  }

  const dryRun = opts.dryRun ?? false;

  // Open the remote graph
  const remoteGraph = await initGraph(remoteRepoPath);

  // Read remote nodes and edges
  const remoteNodes = await remoteGraph.getNodes();
  const remoteEdges = await remoteGraph.getEdges();

  if (dryRun) {
    return { nodes: remoteNodes.length, edges: remoteEdges.length, repoName, dryRun: true };
  }

  // Create a single atomic patch
  const patch = await localGraph.createPatch();
  let nodeCount = 0;
  let edgeCount = 0;

  // Qualify and add remote nodes
  for (const nodeId of remoteNodes) {
    const qualifiedId = qualifyNodeId(nodeId, repoName);
    patch.addNode(qualifiedId);
    nodeCount++;

    // Copy node properties
    const propsMap = await remoteGraph.getNodeProps(nodeId);
    if (propsMap) {
      for (const [key, value] of propsMap) {
        patch.setProperty(qualifiedId, key, value);
      }
    }
  }

  // Qualify and add remote edges
  for (const edge of remoteEdges) {
    const qualifiedSource = qualifyNodeId(edge.from, repoName);
    const qualifiedTarget = qualifyNodeId(edge.to, repoName);

    patch.addEdge(qualifiedSource, qualifiedTarget, edge.label);

    // Copy selected edge properties
    if (edge.props) {
      for (const [key, value] of Object.entries(edge.props)) {
        if (MERGE_EDGE_PROPS.has(key) && value !== undefined && value !== null) {
          patch.setEdgeProperty(qualifiedSource, qualifiedTarget, edge.label, key, value);
        }
      }
    }

    edgeCount++;
  }

  // Commit atomically (guard against empty patch)
  if (nodeCount > 0 || edgeCount > 0) {
    await patch.commit();
  }

  return { nodes: nodeCount, edges: edgeCount, repoName, dryRun: false };
}
