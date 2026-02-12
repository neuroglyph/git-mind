import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, readFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { importFile } from '../src/import.js';
import { exportGraph, serializeExport, exportToFile } from '../src/export.js';

describe('export', () => {
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

  // ── Empty graph ──────────────────────────────────────────────

  it('exports empty graph', async () => {
    const data = await exportGraph(graph);
    expect(data.version).toBe(1);
    expect(data.nodes).toEqual([]);
    expect(data.edges).toEqual([]);
  });

  // ── Basic export ─────────────────────────────────────────────

  it('exports nodes and edges', async () => {
    await createEdge(graph, {
      source: 'spec:auth',
      target: 'file:src/auth.js',
      type: 'documents',
      confidence: 0.9,
      rationale: 'Auth spec',
    });

    const data = await exportGraph(graph);
    expect(data.version).toBe(1);
    expect(data.nodes).toHaveLength(2);
    expect(data.nodes.map(n => n.id).sort()).toEqual(['file:src/auth.js', 'spec:auth']);
    expect(data.edges).toHaveLength(1);
    expect(data.edges[0].source).toBe('spec:auth');
    expect(data.edges[0].target).toBe('file:src/auth.js');
    expect(data.edges[0].type).toBe('documents');
  });

  // ── Edge properties ──────────────────────────────────────────

  it('includes confidence and rationale, excludes timestamps', async () => {
    await createEdge(graph, {
      source: 'task:a',
      target: 'spec:b',
      type: 'implements',
      confidence: 0.8,
      rationale: 'Test rationale',
    });

    const data = await exportGraph(graph);
    const edge = data.edges[0];
    expect(edge.confidence).toBe(0.8);
    expect(edge.rationale).toBe('Test rationale');
    expect(edge.createdAt).toBeUndefined();
    expect(edge.importedAt).toBeUndefined();
  });

  // ── System node exclusion ────────────────────────────────────

  it('excludes decision: nodes', async () => {
    // Create a regular edge
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });

    // Create a decision node (simulating review provenance)
    const patch = await graph.createPatch();
    patch.addNode('decision:123');
    patch.setProperty('decision:123', 'action', 'accept');
    await patch.commit();

    const data = await exportGraph(graph);
    const ids = data.nodes.map(n => n.id);
    expect(ids).not.toContain('decision:123');
    expect(ids).toContain('task:a');
    expect(ids).toContain('spec:b');
  });

  it('excludes commit: nodes', async () => {
    const patch = await graph.createPatch();
    patch.addNode('commit:abc123');
    patch.addNode('task:a');
    await patch.commit();

    const data = await exportGraph(graph);
    const ids = data.nodes.map(n => n.id);
    expect(ids).not.toContain('commit:abc123');
    expect(ids).toContain('task:a');
  });

  // ── Prefix filtering ────────────────────────────────────────

  it('filters by prefix', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
    await createEdge(graph, { source: 'spec:auth', target: 'task:a', type: 'relates-to' });

    const data = await exportGraph(graph, { prefix: 'task' });
    expect(data.nodes).toHaveLength(2);
    expect(data.nodes.every(n => n.id.startsWith('task:'))).toBe(true);
    // Only the edge between two task: nodes should be included
    expect(data.edges).toHaveLength(1);
    expect(data.edges[0].source).toBe('task:a');
    expect(data.edges[0].target).toBe('task:b');
  });

  // ── Node properties ──────────────────────────────────────────

  it('exports node properties', async () => {
    const patch = await graph.createPatch();
    patch.addNode('task:auth');
    patch.setProperty('task:auth', 'status', 'active');
    patch.setProperty('task:auth', 'priority', 'high');
    await patch.commit();

    const data = await exportGraph(graph);
    const node = data.nodes.find(n => n.id === 'task:auth');
    expect(node.properties).toEqual({ status: 'active', priority: 'high' });
  });

  it('omits properties key when node has none', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });

    const data = await exportGraph(graph);
    // Nodes created implicitly by createEdge have no user properties
    // (they may have system properties from edge creation, but node-level props are empty)
    for (const node of data.nodes) {
      if (node.properties) {
        expect(Object.keys(node.properties).length).toBeGreaterThan(0);
      }
    }
  });

  // ── Serialization ────────────────────────────────────────────

  it('serializes to YAML', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const data = await exportGraph(graph);
    const yamlStr = serializeExport(data, 'yaml');
    expect(yamlStr).toContain('version: 1');
    expect(yamlStr).toContain('task:a');
    expect(yamlStr).toContain('implements');
  });

  it('serializes to JSON', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const data = await exportGraph(graph);
    const jsonStr = serializeExport(data, 'json');
    const parsed = JSON.parse(jsonStr);
    expect(parsed.version).toBe(1);
    expect(parsed.nodes.length).toBe(2);
  });

  // ── File export ──────────────────────────────────────────────

  it('exports to file', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const outPath = join(tempDir, 'export.yaml');

    const result = await exportToFile(graph, outPath);
    expect(result.stats.nodes).toBe(2);
    expect(result.stats.edges).toBe(1);
    expect(result.path).toBe(outPath);

    const content = await readFile(outPath, 'utf-8');
    expect(content).toContain('version: 1');
  });

  it('infers JSON format from file extension', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const outPath = join(tempDir, 'export.json');

    await exportToFile(graph, outPath);
    const content = await readFile(outPath, 'utf-8');
    const parsed = JSON.parse(content);
    expect(parsed.version).toBe(1);
  });

  // ── Round-trip ───────────────────────────────────────────────

  it('round-trips: export → import → compare', async () => {
    // Build a graph
    await createEdge(graph, {
      source: 'file:src/auth.js',
      target: 'spec:auth',
      type: 'implements',
      confidence: 0.9,
      rationale: 'Main auth module',
    });

    const patch = await graph.createPatch();
    patch.addNode('task:auth');
    patch.setProperty('task:auth', 'status', 'active');
    await patch.commit();

    // Export
    const exportPath = join(tempDir, 'roundtrip.yaml');
    await exportToFile(graph, exportPath);

    // Import into a fresh graph
    const freshDir = await mkdtemp(join(tmpdir(), 'gitmind-rt-'));
    try {
      execSync('git init', { cwd: freshDir, stdio: 'ignore' });
      const freshGraph = await initGraph(freshDir);

      const importResult = await importFile(freshGraph, exportPath);
      expect(importResult.valid).toBe(true);
      expect(importResult.errors).toEqual([]);

      // Compare
      const freshNodes = (await freshGraph.getNodes()).sort();
      const origExport = await exportGraph(graph);
      const origNodeIds = origExport.nodes.map(n => n.id).sort();
      expect(freshNodes).toEqual(origNodeIds);

      const freshEdges = await freshGraph.getEdges();
      expect(freshEdges).toHaveLength(1);
      expect(freshEdges[0].props.confidence).toBe(0.9);
      expect(freshEdges[0].props.rationale).toBe('Main auth module');
    } finally {
      await rm(freshDir, { recursive: true, force: true });
    }
  });
});
