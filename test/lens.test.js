import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { setNodeProperty } from '../src/nodes.js';
import { renderView } from '../src/views.js';
import { defineLens, listLenses, getLens, resetLenses, composeLenses } from '../src/lens.js';

describe('lens engine', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-lens-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    resetLenses();
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── Registry ─────────────────────────────────────────────────

  describe('registry', () => {
    it('listLenses returns built-in lenses', () => {
      const lenses = listLenses();
      expect(lenses).toContain('incomplete');
      expect(lenses).toContain('frontier');
      expect(lenses).toContain('critical-path');
      expect(lenses).toContain('blocked');
      expect(lenses).toContain('parallel');
    });

    it('defineLens registers a custom lens', () => {
      defineLens('custom-test', (vr) => ({ ...vr, meta: { custom: true } }));
      expect(listLenses()).toContain('custom-test');
      expect(getLens('custom-test')).toBeDefined();
    });

    it('defineLens throws on empty name', () => {
      expect(() => defineLens('', () => {})).toThrow('non-empty string');
    });

    it('defineLens throws on non-function filterFn', () => {
      expect(() => defineLens('bad', 'not-a-function')).toThrow('must be a function');
    });

    it('resetLenses removes custom but keeps built-ins', () => {
      defineLens('ephemeral', (vr) => vr);
      expect(listLenses()).toContain('ephemeral');
      resetLenses();
      expect(listLenses()).not.toContain('ephemeral');
      expect(listLenses()).toContain('incomplete');
    });

    it('resetLenses restores overwritten built-in definition', () => {
      const original = getLens('incomplete');
      const customFn = (vr) => ({ ...vr, meta: { lens: 'custom' } });
      defineLens('incomplete', customFn);
      expect(getLens('incomplete').filterFn).toBe(customFn);
      resetLenses();
      const restored = getLens('incomplete');
      expect(restored.filterFn).not.toBe(customFn);
      expect(restored.filterFn).toBe(original.filterFn);
    });
  });

  // ── Composition ──────────────────────────────────────────────

  describe('composeLenses', () => {
    it('empty array returns identity function', () => {
      const { composedFn, needsProperties } = composeLenses([]);
      const input = { nodes: ['a'], edges: [] };
      expect(composedFn(input)).toBe(input);
      expect(needsProperties).toBe(false);
    });

    it('single lens applies that lens', () => {
      const { composedFn } = composeLenses(['blocked']);
      const input = {
        nodes: ['a', 'b'],
        edges: [{ from: 'a', to: 'b', label: 'blocks' }],
      };
      const result = composedFn(input);
      expect(result.nodes).toEqual(['b']);
    });

    it('composes left-to-right', () => {
      defineLens('add-x', (vr) => ({
        nodes: [...vr.nodes, 'x'],
        edges: vr.edges,
        meta: { ...vr.meta, addedX: true },
      }));
      defineLens('filter-x', (vr) => ({
        nodes: vr.nodes.filter(n => n !== 'x'),
        edges: vr.edges,
        meta: { ...vr.meta, filteredX: true },
      }));

      // add-x then filter-x: x is added then removed
      const { composedFn } = composeLenses(['add-x', 'filter-x']);
      const result = composedFn({ nodes: ['a'], edges: [] });
      expect(result.nodes).toEqual(['a']);
      expect(result.meta.addedX).toBe(true);
      expect(result.meta.filteredX).toBe(true);
    });

    it('throws on unknown lens', () => {
      expect(() => composeLenses(['nonexistent'])).toThrow(/Unknown lens.*nonexistent/);
    });

    it('propagates needsProperties if any lens needs it', () => {
      const { needsProperties } = composeLenses(['incomplete']);
      expect(needsProperties).toBe(true);
    });

    it('needsProperties is false when no lens needs it', () => {
      const { needsProperties } = composeLenses(['frontier']);
      expect(needsProperties).toBe(false);
    });

    it('null or undefined returns identity', () => {
      for (const input of [null, undefined]) {
        const { composedFn, needsProperties } = composeLenses(input);
        const vr = { nodes: ['a'], edges: [] };
        expect(composedFn(vr)).toBe(vr);
        expect(needsProperties).toBe(false);
      }
    });
  });

  // ── Core lenses via renderView ───────────────────────────────

  describe('incomplete lens', () => {
    it('filters out done nodes', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');
      await setNodeProperty(graph, 'task:b', 'status', 'todo');

      const result = await renderView(graph, 'backlog', { lenses: ['incomplete'] });
      expect(result.nodes).toContain('task:b');
      expect(result.nodes).not.toContain('task:a');
    });

    it('keeps nodes with no status property', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');
      // task:b has no status

      const result = await renderView(graph, 'backlog', { lenses: ['incomplete'] });
      expect(result.nodes).toContain('task:b');
      expect(result.nodes).not.toContain('task:a');
    });
  });

  describe('frontier lens', () => {
    it('returns leaf nodes with no outgoing edges', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:b', target: 'task:c', type: 'blocks' });

      const result = await renderView(graph, 'backlog', { lenses: ['frontier'] });
      expect(result.nodes).toContain('task:c');
      expect(result.nodes).not.toContain('task:a');
      expect(result.nodes).not.toContain('task:b');
    });

    it('returns all nodes when no edges exist between them', async () => {
      await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements' });
      await createEdge(graph, { source: 'task:b', target: 'spec:y', type: 'implements' });

      // backlog only includes task: nodes, no edges between them
      const result = await renderView(graph, 'backlog', { lenses: ['frontier'] });
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
    });
  });

  describe('critical-path lens', () => {
    it('returns the longest dependency chain', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'depends-on' });
      await createEdge(graph, { source: 'task:b', target: 'task:c', type: 'depends-on' });
      await createEdge(graph, { source: 'task:d', target: 'task:e', type: 'depends-on' });

      const result = await renderView(graph, 'backlog', { lenses: ['critical-path'] });
      // depends-on edges are reversed for execution order (c→b→a, length 3 > e→d, length 2)
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
      expect(result.nodes).toContain('task:c');
    });

    it('selects longest path not all descendants in branching DAG', async () => {
      // a depends-on b and d; b depends-on c → execution order: c→b→a, d→a
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'depends-on' });
      await createEdge(graph, { source: 'task:b', target: 'task:c', type: 'depends-on' });
      await createEdge(graph, { source: 'task:a', target: 'task:d', type: 'depends-on' });

      const result = await renderView(graph, 'backlog', { lenses: ['critical-path'] });
      // Longest path is c→b→a (length 3); side-branch d→a (length 2) is excluded
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
      expect(result.nodes).toContain('task:c');
      expect(result.nodes).not.toContain('task:d');
    });

    it('returns empty for graph with no dependency edges', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });

      const result = await renderView(graph, 'backlog', { lenses: ['critical-path'] });
      expect(result.nodes).toEqual([]);
    });
  });

  describe('blocked lens', () => {
    it('returns only nodes that are blocked', async () => {
      await createEdge(graph, { source: 'task:blocker', target: 'task:victim', type: 'blocks' });
      await createEdge(graph, { source: 'task:free', target: 'task:also-free', type: 'relates-to' });

      const result = await renderView(graph, 'backlog', { lenses: ['blocked'] });
      expect(result.nodes).toContain('task:victim');
      expect(result.nodes).not.toContain('task:blocker');
      expect(result.nodes).not.toContain('task:free');
    });

    it('returns empty when no blocks edges', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'depends-on' });

      const result = await renderView(graph, 'backlog', { lenses: ['blocked'] });
      expect(result.nodes).toEqual([]);
    });
  });

  describe('parallel lens', () => {
    it('returns nodes not involved in any dependency', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'depends-on' });
      await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'relates-to' });

      const result = await renderView(graph, 'backlog', { lenses: ['parallel'] });
      expect(result.nodes).toContain('task:c');
      expect(result.nodes).toContain('task:d');
      expect(result.nodes).not.toContain('task:a');
      expect(result.nodes).not.toContain('task:b');
    });

    it('returns all nodes when no dependency edges exist', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });

      const result = await renderView(graph, 'backlog', { lenses: ['parallel'] });
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
    });
  });

  // ── Composed lenses via renderView ───────────────────────────

  describe('lens composition via renderView', () => {
    it('incomplete:frontier chains two lenses', async () => {
      // Chain: a (done) → b (todo) → c (todo, leaf)
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
      await createEdge(graph, { source: 'task:b', target: 'task:c', type: 'blocks' });
      await setNodeProperty(graph, 'task:a', 'status', 'done');
      await setNodeProperty(graph, 'task:b', 'status', 'todo');
      await setNodeProperty(graph, 'task:c', 'status', 'todo');

      const result = await renderView(graph, 'backlog', { lenses: ['incomplete', 'frontier'] });
      // incomplete removes task:a (done), frontier finds leaf of remaining (b→c, leaf is c)
      expect(result.nodes).toContain('task:c');
      expect(result.nodes).not.toContain('task:a');
      expect(result.nodes).not.toContain('task:b');
    });

    it('renderView without lenses behaves as before', async () => {
      await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });

      const result = await renderView(graph, 'backlog');
      expect(result.nodes).toContain('task:a');
      expect(result.nodes).toContain('task:b');
    });
  });
});
