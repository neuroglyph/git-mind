import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';

describe('graph', () => {
  let tempDir;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('initGraph creates a WarpGraph', async () => {
    const graph = await initGraph(tempDir);
    expect(graph).toBeDefined();
    expect(typeof graph.createPatch).toBe('function');
    expect(typeof graph.getNodes).toBe('function');
  });

  it('initGraph is idempotent — calling twice returns a valid graph', async () => {
    await initGraph(tempDir);
    const graph = await initGraph(tempDir);
    expect(graph).toBeDefined();
  });

  it('round-trip: add node, reload via initGraph, verify', async () => {
    const graph = await initGraph(tempDir);

    const patch = await graph.createPatch();
    patch.addNode('test-node');
    patch.setProperty('test-node', 'label', 'hello');
    await patch.commit();

    // Reload in a new instance
    const graph2 = await initGraph(tempDir);
    const hasNode = await graph2.hasNode('test-node');
    expect(hasNode).toBe(true);

    const props = await graph2.getNodeProps('test-node');
    expect(props.get('label')).toBe('hello');
  });
});
