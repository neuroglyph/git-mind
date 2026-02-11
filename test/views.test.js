import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { renderView, listViews, defineView, declareView, resetViews } from '../src/views.js';

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
});
