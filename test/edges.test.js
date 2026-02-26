import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge, removeEdge, EDGE_TYPES } from '../src/edges.js';

describe('edges', () => {
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

  it('EDGE_TYPES contains expected types', () => {
    expect(EDGE_TYPES).toContain('implements');
    expect(EDGE_TYPES).toContain('augments');
    expect(EDGE_TYPES).toContain('relates-to');
    expect(EDGE_TYPES).toContain('depends-on');
    expect(EDGE_TYPES.length).toBe(8);
  });

  it('createEdge creates an edge and both nodes', async () => {
    await createEdge(graph, {
      source: 'file:src/auth.js',
      target: 'spec:auth-spec',
      type: 'implements',
    });

    const edges = await graph.getEdges();
    expect(edges.length).toBe(1);
    expect(edges[0].from).toBe('file:src/auth.js');
    expect(edges[0].to).toBe('spec:auth-spec');
    expect(edges[0].label).toBe('implements');
  });

  it('createEdge sets confidence and rationale', async () => {
    await createEdge(graph, {
      source: 'task:a',
      target: 'task:b',
      type: 'relates-to',
      confidence: 0.7,
      rationale: 'test rationale',
    });

    const edges = await graph.getEdges();
    expect(edges[0].props.confidence).toBe(0.7);
    expect(edges[0].props.rationale).toBe('test rationale');
  });

  it('createEdge rejects unknown edge types', async () => {
    await expect(
      createEdge(graph, { source: 'task:a', target: 'task:b', type: 'invalid-type' })
    ).rejects.toThrow(/Unknown edge type/);
  });

  it('createEdge rejects invalid confidence', async () => {
    await expect(
      createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to', confidence: 1.5 })
    ).rejects.toThrow(/between 0\.0 and 1\.0/);
  });

  it('createEdge rejects invalid node IDs', async () => {
    await expect(
      createEdge(graph, { source: 'bad id', target: 'task:b', type: 'relates-to' })
    ).rejects.toThrow(/Invalid node ID/);
  });

  it('createEdge rejects self-edge for blocks', async () => {
    await expect(
      createEdge(graph, { source: 'task:x', target: 'task:x', type: 'blocks' })
    ).rejects.toThrow(/self-edge/i);
  });

  it('createEdge rejects non-number confidence', async () => {
    await expect(
      createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to', confidence: '0.9' })
    ).rejects.toThrow(/must be a number/);
  });

  it('getEdges filters by source', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });
    await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'implements' });

    const filtered = (await graph.getEdges()).filter(e => e.from === 'task:a');
    expect(filtered.length).toBe(1);
    expect(filtered[0].from).toBe('task:a');
  });

  it('getEdges filters by type', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });
    await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'implements' });

    const filtered = (await graph.getEdges()).filter(e => e.label === 'implements');
    expect(filtered.length).toBe(1);
    expect(filtered[0].label).toBe('implements');
  });

  it('removeEdge removes an edge', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });
    expect((await graph.getEdges()).length).toBe(1);

    await removeEdge(graph, 'task:a', 'task:b', 'relates-to');
    expect((await graph.getEdges()).length).toBe(0);
  });
});
