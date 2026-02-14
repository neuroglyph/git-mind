import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { renderView, listViews, defineView, declareView, resetViews, classifyStatus } from '../src/views.js';
import { setNodeProperty } from '../src/nodes.js';

describe('views', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    resetViews();
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── Core engine ───────────────────────────────────────────────

  it('listViews returns all built-in views', () => {
    const views = listViews();
    expect(views).toContain('roadmap');
    expect(views).toContain('architecture');
    expect(views).toContain('backlog');
    expect(views).toContain('suggestions');
    expect(views).toContain('milestone');
    expect(views).toContain('traceability');
    expect(views).toContain('blockers');
    expect(views).toContain('onboarding');
    expect(views).toContain('coverage');
    expect(views).toContain('progress');
  });

  it('renderView throws for unknown views', async () => {
    await expect(renderView(graph, 'nonexistent')).rejects.toThrow(/Unknown view/);
  });

  it('defineView registers a custom imperative view', async () => {
    defineView('custom-test', (nodes, edges) => ({
      nodes: nodes.filter(n => n.startsWith('x:')),
      edges: [],
    }));

    await createEdge(graph, { source: 'x:foo', target: 'y:bar', type: 'relates-to' });

    const result = await renderView(graph, 'custom-test');
    expect(result.nodes).toEqual(['x:foo']);
    expect(result.edges).toEqual([]);
  });

  it('declareView throws on missing or empty prefixes', () => {
    expect(() => declareView('bad-view', {})).toThrow('prefixes must be a non-empty array');
    expect(() => declareView('bad-view', { prefixes: [] })).toThrow('prefixes must be a non-empty array');
  });

  it('declareView registers a config-based view', async () => {
    declareView('declared-test', {
      prefixes: ['pkg'],
      edgeTypes: ['depends-on'],
      requireBothEndpoints: true,
    });

    await createEdge(graph, { source: 'pkg:a', target: 'pkg:b', type: 'depends-on' });
    await createEdge(graph, { source: 'pkg:a', target: 'task:c', type: 'implements' });

    const result = await renderView(graph, 'declared-test');
    expect(result.nodes).toContain('pkg:a');
    expect(result.nodes).toContain('pkg:b');
    expect(result.edges.length).toBe(1);
    expect(result.edges[0].label).toBe('depends-on');
  });

  // ── Existing views (now declarative) ──────────────────────────

  it('roadmap view filters for phase/task nodes', async () => {
    await createEdge(graph, { source: 'phase:alpha', target: 'task:build-cli', type: 'blocks' });
    await createEdge(graph, { source: 'file:src/main.js', target: 'doc:readme', type: 'documents' });

    const result = await renderView(graph, 'roadmap');
    expect(result.nodes).toContain('phase:alpha');
    expect(result.nodes).toContain('task:build-cli');
    expect(result.nodes).not.toContain('file:src/main.js');
  });

  it('architecture view filters for module nodes and depends-on edges', async () => {
    await createEdge(graph, { source: 'module:auth', target: 'module:db', type: 'depends-on' });
    await createEdge(graph, { source: 'task:fix', target: 'task:deploy', type: 'blocks' });

    const result = await renderView(graph, 'architecture');
    expect(result.nodes).toContain('module:auth');
    expect(result.nodes).toContain('module:db');
    expect(result.edges.length).toBe(1);
    expect(result.edges[0].label).toBe('depends-on');
  });

  it('backlog view filters for task nodes', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
    await createEdge(graph, { source: 'module:x', target: 'module:y', type: 'depends-on' });

    const result = await renderView(graph, 'backlog');
    expect(result.nodes).toContain('task:a');
    expect(result.nodes).toContain('task:b');
    expect(result.nodes).not.toContain('module:x');
  });

  it('suggestions view filters for low-confidence edges', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to', confidence: 0.3 });
    await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'implements', confidence: 1.0 });

    const result = await renderView(graph, 'suggestions');
    expect(result.edges.length).toBe(1);
    expect(result.edges[0].from).toBe('task:a');
    expect(result.nodes).toContain('task:a');
    expect(result.nodes).toContain('task:b');
    expect(result.nodes).not.toContain('task:c');
  });

  // ── PRISM: milestone view ─────────────────────────────────────

  describe('milestone view', () => {
    it('shows milestones with completion stats', async () => {
      await createEdge(graph, { source: 'task:a', target: 'milestone:M1', type: 'belongs-to' });
      await createEdge(graph, { source: 'task:b', target: 'milestone:M1', type: 'belongs-to' });
      // Completion heuristic: a child is "done" if it has an outgoing 'implements' edge
      await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements' });

      const result = await renderView(graph, 'milestone');
      expect(result.nodes).toContain('milestone:M1');
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
      expect(result.meta.milestoneStats['milestone:M1'].total).toBe(2);
      expect(result.meta.milestoneStats['milestone:M1'].done).toBe(1);
      expect(result.meta.milestoneStats['milestone:M1'].pct).toBe(50);
    });

    it('reports blockers for a milestone', async () => {
      await createEdge(graph, { source: 'task:a', target: 'milestone:M1', type: 'belongs-to' });
      await createEdge(graph, { source: 'task:blocker', target: 'task:a', type: 'blocks' });

      const result = await renderView(graph, 'milestone');
      expect(result.meta.milestoneStats['milestone:M1'].blockers).toContain('task:blocker');
    });

    it('includes features in milestone stats', async () => {
      await createEdge(graph, { source: 'task:a', target: 'milestone:M1', type: 'belongs-to' });
      await createEdge(graph, { source: 'feature:login', target: 'milestone:M1', type: 'belongs-to' });
      await createEdge(graph, { source: 'feature:login', target: 'spec:auth', type: 'implements' });

      const result = await renderView(graph, 'milestone');
      expect(result.meta.milestoneStats['milestone:M1'].total).toBe(2);
      expect(result.meta.milestoneStats['milestone:M1'].done).toBe(1);
    });

    it('excludes non-task/non-feature children from milestone stats', async () => {
      await createEdge(graph, { source: 'task:a', target: 'milestone:M1', type: 'belongs-to' });
      await createEdge(graph, { source: 'spec:loose', target: 'milestone:M1', type: 'belongs-to' });

      const result = await renderView(graph, 'milestone');
      // spec:loose should not count as a milestone child
      expect(result.meta.milestoneStats['milestone:M1'].total).toBe(1);
    });

    it('returns edges as a self-contained subgraph', async () => {
      await createEdge(graph, { source: 'task:a', target: 'milestone:M1', type: 'belongs-to' });
      // implements edge targets spec:x which is NOT a milestone/task/feature
      await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements' });

      const result = await renderView(graph, 'milestone');
      const nodeSet = new Set(result.nodes);
      for (const e of result.edges) {
        expect(nodeSet.has(e.from)).toBe(true);
        expect(nodeSet.has(e.to)).toBe(true);
      }
    });

    it('handles milestone with no tasks', async () => {
      // Create a milestone node by linking it to something
      await createEdge(graph, { source: 'milestone:empty', target: 'spec:x', type: 'relates-to' });

      const result = await renderView(graph, 'milestone');
      expect(result.meta.milestoneStats['milestone:empty'].total).toBe(0);
      expect(result.meta.milestoneStats['milestone:empty'].pct).toBe(0);
      expect(result.meta.milestoneStats['milestone:empty'].blockers).toEqual([]);
    });
  });

  // ── PRISM: traceability view ──────────────────────────────────

  describe('traceability view', () => {
    it('identifies unimplemented specs as gaps', async () => {
      await createEdge(graph, { source: 'spec:auth', target: 'doc:readme', type: 'documents' });
      await createEdge(graph, { source: 'spec:session', target: 'doc:readme', type: 'documents' });
      await createEdge(graph, { source: 'file:auth.js', target: 'spec:auth', type: 'implements' });

      const result = await renderView(graph, 'traceability');
      expect(result.meta.gaps).toContain('spec:session');
      expect(result.meta.gaps).not.toContain('spec:auth');
      expect(result.meta.covered).toContain('spec:auth');
      expect(result.meta.coveragePct).toBe(50);
    });

    it('reports 100% when all specs are implemented', async () => {
      await createEdge(graph, { source: 'file:a.js', target: 'spec:auth', type: 'implements' });

      const result = await renderView(graph, 'traceability');
      expect(result.meta.gaps).toEqual([]);
      expect(result.meta.coveragePct).toBe(100);
    });

    it('includes ADRs in traceability', async () => {
      await createEdge(graph, { source: 'adr:001', target: 'doc:readme', type: 'documents' });

      const result = await renderView(graph, 'traceability');
      expect(result.meta.gaps).toContain('adr:001');
    });

    it('reports 100% coverage when no specs or ADRs exist', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });

      const result = await renderView(graph, 'traceability');
      expect(result.meta.gaps).toEqual([]);
      expect(result.meta.coveragePct).toBe(100);
    });
  });

  // ── PRISM: blockers view ──────────────────────────────────────

  describe('blockers view', () => {
    it('follows transitive blocking chains', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:b', target: 'task:c', type: 'blocks' });

      const result = await renderView(graph, 'blockers');
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
      expect(result.nodes).toContain('task:c');
      expect(result.meta.rootBlockers).toContain('task:a');
      expect(result.meta.rootBlockers).not.toContain('task:b');
      expect(result.meta.chains.length).toBe(1);
      expect(result.meta.chains[0].length).toBe(3);
    });

    it('detects cycles', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:b', target: 'task:a', type: 'blocks' });

      const result = await renderView(graph, 'blockers');
      expect(result.meta.cycles.length).toBeGreaterThan(0);
    });

    it('handles a root blocker leading into a cycle', async () => {
      await createEdge(graph, { source: 'task:root', target: 'task:a', type: 'blocks' });
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:b', target: 'task:a', type: 'blocks' });

      const result = await renderView(graph, 'blockers');
      expect(result.meta.rootBlockers).toContain('task:root');
      expect(result.meta.cycles.length).toBeGreaterThan(0);
    });

    it('returns empty for graph with no blocks edges', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });

      const result = await renderView(graph, 'blockers');
      expect(result.nodes).toEqual([]);
      expect(result.edges).toEqual([]);
      expect(result.meta.chains).toEqual([]);
      expect(result.meta.cycles).toEqual([]);
    });
  });

  // ── PRISM: onboarding view ────────────────────────────────────

  describe('onboarding view', () => {
    it('returns topologically sorted reading order for a 3-node chain', async () => {
      // spec:a -> spec:b -> spec:c (c depends on b, b depends on a)
      await createEdge(graph, { source: 'spec:b', target: 'spec:a', type: 'depends-on' });
      await createEdge(graph, { source: 'spec:c', target: 'spec:b', type: 'depends-on' });

      const result = await renderView(graph, 'onboarding');
      const order = result.meta.readingOrder;
      expect(order.indexOf('spec:a')).toBeLessThan(order.indexOf('spec:b'));
      expect(order.indexOf('spec:b')).toBeLessThan(order.indexOf('spec:c'));
    });

    it('includes doc and adr nodes', async () => {
      await createEdge(graph, { source: 'doc:guide', target: 'adr:001', type: 'documents' });

      const result = await renderView(graph, 'onboarding');
      expect(result.nodes).toContain('doc:guide');
      expect(result.nodes).toContain('adr:001');
    });

    it('handles graphs with no doc nodes', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });

      const result = await renderView(graph, 'onboarding');
      expect(result.nodes).toEqual([]);
      expect(result.meta.readingOrder).toEqual([]);
      expect(result.meta.hasCycles).toBe(false);
    });

    it('returns edges as a self-contained subgraph', async () => {
      await createEdge(graph, { source: 'spec:a', target: 'spec:b', type: 'depends-on' });
      // implements edge from a non-doc node into a doc node
      await createEdge(graph, { source: 'file:auth.js', target: 'spec:a', type: 'implements' });

      const result = await renderView(graph, 'onboarding');
      const nodeSet = new Set(result.nodes);
      for (const e of result.edges) {
        expect(nodeSet.has(e.from)).toBe(true);
        expect(nodeSet.has(e.to)).toBe(true);
      }
    });

    it('detects cycles in doc dependencies', async () => {
      await createEdge(graph, { source: 'spec:a', target: 'spec:b', type: 'depends-on' });
      await createEdge(graph, { source: 'spec:b', target: 'spec:a', type: 'depends-on' });

      const result = await renderView(graph, 'onboarding');
      expect(result.meta.hasCycles).toBe(true);
      // Both nodes should still appear
      expect(result.nodes).toContain('spec:a');
      expect(result.nodes).toContain('spec:b');
    });
  });

  // ── PROVING GROUND: coverage view ────────────────────────────

  describe('coverage view', () => {
    it('identifies crates not linked to any spec/ADR', async () => {
      await createEdge(graph, { source: 'crate:linked', target: 'spec:auth', type: 'implements' });
      await createEdge(graph, { source: 'crate:orphan', target: 'crate:linked', type: 'depends-on' });

      const result = await renderView(graph, 'coverage');
      expect(result.meta.linked).toContain('crate:linked');
      expect(result.meta.unlinked).toContain('crate:orphan');
      expect(result.meta.coveragePct).toBe(50);
    });

    it('reports 100% when all crates implement specs', async () => {
      await createEdge(graph, { source: 'crate:a', target: 'spec:x', type: 'implements' });
      await createEdge(graph, { source: 'crate:b', target: 'adr:001', type: 'implements' });

      const result = await renderView(graph, 'coverage');
      expect(result.meta.unlinked).toEqual([]);
      expect(result.meta.coveragePct).toBe(100);
    });

    it('handles graph with no crate/module/pkg nodes', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });

      const result = await renderView(graph, 'coverage');
      expect(result.meta.linked).toEqual([]);
      expect(result.meta.unlinked).toEqual([]);
      expect(result.meta.coveragePct).toBe(100);
    });

    it('includes module and pkg nodes', async () => {
      await createEdge(graph, { source: 'module:auth', target: 'spec:auth', type: 'implements' });
      await createEdge(graph, { source: 'pkg:utils', target: 'task:a', type: 'relates-to' });

      const result = await renderView(graph, 'coverage');
      expect(result.meta.linked).toContain('module:auth');
      expect(result.meta.unlinked).toContain('pkg:utils');
    });
  });

  // ── classifyStatus ──────────────────────────────────────────────

  describe('classifyStatus', () => {
    it('normalizes "Done" to "done"', () => {
      expect(classifyStatus('Done')).toBe('done');
    });

    it('normalizes "DONE" to "done"', () => {
      expect(classifyStatus('DONE')).toBe('done');
    });

    it('maps "in_progress" to "in-progress"', () => {
      expect(classifyStatus('in_progress')).toBe('in-progress');
    });

    it('maps "WIP" to "in-progress"', () => {
      expect(classifyStatus('WIP')).toBe('in-progress');
    });

    it('maps "inprogress" to "in-progress"', () => {
      expect(classifyStatus('inprogress')).toBe('in-progress');
    });

    it('maps "complete" to "done"', () => {
      expect(classifyStatus('complete')).toBe('done');
    });

    it('maps "completed" to "done"', () => {
      expect(classifyStatus('completed')).toBe('done');
    });

    it('maps "finished" to "done"', () => {
      expect(classifyStatus('finished')).toBe('done');
    });

    it('trims whitespace: " done " → "done"', () => {
      expect(classifyStatus(' done ')).toBe('done');
    });

    it('returns "unknown" for non-string values', () => {
      expect(classifyStatus(42)).toBe('unknown');
      expect(classifyStatus(null)).toBe('unknown');
      expect(classifyStatus(undefined)).toBe('unknown');
    });

    it('returns "unknown" for unrecognized strings', () => {
      expect(classifyStatus('banana')).toBe('unknown');
    });

    it('passes through valid statuses', () => {
      expect(classifyStatus('done')).toBe('done');
      expect(classifyStatus('in-progress')).toBe('in-progress');
      expect(classifyStatus('todo')).toBe('todo');
      expect(classifyStatus('blocked')).toBe('blocked');
    });
  });

  // ── progress view ──────────────────────────────────────────────

  describe('progress view', () => {
    it('groups nodes by status property', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');
      await setNodeProperty(graph, 'task:b', 'status', 'in-progress');
      await setNodeProperty(graph, 'task:c', 'status', 'todo');
      // task:d has no status

      const result = await renderView(graph, 'progress');
      expect(result.meta.byStatus['done']).toContain('task:a');
      expect(result.meta.byStatus['in-progress']).toContain('task:b');
      expect(result.meta.byStatus['todo']).toContain('task:c');
      expect(result.meta.byStatus['unknown']).toContain('task:d');
    });

    it('computes correct percentage', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');

      const result = await renderView(graph, 'progress');
      expect(result.meta.summary.total).toBe(2);
      expect(result.meta.summary.done).toBe(1);
      expect(result.meta.summary.pct).toBe(50);
    });

    it('returns pct: 0 for empty graph', async () => {
      const result = await renderView(graph, 'progress');
      expect(result.meta.summary.total).toBe(0);
      expect(result.meta.summary.pct).toBe(0);
    });

    it('includes feature: nodes alongside task: nodes', async () => {
      await createEdge(graph, { source: 'feature:login', target: 'task:auth', type: 'relates-to' });
      await setNodeProperty(graph, 'feature:login', 'status', 'done');

      const result = await renderView(graph, 'progress');
      expect(result.nodes).toContain('feature:login');
      expect(result.nodes).toContain('task:auth');
      expect(result.meta.byStatus['done']).toContain('feature:login');
    });

    it('normalizes status synonyms', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });
      await setNodeProperty(graph, 'task:a', 'status', 'WIP');
      await setNodeProperty(graph, 'task:b', 'status', 'Completed');

      const result = await renderView(graph, 'progress');
      expect(result.meta.byStatus['in-progress']).toContain('task:a');
      expect(result.meta.byStatus['done']).toContain('task:b');
    });

    it('excludes non-task/non-feature nodes', async () => {
      await createEdge(graph, { source: 'spec:auth', target: 'task:a', type: 'implements' });
      await setNodeProperty(graph, 'spec:auth', 'status', 'done');

      const result = await renderView(graph, 'progress');
      expect(result.nodes).not.toContain('spec:auth');
      expect(result.nodes).toContain('task:a');
    });

    it('sorts IDs alphabetically within each status bucket', async () => {
      await createEdge(graph, { source: 'task:zebra', target: 'task:alpha', type: 'relates-to' });
      await createEdge(graph, { source: 'task:mango', target: 'task:banana', type: 'relates-to' });
      await setNodeProperty(graph, 'task:zebra', 'status', 'done');
      await setNodeProperty(graph, 'task:alpha', 'status', 'done');
      await setNodeProperty(graph, 'task:mango', 'status', 'todo');
      await setNodeProperty(graph, 'task:banana', 'status', 'todo');

      const result = await renderView(graph, 'progress');
      expect(result.meta.byStatus['done']).toEqual(['task:alpha', 'task:zebra']);
      expect(result.meta.byStatus['todo']).toEqual(['task:banana', 'task:mango']);
    });

    it('includes ratio and remaining in summary', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');

      const result = await renderView(graph, 'progress');
      expect(result.meta.summary.ratio).toBe('1/4');
      expect(result.meta.summary.remaining).toBe(3);
    });

    it('ratio and remaining are correct for empty graph', async () => {
      const result = await renderView(graph, 'progress');
      expect(result.meta.summary.ratio).toBe('0/0');
      expect(result.meta.summary.remaining).toBe(0);
    });

    it('progress view with scope=[task] excludes feature: nodes', async () => {
      await createEdge(graph, { source: 'feature:login', target: 'task:auth', type: 'relates-to' });
      await setNodeProperty(graph, 'feature:login', 'status', 'done');
      await setNodeProperty(graph, 'task:auth', 'status', 'done');

      const result = await renderView(graph, 'progress', { scope: ['task'] });
      expect(result.nodes).toContain('task:auth');
      expect(result.nodes).not.toContain('feature:login');
      expect(result.meta.summary.total).toBe(1);
    });

    it('progress view with default scope includes both task and feature', async () => {
      await createEdge(graph, { source: 'feature:login', target: 'task:auth', type: 'relates-to' });

      const result = await renderView(graph, 'progress');
      expect(result.nodes).toContain('feature:login');
      expect(result.nodes).toContain('task:auth');
    });

    it('renderView forwards options to view filterFn', async () => {
      await createEdge(graph, { source: 'feature:x', target: 'task:y', type: 'relates-to' });
      await setNodeProperty(graph, 'feature:x', 'status', 'todo');
      await setNodeProperty(graph, 'task:y', 'status', 'todo');

      const scoped = await renderView(graph, 'progress', { scope: ['feature'] });
      expect(scoped.nodes).toEqual(['feature:x']);
      expect(scoped.meta.summary.total).toBe(1);

      const unscoped = await renderView(graph, 'progress');
      expect(unscoped.meta.summary.total).toBe(2);
    });
  });
});
