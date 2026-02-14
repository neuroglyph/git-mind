import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { recordEpoch, getCurrentTick } from '../src/epoch.js';
import { diffSnapshots, computeDiff, parseDiffRefs, compareEdge } from '../src/diff.js';

/**
 * Create two separate graph instances in separate temp repos.
 * This is necessary because WarpGraph CRDTs merge all writers in a shared
 * repo — separate repos ensure each graph only sees its own data.
 */
async function createTwoGraphs() {
  const dirA = await mkdtemp(join(tmpdir(), 'gitmind-diff-a-'));
  const dirB = await mkdtemp(join(tmpdir(), 'gitmind-diff-b-'));

  for (const dir of [dirA, dirB]) {
    execSync('git init', { cwd: dir, stdio: 'ignore' });
    execSync('git config user.email "test@test.com"', { cwd: dir, stdio: 'ignore' });
    execSync('git config user.name "Test"', { cwd: dir, stdio: 'ignore' });
  }

  const graphA = await initGraph(dirA);
  const graphB = await initGraph(dirB);

  return { dirA, dirB, graphA, graphB };
}

// ── diffSnapshots ─────────────────────────────────────────────────

describe('diffSnapshots', () => {
  let dirs = [];

  afterEach(async () => {
    for (const d of dirs) {
      await rm(d, { recursive: true, force: true });
    }
    dirs = [];
  });

  it('detects added nodes', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphB, { source: 'task:new', target: 'spec:x', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.nodes.added).toContain('task:new');
    expect(diff.nodes.added).toContain('spec:x');
    expect(diff.nodes.removed).toHaveLength(0);
  });

  it('detects removed nodes', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:old', target: 'spec:y', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.nodes.removed).toContain('task:old');
    expect(diff.nodes.removed).toContain('spec:y');
    expect(diff.nodes.added).toHaveLength(0);
  });

  it('detects added and removed simultaneously', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:old', target: 'spec:shared', type: 'implements' });
    await createEdge(graphB, { source: 'task:new', target: 'spec:shared', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.nodes.added).toContain('task:new');
    expect(diff.nodes.removed).toContain('task:old');
    // spec:shared exists in both, so neither added nor removed
    expect(diff.nodes.added).not.toContain('spec:shared');
    expect(diff.nodes.removed).not.toContain('spec:shared');
  });

  it('returns empty diff for identical graphs', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.nodes.added).toHaveLength(0);
    expect(diff.nodes.removed).toHaveLength(0);
    expect(diff.edges.added).toHaveLength(0);
    expect(diff.edges.removed).toHaveLength(0);
  });

  it('detects added edges', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'documents' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.edges.added).toHaveLength(1);
    expect(diff.edges.added[0]).toEqual({
      source: 'task:a', target: 'spec:b', type: 'documents',
    });
    expect(diff.edges.removed).toHaveLength(0);
  });

  it('detects removed edges', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'documents' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.edges.removed).toHaveLength(1);
    expect(diff.edges.removed[0]).toEqual({
      source: 'task:a', target: 'spec:b', type: 'documents',
    });
    expect(diff.edges.added).toHaveLength(0);
  });

  it('excludes system nodes (epoch, decision, commit)', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    // Add system nodes + one real node to A
    const patchA = await graphA.createPatch();
    patchA.addNode('epoch:abc123def456');
    patchA.addNode('decision:d1');
    patchA.addNode('commit:c1');
    patchA.addNode('task:real');
    await patchA.commit();

    // Add same system nodes + one real node + one new node to B
    const patchB = await graphB.createPatch();
    patchB.addNode('epoch:abc123def456');
    patchB.addNode('decision:d1');
    patchB.addNode('commit:c1');
    patchB.addNode('task:real');
    patchB.addNode('task:new');
    await patchB.commit();

    const diff = await diffSnapshots(graphA, graphB);

    // Only task:new should show as added — system nodes excluded
    expect(diff.nodes.added).toEqual(['task:new']);
    expect(diff.nodes.removed).toHaveLength(0);
  });

  it('filters by prefix option', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:c', target: 'spec:d', type: 'implements' });
    await createEdge(graphB, { source: 'module:x', target: 'module:y', type: 'depends-on' });

    const diff = await diffSnapshots(graphA, graphB, { prefix: 'task' });

    // Only task-prefixed nodes should appear
    expect(diff.nodes.added).toContain('task:c');
    expect(diff.nodes.added).not.toContain('spec:d');
    expect(diff.nodes.added).not.toContain('module:x');
  });

  it('excludes edges when prefix filter removes an endpoint', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'task:b', type: 'blocks' });
    await createEdge(graphB, { source: 'task:a', target: 'task:b', type: 'blocks' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:x', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB, { prefix: 'task' });

    // The task:a -> spec:x edge should be excluded (spec:x doesn't pass filter)
    expect(diff.edges.added).toHaveLength(0);
  });

  it('computes summary nodesByPrefix correctly', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:c', target: 'spec:d', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.summary.nodesByPrefix.task).toEqual({ before: 1, after: 2 });
    expect(diff.summary.nodesByPrefix.spec).toEqual({ before: 1, after: 2 });
  });

  it('computes summary edgesByType correctly', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'documents' });

    const diff = await diffSnapshots(graphA, graphB);

    expect(diff.summary.edgesByType.implements).toEqual({ before: 1, after: 1 });
    expect(diff.summary.edgesByType.documents).toEqual({ before: 0, after: 1 });
  });

  it('includes prefix/type in summary even when only in one snapshot', async () => {
    const { dirA, dirB, graphA, graphB } = await createTwoGraphs();
    dirs.push(dirA, dirB);

    await createEdge(graphA, { source: 'module:x', target: 'module:y', type: 'depends-on' });
    await createEdge(graphB, { source: 'task:a', target: 'spec:b', type: 'implements' });

    const diff = await diffSnapshots(graphA, graphB);

    // module prefix only in A, task/spec only in B
    expect(diff.summary.nodesByPrefix.module).toEqual({ before: 2, after: 0 });
    expect(diff.summary.nodesByPrefix.task).toEqual({ before: 0, after: 1 });
    expect(diff.summary.nodesByPrefix.spec).toEqual({ before: 0, after: 1 });

    // depends-on only in A, implements only in B
    expect(diff.summary.edgesByType['depends-on']).toEqual({ before: 1, after: 0 });
    expect(diff.summary.edgesByType.implements).toEqual({ before: 0, after: 1 });
  });
});

