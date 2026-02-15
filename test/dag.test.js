import { describe, it, expect } from 'vitest';
import { buildAdjacency, topoSort, detectCycles, walkChain, findRoots } from '../src/dag.js';

describe('dag utilities', () => {
  // ── buildAdjacency ───────────────────────────────────────────

  describe('buildAdjacency', () => {
    it('builds forward and reverse maps from edges', () => {
      const edges = [
        { from: 'a', to: 'b', label: 'depends-on' },
        { from: 'b', to: 'c', label: 'depends-on' },
      ];
      const { forward, reverse } = buildAdjacency(edges);
      expect(forward.get('a')).toEqual(['b']);
      expect(forward.get('b')).toEqual(['c']);
      expect(reverse.get('b')).toEqual(['a']);
      expect(reverse.get('c')).toEqual(['b']);
    });

    it('filters by edge type when specified', () => {
      const edges = [
        { from: 'a', to: 'b', label: 'depends-on' },
        { from: 'c', to: 'd', label: 'blocks' },
      ];
      const { forward } = buildAdjacency(edges, 'blocks');
      expect(forward.has('a')).toBe(false);
      expect(forward.get('c')).toEqual(['d']);
    });

    it('returns empty maps for empty edges', () => {
      const { forward, reverse } = buildAdjacency([]);
      expect(forward.size).toBe(0);
      expect(reverse.size).toBe(0);
    });

    it('handles multiple edges from same source', () => {
      const edges = [
        { from: 'a', to: 'b', label: 'x' },
        { from: 'a', to: 'c', label: 'x' },
      ];
      const { forward } = buildAdjacency(edges);
      expect(forward.get('a')).toEqual(['b', 'c']);
    });
  });

  // ── topoSort ─────────────────────────────────────────────────

  describe('topoSort', () => {
    it('sorts a simple chain', () => {
      const forward = new Map([
        ['a', ['b']],
        ['b', ['c']],
      ]);
      const { sorted, remaining } = topoSort(['a', 'b', 'c'], forward);
      expect(sorted).toEqual(['a', 'b', 'c']);
      expect(remaining).toEqual([]);
    });

    it('handles independent nodes deterministically (alphabetical)', () => {
      const forward = new Map();
      const { sorted } = topoSort(['c', 'a', 'b'], forward);
      expect(sorted).toEqual(['a', 'b', 'c']);
    });

    it('puts cycle members in remaining', () => {
      const forward = new Map([
        ['a', ['b']],
        ['b', ['a']],
      ]);
      const { sorted, remaining } = topoSort(['a', 'b'], forward);
      expect(sorted).toEqual([]);
      expect(remaining).toEqual(['a', 'b']);
    });

    it('handles diamond dependency', () => {
      // a → b, a → c, b → d, c → d
      const forward = new Map([
        ['a', ['b', 'c']],
        ['b', ['d']],
        ['c', ['d']],
      ]);
      const { sorted } = topoSort(['a', 'b', 'c', 'd'], forward);
      expect(sorted.indexOf('a')).toBeLessThan(sorted.indexOf('b'));
      expect(sorted.indexOf('a')).toBeLessThan(sorted.indexOf('c'));
      expect(sorted.indexOf('b')).toBeLessThan(sorted.indexOf('d'));
      expect(sorted.indexOf('c')).toBeLessThan(sorted.indexOf('d'));
    });

    it('handles empty input', () => {
      const { sorted, remaining } = topoSort([], new Map());
      expect(sorted).toEqual([]);
      expect(remaining).toEqual([]);
    });
  });

  // ── detectCycles ─────────────────────────────────────────────

  describe('detectCycles', () => {
    it('returns empty for acyclic graph', () => {
      const forward = new Map([['a', ['b']], ['b', ['c']]]);
      expect(detectCycles(forward)).toEqual([]);
    });

    it('detects a simple 2-node cycle', () => {
      const forward = new Map([['a', ['b']], ['b', ['a']]]);
      const cycles = detectCycles(forward);
      expect(cycles.length).toBeGreaterThan(0);
      // At least one cycle contains both a and b
      const flatCycles = cycles.flat();
      expect(flatCycles).toContain('a');
      expect(flatCycles).toContain('b');
    });

    it('detects a 3-node cycle', () => {
      const forward = new Map([['a', ['b']], ['b', ['c']], ['c', ['a']]]);
      const cycles = detectCycles(forward);
      expect(cycles.length).toBeGreaterThan(0);
    });

    it('returns empty for empty graph', () => {
      expect(detectCycles(new Map())).toEqual([]);
    });
  });

  // ── walkChain ────────────────────────────────────────────────

  describe('walkChain', () => {
    it('walks a linear chain with depth annotations', () => {
      const forward = new Map([['a', ['b']], ['b', ['c']]]);
      const chain = walkChain('a', forward);
      expect(chain).toEqual([
        { node: 'a', depth: 0 },
        { node: 'b', depth: 1 },
        { node: 'c', depth: 2 },
      ]);
    });

    it('handles branching (visits all reachable nodes)', () => {
      const forward = new Map([['a', ['b', 'c']], ['b', ['d']], ['c', ['d']]]);
      const chain = walkChain('a', forward);
      const nodes = chain.map(c => c.node);
      expect(nodes).toContain('a');
      expect(nodes).toContain('b');
      expect(nodes).toContain('c');
      expect(nodes).toContain('d');
    });

    it('does not loop on cycles', () => {
      const forward = new Map([['a', ['b']], ['b', ['a']]]);
      const chain = walkChain('a', forward);
      expect(chain.length).toBe(2);
    });

    it('returns single node for isolated start', () => {
      const chain = walkChain('x', new Map());
      expect(chain).toEqual([{ node: 'x', depth: 0 }]);
    });
  });

  // ── findRoots ────────────────────────────────────────────────

  describe('findRoots', () => {
    it('finds nodes with no incoming edges', () => {
      const forward = new Map([['a', ['b']], ['b', ['c']]]);
      expect(findRoots(forward)).toEqual(['a']);
    });

    it('returns all nodes when none have incoming edges', () => {
      const forward = new Map([['a', []], ['b', []]]);
      const roots = findRoots(forward);
      expect(roots).toContain('a');
      expect(roots).toContain('b');
    });

    it('returns empty when all nodes have incoming edges (cycle)', () => {
      const forward = new Map([['a', ['b']], ['b', ['a']]]);
      expect(findRoots(forward)).toEqual([]);
    });

    it('returns empty for empty graph', () => {
      expect(findRoots(new Map())).toEqual([]);
    });

    it('handles multiple roots in a diamond', () => {
      const forward = new Map([['a', ['c']], ['b', ['c']]]);
      const roots = findRoots(forward);
      expect(roots).toContain('a');
      expect(roots).toContain('b');
      expect(roots).not.toContain('c');
    });
  });
});
