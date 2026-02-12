/**
 * @module review
 * Review decisions and provenance for git-mind.
 * Stores decisions as decision: prefixed nodes in the graph.
 */

import { createHash } from 'node:crypto';
import { isLowConfidence } from './validators.js';
import { removeEdge, createEdge } from './edges.js';

/**
 * @typedef {object} PendingSuggestion
 * @property {string} source - Source node ID
 * @property {string} target - Target node ID
 * @property {string} type - Edge label
 * @property {number} confidence
 * @property {string} [rationale]
 * @property {string} [createdAt]
 */

/**
 * @typedef {object} ReviewDecision
 * @property {string} id - Decision node ID
 * @property {'accept'|'reject'|'adjust'|'skip'} action
 * @property {string} source
 * @property {string} target
 * @property {string} edgeType
 * @property {number} confidence
 * @property {string} [rationale]
 * @property {number} timestamp
 * @property {string} [reviewer]
 */

/**
 * Generate a unique decision node ID from edge components.
 *
 * @param {string} source
 * @param {string} target
 * @param {string} type
 * @returns {string}
 */
function makeDecisionId(source, target, type) {
  const hash = createHash('sha256')
    .update(`${source}|${target}|${type}`)
    .digest('hex')
    .slice(0, 8);
  const epoch = Math.floor(Date.now() / 1000);
  return `decision:${epoch}-${hash}`;
}

/**
 * Record a decision node in the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {ReviewDecision} decision
 * @returns {Promise<void>}
 */
async function recordDecision(graph, decision) {
  const patch = await graph.createPatch();
  patch.addNode(decision.id);
  patch.setProperty(decision.id, 'action', decision.action);
  patch.setProperty(decision.id, 'source', decision.source);
  patch.setProperty(decision.id, 'target', decision.target);
  patch.setProperty(decision.id, 'edgeType', decision.edgeType);
  patch.setProperty(decision.id, 'confidence', decision.confidence);
  patch.setProperty(decision.id, 'timestamp', decision.timestamp);
  if (decision.rationale) {
    patch.setProperty(decision.id, 'rationale', decision.rationale);
  }
  if (decision.reviewer) {
    patch.setProperty(decision.id, 'reviewer', decision.reviewer);
  }
  await patch.commit();
}

/**
 * Get pending suggestions: low-confidence edges that haven't been reviewed yet.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<PendingSuggestion[]>}
 */
export async function getPendingSuggestions(graph) {
  const edges = await graph.getEdges();
  const lowConf = edges.filter(isLowConfidence);

  if (lowConf.length === 0) return [];

  // Find reviewed edge keys from decision nodes
  const nodes = await graph.getNodes();
  const decisionNodes = nodes.filter(n => n.startsWith('decision:'));

  const reviewedKeys = new Set();
  const propsResults = await Promise.all(decisionNodes.map(id => graph.getNodeProps(id)));
  for (const propsMap of propsResults) {
    if (!propsMap) continue;
    const source = propsMap.get('source');
    const target = propsMap.get('target');
    const edgeType = propsMap.get('edgeType');
    if (source && target && edgeType) {
      reviewedKeys.add(`${source}|${target}|${edgeType}`);
    }
  }

  return lowConf
    .filter(e => !reviewedKeys.has(`${e.from}|${e.to}|${e.label}`))
    .map(e => ({
      source: e.from,
      target: e.to,
      type: e.label,
      confidence: e.props?.confidence ?? 0,
      rationale: e.props?.rationale,
      createdAt: e.props?.createdAt,
    }));
}

/**
 * Accept a suggestion: promote edge confidence to 1.0, record decision.
 * Assumes single-writer: the edge must still exist when called.
 * If the edge was concurrently deleted, setEdgeProperty will throw.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {PendingSuggestion} suggestion
 * @param {{ reviewer?: string }} [opts={}]
 * @returns {Promise<ReviewDecision>}
 */
export async function acceptSuggestion(graph, suggestion, opts = {}) {
  // Update edge confidence to 1.0 and add reviewedAt
  const patch = await graph.createPatch();
  patch.setEdgeProperty(suggestion.source, suggestion.target, suggestion.type, 'confidence', 1.0);
  patch.setEdgeProperty(suggestion.source, suggestion.target, suggestion.type, 'reviewedAt', new Date().toISOString());
  await patch.commit();

  const decision = {
    id: makeDecisionId(suggestion.source, suggestion.target, suggestion.type),
    action: 'accept',
    source: suggestion.source,
    target: suggestion.target,
    edgeType: suggestion.type,
    confidence: 1.0,
    rationale: suggestion.rationale,
    timestamp: Math.floor(Date.now() / 1000),
    reviewer: opts.reviewer,
  };

  await recordDecision(graph, decision);
  return decision;
}

/**
 * Reject a suggestion: remove the low-confidence edge, record decision.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {PendingSuggestion} suggestion
 * @param {{ reviewer?: string }} [opts={}]
 * @returns {Promise<ReviewDecision>}
 */