// ── computeDiff ───────────────────────────────────────────────────

describe('computeDiff', () => {
  let tempDir;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-diff-int-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.email "test@test.com"', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.name "Test"', { cwd: tempDir, stdio: 'ignore' });
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('computes diff between two epochs end-to-end', async () => {
    const graph = await initGraph(tempDir);

    // Phase 1: create edges, record epoch
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick1 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const sha1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha1, tick1);

    // Phase 2: add more edges, record epoch
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'documents' });
    const tick2 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'b.txt'), 'b');
    execSync('git add b.txt && git commit -m "c2"', { cwd: tempDir, stdio: 'ignore' });
    const sha2 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha2, tick2);

    const diff = await computeDiff(tempDir, sha1.slice(0, 8), sha2.slice(0, 8));

    expect(diff.schemaVersion).toBe(1);
    expect(diff.nodes.added).toContain('task:c');
    expect(diff.nodes.added).toContain('spec:d');
    expect(diff.edges.added).toHaveLength(1);
    expect(diff.edges.added[0].type).toBe('documents');
    expect(diff.stats.materializeMs.a).toBeGreaterThanOrEqual(0);
    expect(diff.stats.materializeMs.b).toBeGreaterThanOrEqual(0);
  });

  it('throws descriptive error for ref with no epoch', async () => {
    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });

    await expect(computeDiff(tempDir, 'HEAD', 'HEAD'))
      .rejects.toThrow(/No epoch found for "HEAD"/);
  });

  it('handles nearest-epoch fallback', async () => {
    const graph = await initGraph(tempDir);

    // Create edge and epoch at c1
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick1 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const sha1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha1, tick1);

    // Add more edges and epoch at c2
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'documents' });
    const tick2 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'b.txt'), 'b');
    execSync('git add b.txt && git commit -m "c2"', { cwd: tempDir, stdio: 'ignore' });
    const sha2 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha2, tick2);

    // Create c3 WITHOUT an epoch
    await writeFile(join(tempDir, 'c.txt'), 'c');
    execSync('git add c.txt && git commit -m "c3"', { cwd: tempDir, stdio: 'ignore' });

    // Diff using HEAD (c3, no direct epoch) — should fall back to c2's epoch
    const diff = await computeDiff(tempDir, sha1.slice(0, 8), 'HEAD');

    expect(diff.to.nearest).toBe(true);
    expect(diff.nodes.added).toContain('task:c');
  });

  it('returns empty diff when both refs resolve to same tick', async () => {
    const graph = await initGraph(tempDir);

    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const sha = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha, tick);

    // Both refs point to same commit → same tick → empty diff
    const diff = await computeDiff(tempDir, 'HEAD', 'HEAD');

    expect(diff.nodes.added).toHaveLength(0);
    expect(diff.nodes.removed).toHaveLength(0);
    expect(diff.edges.added).toHaveLength(0);
    expect(diff.edges.removed).toHaveLength(0);
  });

  it('non-linear history: branch + merge with nearest fallback on one side', async () => {
    const graph = await initGraph(tempDir);

    // c1 on main with epoch
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick1 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const sha1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha1, tick1);

    // Branch off, add edges
    execSync('git checkout -b feature', { cwd: tempDir, stdio: 'ignore' });

    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'documents' });
    const tick2 = await getCurrentTick(graph);

    await writeFile(join(tempDir, 'b.txt'), 'b');
    execSync('git add b.txt && git commit -m "c2-feature"', { cwd: tempDir, stdio: 'ignore' });
    const sha2 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();
    await recordEpoch(graph, sha2, tick2);

    // Go back to main, make a commit so merge is not fast-forward
    execSync('git checkout -', { cwd: tempDir, stdio: 'ignore' });
    await writeFile(join(tempDir, 'c.txt'), 'c');
    execSync('git add c.txt && git commit -m "c3-main"', { cwd: tempDir, stdio: 'ignore' });

    // Merge feature into main (--no-ff ensures merge commit)
    execSync('git merge feature --no-edit', { cwd: tempDir, stdio: 'ignore' });

    // HEAD is now the merge commit (no epoch) — should fall back to ancestor's epoch
    const diff = await computeDiff(tempDir, sha1.slice(0, 8), 'HEAD');

    expect(diff.to.nearest).toBe(true);
    expect(diff.nodes.added).toContain('task:c');
  });
});

