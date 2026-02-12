import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import {
  detectDanglingEdges,
  detectOrphanMilestones,
  detectOrphanNodes,
  detectLowConfidenceEdges,
  runDoctor,
  fixIssues,
} from '../src/doctor.js';

describe('doctor', () => {
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

  // ── detectDanglingEdges ─────────────────────────────────────

  it('returns empty for graph with no dangling edges', () => {
    const nodes = ['task:a', 'spec:b'];
    const edges = [{ from: 'task:a', to: 'spec:b', label: 'implements' }];
    expect(detectDanglingEdges(nodes, edges)).toEqual([]);
  });

  it('detects edges with missing source or target', () => {
    const nodes = ['task:a'];
    const edges = [{ from: 'task:a', to: 'spec:gone', label: 'implements' }];
    const issues = detectDanglingEdges(nodes, edges);

    expect(issues).toHaveLength(1);
    expect(issues[0].type).toBe('dangling-edge');
    expect(issues[0].severity).toBe('error');
    expect(issues[0].affected).toContain('spec:gone');
  });

  // ── detectOrphanMilestones ──────────────────────────────────

  it('detects milestones with no belongs-to children', () => {
    const nodes = ['milestone:v1', 'task:a'];
    const edges = [{ from: 'task:a', to: 'task:a', label: 'relates-to' }];
    const issues = detectOrphanMilestones(nodes, edges);

    expect(issues).toHaveLength(1);
    expect(issues[0].type).toBe('orphan-milestone');
    expect(issues[0].severity).toBe('warning');
  });

  it('does not flag milestones that have children', () => {
    const nodes = ['milestone:v1', 'task:a'];
    const edges = [{ from: 'task:a', to: 'milestone:v1', label: 'belongs-to' }];
    expect(detectOrphanMilestones(nodes, edges)).toEqual([]);
  });

  // ── detectOrphanNodes ───────────────────────────────────────

  it('detects nodes not connected to any edge', () => {
    const nodes = ['task:a', 'task:b', 'task:c'];
    const edges = [{ from: 'task:a', to: 'task:b', label: 'blocks' }];
    const issues = detectOrphanNodes(nodes, edges);

    expect(issues).toHaveLength(1);
    expect(issues[0].type).toBe('orphan-node');
    expect(issues[0].severity).toBe('info');
    expect(issues[0].affected).toEqual(['task:c']);
  });

  it('excludes decision: nodes from orphan detection', () => {
    const nodes = ['task:a', 'task:b', 'decision:123-abc'];
    const edges = [{ from: 'task:a', to: 'task:b', label: 'blocks' }];
    const issues = detectOrphanNodes(nodes, edges);

    expect(issues).toHaveLength(0);
    expect(issues.find(i => i.affected[0] === 'decision:123-abc')).toBeUndefined();
  });

  // ── detectLowConfidenceEdges ────────────────────────────────

  it('detects edges below the confidence threshold', () => {
    const edges = [
      { from: 'task:a', to: 'spec:b', label: 'implements', props: { confidence: 0.2 } },
      { from: 'task:c', to: 'spec:d', label: 'implements', props: { confidence: 0.5 } },
      { from: 'task:e', to: 'spec:f', label: 'implements', props: { confidence: 1.0 } },
    ];
    const issues = detectLowConfidenceEdges(edges, 0.3);

    expect(issues).toHaveLength(1);
    expect(issues[0].type).toBe('low-confidence');
    expect(issues[0].affected).toContain('task:a');
  });

  it('uses default threshold of 0.3', () => {
    const edges = [
      { from: 'task:a', to: 'spec:b', label: 'implements', props: { confidence: 0.29 } },
      { from: 'task:c', to: 'spec:d', label: 'implements', props: { confidence: 0.3 } },
    ];
    const issues = detectLowConfidenceEdges(edges);
    expect(issues).toHaveLength(1);
  });

  // ── runDoctor ───────────────────────────────────────────────

  it('returns clean result for a healthy graph', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const result = await runDoctor(graph);

    expect(result.clean).toBe(true);
    expect(result.issues).toHaveLength(0);
    expect(result.summary).toEqual({ errors: 0, warnings: 0, info: 0 });
  });

  it('aggregates issues from all detectors', async () => {
    // Create a low-confidence edge
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.1 });
    // Add a milestone with a belongs-to child so it's not orphan-node,
    // but then add a second milestone with no children
    await createEdge(graph, { source: 'task:a', target: 'milestone:v1', type: 'belongs-to' });
    const patch = await graph.createPatch();
    patch.addNode('milestone:v2');
    await patch.commit();

    const result = await runDoctor(graph, { threshold: 0.3 });

    expect(result.clean).toBe(false);
    // orphan milestone (v2 has no belongs-to children)
    expect(result.summary.warnings).toBeGreaterThanOrEqual(1);
    // low-confidence edge + orphan node (v2)
    expect(result.summary.info).toBeGreaterThanOrEqual(1);
    const types = result.issues.map(i => i.type);
    expect(types).toContain('orphan-milestone');
    expect(types).toContain('low-confidence');
    expect(types).toContain('orphan-node');
  });

  // ── fixIssues ───────────────────────────────────────────────

  it('removes dangling edges and skips non-fixable issues', async () => {
    // Create a valid edge, then simulate a dangling edge scenario
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });

    const issues = [
      {
        type: 'dangling-edge',
        severity: 'error',
        message: 'test',
        affected: ['task:a', 'spec:b', 'implements'],
        source: 'task:a',
        target: 'spec:b',
        edgeType: 'implements',
      },
      {
        type: 'orphan-node',
        severity: 'info',
        message: 'test orphan',
        affected: ['task:orphan'],
      },
    ];

    const result = await fixIssues(graph, issues);

    expect(result.fixed).toBe(1);
    expect(result.skipped).toBe(1);
    expect(result.details).toHaveLength(2);
    expect(result.details[0]).toMatch(/Removed dangling edge/);
    expect(result.details[1]).toMatch(/Cannot auto-fix/);
  });
});