export async function rejectSuggestion(graph, suggestion, opts = {}) {
  await removeEdge(graph, suggestion.source, suggestion.target, suggestion.type);

  const decision = {
    id: makeDecisionId(suggestion.source, suggestion.target, suggestion.type),
    action: 'reject',
    source: suggestion.source,
    target: suggestion.target,
    edgeType: suggestion.type,
    confidence: suggestion.confidence,
    rationale: suggestion.rationale,
    timestamp: Math.floor(Date.now() / 1000),
    reviewer: opts.reviewer,
  };

  await recordDecision(graph, decision);
  return decision;
}

/**
 * Adjust a suggestion: update edge props, record decision.
 * Assumes single-writer: the edge must still exist when called.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {PendingSuggestion} original
 * @param {{ type?: string, confidence?: number, rationale?: string, reviewer?: string }} adjustments
 * @returns {Promise<ReviewDecision>}
 */
export async function adjustSuggestion(graph, original, adjustments = {}) {
  const newType = adjustments.type ?? original.type;
  const newConf = adjustments.confidence ?? original.confidence;

  // If type changed, create new edge first, then remove old (safer ordering)
  if (newType !== original.type) {
    await createEdge(graph, {
      source: original.source,
      target: original.target,
      type: newType,
      confidence: newConf,
      rationale: adjustments.rationale ?? original.rationale,
    });
    // Set reviewedAt on the new edge
    const patch = await graph.createPatch();
    patch.setEdgeProperty(original.source, original.target, newType, 'reviewedAt', new Date().toISOString());
    await patch.commit();
    await removeEdge(graph, original.source, original.target, original.type);
  } else {
    // Update existing edge
    const patch = await graph.createPatch();
    patch.setEdgeProperty(original.source, original.target, original.type, 'confidence', newConf);
    if (adjustments.rationale) {
      patch.setEdgeProperty(original.source, original.target, original.type, 'rationale', adjustments.rationale);
    }
    patch.setEdgeProperty(original.source, original.target, original.type, 'reviewedAt', new Date().toISOString());
    await patch.commit();
  }

  const decision = {
    id: makeDecisionId(original.source, original.target, original.type),
    action: 'adjust',
    source: original.source,
    target: original.target,
    edgeType: newType,
    confidence: newConf,
    rationale: adjustments.rationale ?? original.rationale,
    timestamp: Math.floor(Date.now() / 1000),
    reviewer: adjustments.reviewer,
  };

  await recordDecision(graph, decision);
  return decision;
}

/**
 * Skip a suggestion: defers the decision without persisting.
 * Skipped items intentionally remain pending and will reappear in future
 * review sessions, allowing the reviewer to revisit them later.
 *
 * @param {PendingSuggestion} suggestion
 * @returns {ReviewDecision}
 */
export function skipSuggestion(suggestion) {
  return {
    id: makeDecisionId(suggestion.source, suggestion.target, suggestion.type),
    action: 'skip',
    source: suggestion.source,
    target: suggestion.target,
    edgeType: suggestion.type,
    confidence: suggestion.confidence,
    rationale: suggestion.rationale,
    timestamp: Math.floor(Date.now() / 1000),
  };
}

/**
 * Get review history from decision nodes in the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {{ action?: string }} [filter={}]
 * @returns {Promise<ReviewDecision[]>}
 */
export async function getReviewHistory(graph, filter = {}) {
  const nodes = await graph.getNodes();
  const decisionNodes = nodes.filter(n => n.startsWith('decision:'));

  const decisions = [];
  const propsResults = await Promise.all(decisionNodes.map(id => graph.getNodeProps(id)));
  for (let i = 0; i < decisionNodes.length; i++) {
    const propsMap = propsResults[i];
    if (!propsMap) continue;

    const action = propsMap.get('action');
    if (filter.action && action !== filter.action) continue;

    decisions.push({
      id: decisionNodes[i],
      action,
      source: propsMap.get('source'),
      target: propsMap.get('target'),
      edgeType: propsMap.get('edgeType'),
      confidence: propsMap.get('confidence'),
      rationale: propsMap.get('rationale'),
      timestamp: propsMap.get('timestamp'),
      reviewer: propsMap.get('reviewer'),
    });
  }

  return decisions.sort((a, b) => (b.timestamp ?? 0) - (a.timestamp ?? 0));
}

/**
 * Apply a batch decision to all pending suggestions.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {'accept'|'reject'} action
 * @param {{ reviewer?: string }} [opts={}]
 * @returns {Promise<{ processed: number, decisions: ReviewDecision[] }>}
 */
export async function batchDecision(graph, action, opts = {}) {
  if (action !== 'accept' && action !== 'reject') {
    throw new Error(`Invalid batch action: ${action}. Must be "accept" or "reject".`);
  }
  const pending = await getPendingSuggestions(graph);
  const decisions = [];

  for (const suggestion of pending) {
    const decision = action === 'accept'
      ? await acceptSuggestion(graph, suggestion, opts)
      : await rejectSuggestion(graph, suggestion, opts);
    decisions.push(decision);
  }

  return { processed: decisions.length, decisions };
}
