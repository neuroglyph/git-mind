import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { getNodes, hasNode, getNode, getNodesByPrefix } from '../src/nodes.js';

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

  describe('getNodes', () => {
    it('returns empty array for a fresh graph', async () => {
      const nodes = await getNodes(graph);
      expect(nodes).toEqual([]);
    });

    it('returns nodes created by edges', async () => {
      await createEdge(graph, {
        source: 'file:src/auth.js',
        target: 'spec:auth',
        type: 'implements',
      });

      const nodes = await getNodes(graph);
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

      const nodes = await getNodes(graph);
      expect(nodes).toContain('file:a.js');
      expect(nodes).toContain('spec:auth');
      expect(nodes).toContain('doc:readme');
      expect(nodes.length).toBe(3);
    });
  });

  describe('hasNode', () => {
    it('returns false for non-existent node', async () => {
      expect(await hasNode(graph, 'task:nonexistent')).toBe(false);
    });

    it('returns true for node created via edge', async () => {
      await createEdge(graph, {
        source: 'task:auth',
        target: 'spec:auth',
        type: 'implements',
      });

      expect(await hasNode(graph, 'task:auth')).toBe(true);
      expect(await hasNode(graph, 'spec:auth')).toBe(true);
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
});
