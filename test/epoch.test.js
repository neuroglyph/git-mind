import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { getCurrentTick, recordEpoch, lookupEpoch, lookupNearestEpoch, getEpochForRef } from '../src/epoch.js';
import { processCommit } from '../src/hooks.js';
import { exportGraph } from '../src/export.js';
import { detectOrphanNodes } from '../src/doctor.js';
import { classifyPrefix } from '../src/validators.js';

describe('epoch', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.email "test@test.com"', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.name "Test"', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── getCurrentTick ─────────────────────────────────────────

  it('returns 0 for an empty graph', async () => {
    const tick = await getCurrentTick(graph);
    expect(tick).toBe(0);
  });

  it('returns positive tick after creating edges', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick = await getCurrentTick(graph);
    expect(tick).toBeGreaterThan(0);
  });

  // ── recordEpoch / lookupEpoch ──────────────────────────────

  it('records and looks up an epoch by SHA', async () => {
    await recordEpoch(graph, 'abc123def456', 42);
    const epoch = await lookupEpoch(graph, 'abc123def456');

    expect(epoch).not.toBeNull();
    expect(epoch.tick).toBe(42);
    expect(epoch.fullSha).toBe('abc123def456');
    expect(epoch.recordedAt).toBeTruthy();
  });

  it('returns null for a missing epoch', async () => {
    const epoch = await lookupEpoch(graph, 'nonexistent');
    expect(epoch).toBeNull();
  });

  it('uses first 8 chars of SHA as node ID', async () => {
    await recordEpoch(graph, 'abc123def456789', 10);
    const nodes = await graph.getNodes();
    expect(nodes).toContain('epoch:abc123de');
  });

  // ── lookupNearestEpoch ─────────────────────────────────────

  it('finds ancestor epoch when direct lookup fails', async () => {
    // Create a commit chain: c1 -> c2 -> c3
    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const c1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();

    await writeFile(join(tempDir, 'b.txt'), 'b');
    execSync('git add b.txt && git commit -m "c2"', { cwd: tempDir, stdio: 'ignore' });

    await writeFile(join(tempDir, 'c.txt'), 'c');
    execSync('git add c.txt && git commit -m "c3"', { cwd: tempDir, stdio: 'ignore' });
    const c3 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();

    // Record epoch only for c1
    await recordEpoch(graph, c1, 5);

    // Looking up from c3 should find c1's epoch as nearest
    const epoch = await lookupNearestEpoch(graph, tempDir, c3);
    expect(epoch).not.toBeNull();
    expect(epoch.tick).toBe(5);
    expect(epoch.nearest).toBe(true);
  });

  it('returns null when no ancestors have epochs', async () => {
    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const c1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();

    const epoch = await lookupNearestEpoch(graph, tempDir, c1);
    expect(epoch).toBeNull();
  });

  // ── getEpochForRef ─────────────────────────────────────────

  it('resolves ref and finds epoch', async () => {
    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const sha = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();

    await recordEpoch(graph, sha, 7);

    const result = await getEpochForRef(graph, tempDir, 'HEAD');
    expect(result).not.toBeNull();
    expect(result.sha).toBe(sha);
    expect(result.epoch.tick).toBe(7);
  });

  it('returns null for invalid ref', async () => {
    const result = await getEpochForRef(graph, tempDir, 'nonexistent-ref');
    expect(result).toBeNull();
  });

  it('falls back to nearest ancestor epoch', async () => {
    await writeFile(join(tempDir, 'a.txt'), 'a');
    execSync('git add a.txt && git commit -m "c1"', { cwd: tempDir, stdio: 'ignore' });
    const c1 = execSync('git rev-parse HEAD', { cwd: tempDir, encoding: 'utf-8' }).trim();

    await writeFile(join(tempDir, 'b.txt'), 'b');
    execSync('git add b.txt && git commit -m "c2"', { cwd: tempDir, stdio: 'ignore' });

    // Record epoch only for c1
    await recordEpoch(graph, c1, 3);

    const result = await getEpochForRef(graph, tempDir, 'HEAD');
    expect(result).not.toBeNull();
    expect(result.epoch.tick).toBe(3);
    expect(result.epoch.nearest).toBe(true);
  });

  // ── Integration: ceiling materialization ────────────────────

  it('materializes graph at historical tick, hiding later edges', async () => {
    // Create first batch of edges
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    const tick1 = await getCurrentTick(graph);

    // Record an epoch at this point
    await recordEpoch(graph, 'aaaa1111bbbb2222', tick1);

    // Create more edges
    await createEdge(graph, { source: 'task:c', target: 'spec:d', type: 'documents' });

    // Verify both edges exist now
    let edges = await graph.getEdges();
    expect(edges.length).toBe(2);

    // Materialize at ceiling = tick1 (should only see the first edge)
    await graph.materialize({ ceiling: tick1 });
    edges = await graph.getEdges();
    expect(edges.length).toBe(1);
    expect(edges[0].from).toBe('task:a');
    expect(edges[0].to).toBe('spec:b');
  });

  // ── Export filtering ────────────────────────────────────────

  it('excludes epoch nodes from export', async () => {
    await createEdge(graph, { source: 'task:a', target: 'spec:b', type: 'implements' });
    await recordEpoch(graph, 'abc123def456', 42);

    const data = await exportGraph(graph);
    const ids = data.nodes.map(n => n.id);
    expect(ids).not.toContain('epoch:abc123de');
    expect(ids).toContain('task:a');
  });

  // ── Doctor filtering ───────────────────────────────────────

  it('excludes epoch nodes from orphan detection', () => {
    const nodes = ['task:a', 'task:b', 'epoch:abc123de'];
    const edges = [{ from: 'task:a', to: 'task:b', label: 'blocks' }];
    const issues = detectOrphanNodes(nodes, edges);

    expect(issues).toHaveLength(0);
    expect(issues.find(i => i.affected[0] === 'epoch:abc123de')).toBeUndefined();
  });

  // ── Validators ──────────────────────────────────────────────

  it('classifies epoch prefix as system', () => {
    expect(classifyPrefix('epoch')).toBe('system');
  });

  // ── processCommit epoch recording ──────────────────────────

  it('processCommit records an epoch automatically', async () => {
    await processCommit(graph, {
      sha: 'fade0123cafe4567',
      message: 'fix: update auth\n\nIMPLEMENTS: spec:auth',
    });

    const epoch = await lookupEpoch(graph, 'fade0123cafe4567');
    expect(epoch).not.toBeNull();
    expect(epoch.tick).toBeGreaterThanOrEqual(0);
    expect(epoch.fullSha).toBe('fade0123cafe4567');
  });

  it('processCommit records epoch even without directives', async () => {
    await processCommit(graph, {
      sha: 'dead0000beef1111',
      message: 'chore: update deps',
    });

    const epoch = await lookupEpoch(graph, 'dead0000beef1111');
    expect(epoch).not.toBeNull();
  });
});
