import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge, queryEdges } from '../src/edges.js';
import {
  getPendingSuggestions,
  acceptSuggestion,
  rejectSuggestion,
  adjustSuggestion,
  skipSuggestion,
  getReviewHistory,
  batchDecision,
} from '../src/review.js';

describe('review', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── getPendingSuggestions ───────────────────────────────────

  it('returns low-confidence edges that have not been reviewed', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'implements', confidence: 1.0 });

    const pending = await getPendingSuggestions(graph);

    expect(pending).toHaveLength(1);
    expect(pending[0].source).toBe('task:a');
    expect(pending[0].confidence).toBe(0.3);
  });

  it('excludes edges that already have a decision', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    // Record a decision for this edge
    const patch = await graph.createPatch();
    patch.addNode('decision:test-1');
    patch.setProperty('decision:test-1', 'action', 'accept');
    patch.setProperty('decision:test-1', 'source', 'task:a');
    patch.setProperty('decision:test-1', 'target', 'spec:b');
    patch.setProperty('decision:test-1', 'edgeType', 'implements');
    await patch.commit();

    const pending = await getPendingSuggestions(graph);
    expect(pending).toHaveLength(0);
  });

  // ── acceptSuggestion ───────────────────────────────────────

  it('promotes edge confidence to 1.0 and records decision', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    const suggestion = { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 };
    const decision = await acceptSuggestion(graph, suggestion, { reviewer: 'james' });

    expect(decision.action).toBe('accept');
    expect(decision.confidence).toBe(1.0);
    expect(decision.reviewer).toBe('james');

    // Check edge was updated
    const edges = await queryEdges(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    expect(edges[0].props.confidence).toBe(1.0);
    expect(edges[0].props.reviewedAt).toBeTruthy();

    // Check decision node was recorded
    const history = await getReviewHistory(graph);
    expect(history).toHaveLength(1);
    expect(history[0].action).toBe('accept');
  });

  // ── rejectSuggestion ───────────────────────────────────────

  it('removes edge and records decision', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    const suggestion = { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 };
    const decision = await rejectSuggestion(graph, suggestion);

    expect(decision.action).toBe('reject');

    // Edge should be gone
    const edges = await queryEdges(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    expect(edges).toHaveLength(0);

    // Decision node persists
    const history = await getReviewHistory(graph);
    expect(history).toHaveLength(1);
    expect(history[0].action).toBe('reject');
  });

  // ── adjustSuggestion ───────────────────────────────────────

  it('modifies edge confidence and records decision', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    const original = { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 };
    const decision = await adjustSuggestion(graph, original, { confidence: 0.9, rationale: 'looks good' });

    expect(decision.action).toBe('adjust');
    expect(decision.confidence).toBe(0.9);

    // Check edge was updated
    const edges = await queryEdges(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    expect(edges[0].props.confidence).toBe(0.9);
  });

  // ── skipSuggestion ─────────────────────────────────────────

  it('returns decision without modifying graph', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    const suggestion = { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 };
    const decision = skipSuggestion(suggestion);

    expect(decision.action).toBe('skip');

    // Edge untouched
    const edges = await queryEdges(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    expect(edges[0].props.confidence).toBe(0.3);

    // No decision node in graph (skip doesn't write)
    const history = await getReviewHistory(graph);
    expect(history).toHaveLength(0);
  });

  // ── getReviewHistory ───────────────────────────────────────

  it('filters history by action', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'implements', confidence: 0.2 });

    await acceptSuggestion(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });
    await rejectSuggestion(graph, { source: 'task:c', target: 'spec:d', type: 'implements', confidence: 0.2 });

    const accepts = await getReviewHistory(graph, { action: 'accept' });
    expect(accepts).toHaveLength(1);
    expect(accepts[0].action).toBe('accept');

    const rejects = await getReviewHistory(graph, { action: 'reject' });
    expect(rejects).toHaveLength(1);
    expect(rejects[0].action).toBe('reject');
  });

  // ── batchDecision ──────────────────────────────────────────

  it('batch reject processes all pending', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'implements', confidence: 0.2 });
    await createEdge(graph, { source: 'task:e', target: 'spec:f', type: 'implements', confidence: 1.0 }); // not pending

    const result = await batchDecision(graph, 'reject');

    expect(result.processed).toBe(2);
    expect(result.decisions).toHaveLength(2);
    expect(result.decisions.every(d => d.action === 'reject')).toBe(true);
  });

  it('batch accept promotes all pending', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 });

    const result = await batchDecision(graph, 'accept');

    expect(result.processed).toBe(1);
    expect(result.decisions[0].action).toBe('accept');

    // Confirm edge was promoted
    const edges = await queryEdges(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    expect(edges[0].props.confidence).toBe(1.0);
  });
});
