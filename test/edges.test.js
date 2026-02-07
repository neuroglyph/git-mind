import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge, queryEdges, removeEdge, EDGE_TYPES } from '../src/edges.js';

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
      source: 'src/auth.js',
      target: 'docs/auth-spec.md',
      type: 'implements',
    });

    const edges = await queryEdges(graph);
    expect(edges.length).toBe(1);
    expect(edges[0].from).toBe('src/auth.js');
    expect(edges[0].to).toBe('docs/auth-spec.md');
    expect(edges[0].label).toBe('implements');
  });

  it('createEdge sets confidence and rationale', async () => {
    await createEdge(graph, {
      source: 'a',
      target: 'b',
      type: 'relates-to',
      confidence: 0.7,
      rationale: 'test rationale',
    });

    const edges = await queryEdges(graph);
    expect(edges[0].props.confidence).toBe(0.7);
    expect(edges[0].props.rationale).toBe('test rationale');
  });

  it('createEdge rejects unknown edge types', async () => {
    await expect(
      createEdge(graph, { source: 'a', target: 'b', type: 'invalid-type' })
    ).rejects.toThrow(/Unknown edge type/);
  });

  it('createEdge rejects invalid confidence', async () => {
    await expect(
      createEdge(graph, { source: 'a', target: 'b', type: 'relates-to', confidence: 1.5 })
    ).rejects.toThrow(/Confidence must be between/);
  });

  it('queryEdges filters by source', async () => {
    await createEdge(graph, { source: 'a', target: 'b', type: 'relates-to' });
    await createEdge(graph, { source: 'c', target: 'd', type: 'implements' });

    const filtered = await queryEdges(graph, { source: 'a' });
    expect(filtered.length).toBe(1);
    expect(filtered[0].from).toBe('a');
  });

  it('queryEdges filters by type', async () => {
    await createEdge(graph, { source: 'a', target: 'b', type: 'relates-to' });
    await createEdge(graph, { source: 'c', target: 'd', type: 'implements' });

    const filtered = await queryEdges(graph, { type: 'implements' });
    expect(filtered.length).toBe(1);
    expect(filtered[0].label).toBe('implements');
  });

  it('removeEdge removes an edge', async () => {
    await createEdge(graph, { source: 'a', target: 'b', type: 'relates-to' });
    expect((await queryEdges(graph)).length).toBe(1);

    await removeEdge(graph, 'a', 'b', 'relates-to');
    expect((await queryEdges(graph)).length).toBe(0);
  });
});
