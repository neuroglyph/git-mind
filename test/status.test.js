import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { computeStatus } from '../src/status.js';

describe('status', () => {
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

  it('returns all zeros for an empty graph', async () => {
    const s = await computeStatus(graph);

    expect(s.nodes.total).toBe(0);
    expect(s.nodes.byPrefix).toEqual({});
    expect(s.edges.total).toBe(0);
    expect(s.edges.byType).toEqual({});
    expect(s.health.blockedItems).toBe(0);
    expect(s.health.lowConfidence).toBe(0);
    expect(s.health.orphanNodes).toBe(0);
  });

  it('counts nodes and edges correctly', async () => {
    await createEdge(graph, { source: 'file:a.js', target: 'spec:auth', type: 'implements' });
    await createEdge(graph, { source: 'file:b.js', target: 'spec:auth', type: 'implements' });

    const s = await computeStatus(graph);

    expect(s.nodes.total).toBe(3);
    expect(s.edges.total).toBe(2);
  });

  it('groups nodes by prefix', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:auth', type: 'implements' });
    await createEdge(graph, { source: 'task:b', target: 'spec:session', type: 'implements' });
    await createEdge(graph, { source: 'file:x.js', target: 'spec:auth', type: 'documents' });

    const s = await computeStatus(graph);

    expect(s.nodes.byPrefix.task).toBe(2);
    expect(s.nodes.byPrefix.spec).toBe(2);
    expect(s.nodes.byPrefix.file).toBe(1);
  });

  it('groups edges by type', async () => {
    await createEdge(graph, { source: 'file:a.js', target: 'spec:auth', type: 'implements' });
    await createEdge(graph, { source: 'file:b.js', target: 'spec:auth', type: 'implements' });
    await createEdge(graph, { source: 'file:a.js', target: 'doc:readme', type: 'documents' });

    const s = await computeStatus(graph);

    expect(s.edges.byType.implements).toBe(2);
    expect(s.edges.byType.documents).toBe(1);
  });

  it('counts blocked items', async () => {
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'blocks' });
    await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'blocks' });
    await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements' });

    const s = await computeStatus(graph);

    expect(s.health.blockedItems).toBe(2);
  });

  it('counts low-confidence edges', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements', confidence: 0.3 });
    await createEdge(graph, { source: 'task:b', target: 'spec:y', type: 'implements', confidence: 0.49 });
    await createEdge(graph, { source: 'task:c', target: 'spec:z', type: 'implements', confidence: 0.5 });
    await createEdge(graph, { source: 'task:d', target: 'spec:w', type: 'implements', confidence: 1.0 });

    const s = await computeStatus(graph);

    expect(s.health.lowConfidence).toBe(2);
  });

  it('detects orphan nodes', async () => {
    // Create two edges, then remove one — the disconnected nodes become orphans
    await createEdge(graph, { source: 'task:a', target: 'task:b', type: 'relates-to' });
    await createEdge(graph, { source: 'task:c', target: 'task:d', type: 'relates-to' });

    // Remove the second edge — task:c and task:d become orphans
    const patch = await graph.createPatch();
    patch.removeEdge('task:c', 'task:d', 'relates-to');
    await patch.commit();

    const s = await computeStatus(graph);

    expect(s.health.orphanNodes).toBe(2);
  });

  it('returns correct structure for JSON serialization', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:x', type: 'implements' });

    const s = await computeStatus(graph);
    const json = JSON.parse(JSON.stringify(s));

    expect(json).toHaveProperty('nodes.total');
    expect(json).toHaveProperty('nodes.byPrefix');
    expect(json).toHaveProperty('edges.total');
    expect(json).toHaveProperty('edges.byType');
    expect(json).toHaveProperty('health.blockedItems');
    expect(json).toHaveProperty('health.lowConfidence');
    expect(json).toHaveProperty('health.orphanNodes');
  });
});
