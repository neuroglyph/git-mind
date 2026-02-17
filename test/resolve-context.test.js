/**
 * @module test/resolve-context
 * Unit tests for the resolveContext() CLI boundary utility.
 *
 * resolveContext() is tested by mocking loadGraph and getEpochForRef so
 * no real git repo or CRDT graph is required.
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';

// ── Mock graph module before importing commands ──────────────────────────────

vi.mock('../src/graph.js', () => ({
  loadGraph: vi.fn(),
  initGraph: vi.fn(),
}));

vi.mock('../src/epoch.js', () => ({
  getEpochForRef: vi.fn(),
}));

// Import after mocks are set up
const { loadGraph } = await import('../src/graph.js');
const { getEpochForRef } = await import('../src/epoch.js');
const { resolveContext } = await import('../src/cli/commands.js');
const { DEFAULT_CONTEXT, createContext } = await import('../src/context-envelope.js');

// ── Shared fake graph factories ──────────────────────────────────────────────

function makeFakeGraph(overrides = {}) {
  return {
    getNodes: vi.fn().mockResolvedValue([]),
    getEdges: vi.fn().mockResolvedValue([]),
    getNodeProps: vi.fn().mockResolvedValue(null),
    materialize: vi.fn().mockResolvedValue(undefined),
    observer: vi.fn(),
    ...overrides,
  };
}

// ── Tests ────────────────────────────────────────────────────────────────────

describe('resolveContext()', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe('default context (HEAD, no observer)', () => {
    it('returns graph from loadGraph with resolvedTick null', async () => {
      const fakeGraph = makeFakeGraph();
      loadGraph.mockResolvedValue(fakeGraph);

      const { graph, resolvedContext } = await resolveContext('/repo', DEFAULT_CONTEXT);

      expect(loadGraph).toHaveBeenCalledWith('/repo', { writerId: 'ctx-reader' });
      expect(graph).toBe(fakeGraph);
      expect(resolvedContext.asOf).toBe('HEAD');
      expect(resolvedContext.resolvedTick).toBeNull();
      expect(resolvedContext.observer).toBeNull();
      expect(resolvedContext.trustPolicy).toBe('open');
    });

    it('does not call materialize for HEAD', async () => {
      const fakeGraph = makeFakeGraph();
      loadGraph.mockResolvedValue(fakeGraph);

      await resolveContext('/repo', DEFAULT_CONTEXT);

      expect(fakeGraph.materialize).not.toHaveBeenCalled();
    });

    it('does not call observer() for null observer', async () => {
      const fakeGraph = makeFakeGraph();
      loadGraph.mockResolvedValue(fakeGraph);

      await resolveContext('/repo', DEFAULT_CONTEXT);

      expect(fakeGraph.observer).not.toHaveBeenCalled();
    });
  });

  describe('asOf time-travel', () => {
    it('opens resolver + fresh temporal instance when asOf != HEAD', async () => {
      const resolver = makeFakeGraph();
      const temporal = makeFakeGraph();
      // First loadGraph call → resolver; second → temporal
      loadGraph
        .mockResolvedValueOnce(resolver)
        .mockResolvedValueOnce(temporal);

      getEpochForRef.mockResolvedValue({ sha: 'abc123', epoch: { tick: 42, nearest: false } });

      const envelope = createContext({ asOf: 'main~5' });
      const { graph, resolvedContext } = await resolveContext('/repo', envelope);

      expect(loadGraph).toHaveBeenCalledWith('/repo', { writerId: 'ctx-resolver' });
      expect(loadGraph).toHaveBeenCalledWith('/repo', { writerId: 'ctx-temporal' });
      expect(getEpochForRef).toHaveBeenCalledWith(resolver, '/repo', 'main~5');
      expect(temporal.materialize).toHaveBeenCalledWith({ ceiling: 42 });
      expect(graph).toBe(temporal);
      expect(resolvedContext.resolvedTick).toBe(42);
      expect(resolvedContext.asOf).toBe('main~5');
    });

    it('throws when no epoch found for ref', async () => {
      const resolver = makeFakeGraph();
      loadGraph.mockResolvedValue(resolver);
      getEpochForRef.mockResolvedValue(null);

      const envelope = createContext({ asOf: 'does-not-exist' });

      await expect(resolveContext('/repo', envelope)).rejects.toThrow(
        'No epoch marker found for "does-not-exist"',
      );
    });
  });

  describe('observer filtering', () => {
    it('reads observer config from graph and calls graph.observer()', async () => {
      const propsMap = new Map([['match', 'task:*'], ['expose', ['status']]]);
      const fakeView = { getNodes: vi.fn(), getEdges: vi.fn() };
      const fakeGraph = makeFakeGraph({
        getNodeProps: vi.fn().mockResolvedValue(propsMap),
        observer: vi.fn().mockResolvedValue(fakeView),
      });
      loadGraph.mockResolvedValue(fakeGraph);

      const envelope = createContext({ observer: 'security-team' });
      const { graph, resolvedContext } = await resolveContext('/repo', envelope);

      expect(fakeGraph.getNodeProps).toHaveBeenCalledWith('observer:security-team');
      expect(fakeGraph.observer).toHaveBeenCalledWith('security-team', {
        match: 'task:*',
        expose: ['status'],
      });
      expect(graph).toBe(fakeView);
      expect(resolvedContext.observer).toBe('security-team');
    });

    it('omits expose/redact from config when not set', async () => {
      const propsMap = new Map([['match', 'spec:*']]);
      const fakeView = {};
      const fakeGraph = makeFakeGraph({
        getNodeProps: vi.fn().mockResolvedValue(propsMap),
        observer: vi.fn().mockResolvedValue(fakeView),
      });
      loadGraph.mockResolvedValue(fakeGraph);

      const envelope = createContext({ observer: 'minimal' });
      await resolveContext('/repo', envelope);

      expect(fakeGraph.observer).toHaveBeenCalledWith('minimal', { match: 'spec:*' });
    });

    it('throws when observer node does not exist', async () => {
      const fakeGraph = makeFakeGraph({ getNodeProps: vi.fn().mockResolvedValue(null) });
      loadGraph.mockResolvedValue(fakeGraph);

      const envelope = createContext({ observer: 'missing-profile' });

      await expect(resolveContext('/repo', envelope)).rejects.toThrow(
        "Observer 'missing-profile' not found",
      );
    });
  });

  describe('resolvedContext fields', () => {
    it('passes trustPolicy through to resolvedContext', async () => {
      const fakeGraph = makeFakeGraph();
      loadGraph.mockResolvedValue(fakeGraph);

      const envelope = createContext({ trustPolicy: 'approved-only' });
      const { resolvedContext } = await resolveContext('/repo', envelope);

      expect(resolvedContext.trustPolicy).toBe('approved-only');
    });

    it('passes extensionLock through to resolvedContext', async () => {
      const fakeGraph = makeFakeGraph();
      loadGraph.mockResolvedValue(fakeGraph);

      const envelope = createContext({ extensionLock: 'deadbeef' });
      const { resolvedContext } = await resolveContext('/repo', envelope);

      expect(resolvedContext.extensionLock).toBe('deadbeef');
    });
  });
});
