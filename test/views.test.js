import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { renderView, listViews, defineView } from '../src/views.js';

describe('views', () => {
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

  it('listViews returns built-in views', () => {
    const views = listViews();
    expect(views).toContain('roadmap');
    expect(views).toContain('architecture');
    expect(views).toContain('backlog');
    expect(views).toContain('suggestions');
  });

  it('renderView throws for unknown views', async () => {
    await expect(renderView(graph, 'nonexistent')).rejects.toThrow(/Unknown view/);
  });

  it('roadmap view filters for phase/task nodes', async () => {
    await createEdge(graph, { source: 'phase:alpha', target: 'task:build-cli', type: 'blocks' });
    await createEdge(graph, { source: 'src/main.js', target: 'docs/readme.md', type: 'documents' });

    const result = await renderView(graph, 'roadmap');
    expect(result.nodes).toContain('phase:alpha');
    expect(result.nodes).toContain('task:build-cli');
    expect(result.nodes).not.toContain('src/main.js');
  });

  it('architecture view filters for module nodes and depends-on edges', async () => {
    await createEdge(graph, { source: 'module:auth', target: 'module:db', type: 'depends-on' });
    await createEdge(graph, { source: 'task:fix', target: 'task:deploy', type: 'blocks' });

    const result = await renderView(graph, 'architecture');
    expect(result.nodes).toContain('module:auth');
    expect(result.nodes).toContain('module:db');
    expect(result.edges.length).toBe(1);
    expect(result.edges[0].label).toBe('depends-on');
  });

  it('suggestions view filters for low-confidence edges', async () => {
    await createEdge(graph, { source: 'a', target: 'b', type: 'relates-to', confidence: 0.3 });
    await createEdge(graph, { source: 'c', target: 'd', type: 'implements', confidence: 1.0 });

    const result = await renderView(graph, 'suggestions');
    expect(result.edges.length).toBe(1);
    expect(result.edges[0].from).toBe('a');
    expect(result.nodes).toContain('a');
    expect(result.nodes).toContain('b');
    expect(result.nodes).not.toContain('c');
  });

  it('defineView registers a custom view', async () => {
    defineView('custom', (nodes, edges) => ({
      nodes: nodes.filter(n => n.startsWith('x:')),
      edges: [],
    }));

    await createEdge(graph, { source: 'x:foo', target: 'y:bar', type: 'relates-to' });

    const result = await renderView(graph, 'custom');
    expect(result.nodes).toEqual(['x:foo']);
    expect(result.edges).toEqual([]);
  });
});
