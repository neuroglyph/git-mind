/**
 * @module doctor
 * Graph integrity detectors for git-mind.
 * Composable checks that identify structural issues in the knowledge graph.
 */

import { SYSTEM_PREFIXES, extractPrefix } from './validators.js';
import { removeEdge } from './edges.js';

/** Prefixes excluded from orphan-node detection (system-generated + review decisions). */
const EXCLUDED_ORPHAN_PREFIXES = new Set([...SYSTEM_PREFIXES, 'decision']);

/**
 * @typedef {object} DoctorIssue
 * @property {string} type - Issue type identifier
 * @property {'error'|'warning'|'info'} severity
 * @property {string} message - Human-readable description
 * @property {string[]} affected - IDs of affected nodes/edges
 * @property {string} [source] - Source node ID (set by dangling-edge)
 * @property {string} [target] - Target node ID (set by dangling-edge)
 * @property {string} [edgeType] - Edge type (set by dangling-edge)
 */

/**
 * @typedef {object} DoctorResult
 * @property {DoctorIssue[]} issues
 * @property {{ errors: number, warnings: number, info: number }} summary
 * @property {boolean} clean
 */

/**
 * Detect edges whose source or target node does not exist in the graph.
 *
 * @param {string[]} nodes
 * @param {Array<{from: string, to: string, label: string}>} edges
 * @returns {DoctorIssue[]}
 */
export function detectDanglingEdges(nodes, edges) {
  const nodeSet = new Set(nodes);
  const issues = [];

  for (const edge of edges) {
    const missing = [];
    if (!nodeSet.has(edge.from)) missing.push(edge.from);
    if (!nodeSet.has(edge.to)) missing.push(edge.to);

    if (missing.length > 0) {
      issues.push({
        type: 'dangling-edge',
        severity: 'error',
        message: `Edge ${edge.from} --[${edge.label}]--> ${edge.to} references missing node(s): ${missing.join(', ')}`,
        affected: [edge.from, edge.to, edge.label],
        source: edge.from,
        target: edge.to,
        edgeType: edge.label,
      });
    }
  }

  return issues;
}

/**
 * Detect milestone nodes with no belongs-to edges pointing at them.
 *
 * @param {string[]} nodes
 * @param {Array<{from: string, to: string, label: string}>} edges
 * @returns {DoctorIssue[]}
 */
export function detectOrphanMilestones(nodes, edges) {
  const milestones = nodes.filter(n => n.startsWith('milestone:'));
  if (milestones.length === 0) return [];

  const belongsToTargets = new Set(
    edges.filter(e => e.label === 'belongs-to').map(e => e.to)
  );

  return milestones
    .filter(m => !belongsToTargets.has(m))
    .map(m => ({
      type: 'orphan-milestone',
      severity: 'warning',
      message: `Milestone ${m} has no children (no belongs-to edges target it)`,
      affected: [m],
    }));
}

/**
 * Detect nodes that are not referenced by any edge.
 *
 * @param {string[]} nodes
 * @param {Array<{from: string, to: string, label: string}>} edges
 * @returns {DoctorIssue[]}
 */
export function detectOrphanNodes(nodes, edges) {
  const connected = new Set();
  for (const e of edges) {
    connected.add(e.from);
    connected.add(e.to);
  }

  return nodes
    .filter(n => !connected.has(n) && !EXCLUDED_ORPHAN_PREFIXES.has(extractPrefix(n)))
    .map(n => ({
      type: 'orphan-node',
      severity: 'info',
      message: `Node ${n} is not connected to any edge`,
      affected: [n],
    }));
}

/**
 * Detect edges with confidence below the given threshold.
 *
 * @param {Array<{from: string, to: string, label: string, props?: object}>} edges
 * @param {number} [threshold=0.3]
 * @returns {DoctorIssue[]}
 */
export function detectLowConfidenceEdges(edges, threshold = 0.3) {
  return edges
    .filter(e => {
      const c = e.props?.confidence;
      return typeof c === 'number' && c < threshold;
    })
    .map(e => ({
      type: 'low-confidence',
      severity: 'info',
      message: `Edge ${e.from} --[${e.label}]--> ${e.to} has low confidence (${e.props.confidence})`,
      affected: [e.from, e.to, e.label],
    }));
}

/**
 * Run all doctor detectors against the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {{ threshold?: number }} [opts={}]
 * @returns {Promise<DoctorResult>}
 */
export async function runDoctor(graph, opts = {}) {
  const nodes = await graph.getNodes();
  const edges = await graph.getEdges();

  const issues = [
    ...detectDanglingEdges(nodes, edges),
    ...detectOrphanMilestones(nodes, edges),
    ...detectOrphanNodes(nodes, edges),
    ...detectLowConfidenceEdges(edges, opts.threshold),
  ];

  const summary = { errors: 0, warnings: 0, info: 0 };
  for (const issue of issues) {
    if (issue.severity === 'error') summary.errors++;
    else if (issue.severity === 'warning') summary.warnings++;
    else if (issue.severity === 'info') summary.info++;
  }

  return { issues, summary, clean: issues.length === 0 };
}

/**
 * Fix auto-fixable issues. Currently supports removing dangling edges.
 * Note: orphan node removal is not supported by git-warp (nodes cannot be deleted).
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {DoctorIssue[]} issues
 * @returns {Promise<{ fixed: number, skipped: number, details: string[] }>}
 */
export async function fixIssues(graph, issues) {
  let fixed = 0;
  let skipped = 0;
  const details = [];

  for (const issue of issues) {
    if (issue.type === 'dangling-edge') {
      const { source, target, edgeType } = issue;
      try {
        await removeEdge(graph, source, target, edgeType);
        fixed++;
        details.push(`Removed dangling edge: ${source} --[${edgeType}]--> ${target}`);
      } catch (err) {
        skipped++;
        details.push(`Failed to remove edge ${source} --[${edgeType}]--> ${target}: ${err.message}`);
      }
    } else {
      skipped++;
      details.push(`Cannot auto-fix ${issue.type}: ${issue.message}`);
    }
  }

  return { fixed, skipped, details };
}