// ── parseDiffRefs ─────────────────────────────────────────────────

describe('parseDiffRefs', () => {
  it('parses A..B range syntax', () => {
    const result = parseDiffRefs(['HEAD~3..HEAD']);
    expect(result).toEqual({ refA: 'HEAD~3', refB: 'HEAD' });
  });

  it('parses A B two-arg syntax', () => {
    const result = parseDiffRefs(['abc123', 'def456']);
    expect(result).toEqual({ refA: 'abc123', refB: 'def456' });
  });

  it('single ref A defaults to A..HEAD', () => {
    const result = parseDiffRefs(['HEAD~5']);
    expect(result).toEqual({ refA: 'HEAD~5', refB: 'HEAD' });
  });

  it('rejects A..B..C (multiple "..")', () => {
    expect(() => parseDiffRefs(['A..B..C'])).toThrow(/multiple "\.\." separators/);
  });

  it('rejects ..B (empty left side)', () => {
    expect(() => parseDiffRefs(['..B'])).toThrow(/Left side.+empty/);
  });

  it('rejects A.. (empty right side)', () => {
    expect(() => parseDiffRefs(['A..'])).toThrow(/Right side.+empty/);
  });

  it('rejects empty input', () => {
    expect(() => parseDiffRefs([])).toThrow(/Usage/);
  });
});

// ── compareEdge ───────────────────────────────────────────────────

describe('compareEdge', () => {
  it('sorts by type first, then source, then target', () => {
    const edges = [
      { source: 'b:1', target: 'c:2', type: 'implements' },
      { source: 'a:1', target: 'c:2', type: 'documents' },
      { source: 'a:1', target: 'b:2', type: 'implements' },
    ];
    const sorted = [...edges].sort(compareEdge);

    expect(sorted[0].type).toBe('documents');
    expect(sorted[1].type).toBe('implements');
    expect(sorted[1].source).toBe('a:1');
    expect(sorted[2].type).toBe('implements');
    expect(sorted[2].source).toBe('b:1');
  });
});
