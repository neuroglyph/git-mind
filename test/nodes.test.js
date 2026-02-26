import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { getNode, getNodesByPrefix, setNodeProperty, unsetNodeProperty } from '../src/nodes.js';

describe('nodes', () => {
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

  describe('graph.getNodes (native API)', () => {
    it('returns empty array for a fresh graph', async () => {
      const nodes = await graph.getNodes();
      expect(nodes).toEqual([]);
    });

    it('returns nodes created by edges', async () => {
      await createEdge(graph, {
        source: 'file:src/auth.js',
        target: 'spec:auth',
        type: 'implements',
      });

      const nodes = await graph.getNodes();
      expect(nodes).toContain('file:src/auth.js');
      expect(nodes).toContain('spec:auth');
      expect(nodes.length).toBe(2);
    });

    it('returns nodes from multiple edges without duplicates', async () => {
      await createEdge(graph, {
        source: 'file:a.js',
        target: 'spec:auth',
        type: 'implements',
      });
      await createEdge(graph, {
        source: 'file:a.js',
        target: 'doc:readme',
        type: 'documents',
      });

      const nodes = await graph.getNodes();
      expect(nodes).toContain('file:a.js');
      expect(nodes).toContain('spec:auth');
      expect(nodes).toContain('doc:readme');
      expect(nodes.length).toBe(3);
    });
  });

  describe('graph.hasNode (native API)', () => {
    it('returns false for non-existent node', async () => {
      expect(await graph.hasNode('task:nonexistent')).toBe(false);
    });

    it('returns true for node created via edge', async () => {
      await createEdge(graph, {
        source: 'task:auth',
        target: 'spec:auth',
        type: 'implements',
      });

      expect(await graph.hasNode('task:auth')).toBe(true);
      expect(await graph.hasNode('spec:auth')).toBe(true);
    });
  });

  describe('getNode', () => {
    it('returns null for non-existent node', async () => {
      const node = await getNode(graph, 'task:nonexistent');
      expect(node).toBeNull();
    });

    it('returns node info for existing node', async () => {
      await createEdge(graph, {
        source: 'file:src/auth.js',
        target: 'spec:auth',
        type: 'implements',
      });

      const node = await getNode(graph, 'file:src/auth.js');
      expect(node).not.toBeNull();
      expect(node.id).toBe('file:src/auth.js');
      expect(node.prefix).toBe('file');
      expect(node.prefixClass).toBe('canonical');
      expect(node.properties).toBeDefined();
    });

    it('classifies system prefix correctly', async () => {
      await createEdge(graph, {
        source: 'commit:abc123',
        target: 'spec:auth',
        type: 'implements',
      });

      const node = await getNode(graph, 'commit:abc123');
      expect(node.prefix).toBe('commit');
      expect(node.prefixClass).toBe('system');
    });

    it('classifies unknown prefix correctly', async () => {
      await createEdge(graph, {
        source: 'custom:thing',
        target: 'spec:auth',
        type: 'implements',
      });

      const node = await getNode(graph, 'custom:thing');
      expect(node.prefix).toBe('custom');
      expect(node.prefixClass).toBe('unknown');
    });
  });

  describe('getNodesByPrefix', () => {
    beforeEach(async () => {
      await createEdge(graph, { source: 'task:auth', target: 'spec:auth', type: 'implements' });
      await createEdge(graph, { source: 'task:login', target: 'spec:session', type: 'implements' });
      await createEdge(graph, { source: 'file:src/auth.js', target: 'spec:auth', type: 'documents' });
    });

    it('returns nodes matching the prefix', async () => {
      const tasks = await getNodesByPrefix(graph, 'task');
      expect(tasks).toContain('task:auth');
      expect(tasks).toContain('task:login');
      expect(tasks.length).toBe(2);
    });

    it('returns empty array for non-matching prefix', async () => {
      const modules = await getNodesByPrefix(graph, 'module');
      expect(modules).toEqual([]);
    });

    it('does not match partial prefixes', async () => {
      const results = await getNodesByPrefix(graph, 'tas');
      expect(results).toEqual([]);
    });

    it('returns spec nodes correctly', async () => {
      const specs = await getNodesByPrefix(graph, 'spec');
      expect(specs).toContain('spec:auth');
      expect(specs).toContain('spec:session');
      expect(specs.length).toBe(2);
    });
  });

  describe('setNodeProperty', () => {
    beforeEach(async () => {
      await createEdge(graph, { source: 'task:auth', target: 'spec:auth', type: 'implements' });
    });

    it('sets a property and returns changed: true on first set', async () => {
      const result = await setNodeProperty(graph, 'task:auth', 'status', 'done');
      expect(result.id).toBe('task:auth');
      expect(result.key).toBe('status');
      expect(result.value).toBe('done');
      expect(result.previous).toBeNull();
      expect(result.changed).toBe(true);
    });

    it('property is visible via getNode after set', async () => {
      await setNodeProperty(graph, 'task:auth', 'status', 'done');
      const node = await getNode(graph, 'task:auth');
      expect(node.properties.status).toBe('done');
    });

    it('returns previous value on overwrite', async () => {
      await setNodeProperty(graph, 'task:auth', 'status', 'todo');
      const result = await setNodeProperty(graph, 'task:auth', 'status', 'done');
      expect(result.previous).toBe('todo');
      expect(result.changed).toBe(true);
    });

    it('returns changed: false when setting same value (idempotent)', async () => {
      await setNodeProperty(graph, 'task:auth', 'status', 'done');
      const result = await setNodeProperty(graph, 'task:auth', 'status', 'done');
      expect(result.changed).toBe(false);
      expect(result.previous).toBe('done');
    });

    it('throws for non-existent node', async () => {
      await expect(setNodeProperty(graph, 'task:nonexistent', 'status', 'done'))
        .rejects.toThrow('Node not found: task:nonexistent');
    });

    it('throws for empty key', async () => {
      await expect(setNodeProperty(graph, 'task:auth', '', 'done'))
        .rejects.toThrow('Property key must be a non-empty string');
    });

    it('two rapid sets on same node yield deterministic LWW result', async () => {
      await Promise.all([
        setNodeProperty(graph, 'task:auth', 'status', 'in-progress'),
        setNodeProperty(graph, 'task:auth', 'status', 'done'),
      ]);
      const node = await getNode(graph, 'task:auth');
      // LWW: one of the two values wins deterministically
      expect(['in-progress', 'done']).toContain(node.properties.status);
    });
  });

  describe('unsetNodeProperty', () => {
    beforeEach(async () => {
      await createEdge(graph, { source: 'task:auth', target: 'spec:auth', type: 'implements' });
    });

    it('returns removed: true when property existed', async () => {
      await setNodeProperty(graph, 'task:auth', 'status', 'done');
      const result = await unsetNodeProperty(graph, 'task:auth', 'status');
      expect(result.id).toBe('task:auth');
      expect(result.key).toBe('status');
      expect(result.previous).toBe('done');
      expect(result.removed).toBe(true);
    });

    it('property is nullified after unset', async () => {
      await setNodeProperty(graph, 'task:auth', 'status', 'done');
      await unsetNodeProperty(graph, 'task:auth', 'status');
      const node = await getNode(graph, 'task:auth');
      // CRDT stores null for removed properties (LWW semantics)
      expect(node.properties.status).toBeNull();
    });

    it('returns removed: false when property did not exist', async () => {
      const result = await unsetNodeProperty(graph, 'task:auth', 'status');
      expect(result.previous).toBeNull();
      expect(result.removed).toBe(false);
    });

    it('throws for non-existent node', async () => {
      await expect(unsetNodeProperty(graph, 'task:nonexistent', 'status'))
        .rejects.toThrow('Node not found: task:nonexistent');
    });

    it('throws for empty key', async () => {
      await expect(unsetNodeProperty(graph, 'task:auth', ''))
        .rejects.toThrow('Property key must be a non-empty string');
    });
  });
});
