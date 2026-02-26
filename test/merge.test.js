import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { detectRepoIdentifier, mergeFromRepo } from '../src/merge.js';

describe('merge', () => {
  let localDir;
  let remoteDir;
  let localGraph;
  let remoteGraph;

  beforeEach(async () => {
    localDir = await mkdtemp(join(tmpdir(), 'gitmind-local-'));
    remoteDir = await mkdtemp(join(tmpdir(), 'gitmind-remote-'));
    execSync('git init', { cwd: localDir, stdio: 'ignore' });
    execSync('git init', { cwd: remoteDir, stdio: 'ignore' });
    localGraph = await initGraph(localDir);
    remoteGraph = await initGraph(remoteDir);
  });

  afterEach(async () => {
    await rm(localDir, { recursive: true, force: true });
    await rm(remoteDir, { recursive: true, force: true });
  });

  // ── detectRepoIdentifier ────────────────────────────────────

  describe('detectRepoIdentifier', () => {
    it('parses HTTPS URL', () => {
      execSync('git remote add origin https://github.com/neuroglyph/echo.git', { cwd: remoteDir, stdio: 'ignore' });
      expect(detectRepoIdentifier(remoteDir)).toBe('neuroglyph/echo');
    });

    it('parses SSH URL', () => {
      execSync('git remote add origin git@github.com:neuroglyph/echo.git', { cwd: remoteDir, stdio: 'ignore' });
      expect(detectRepoIdentifier(remoteDir)).toBe('neuroglyph/echo');
    });

    it('returns null when no remote exists', () => {
      expect(detectRepoIdentifier(remoteDir)).toBeNull();
    });

    it('handles URLs without .git suffix', () => {
      execSync('git remote add origin https://github.com/owner/name', { cwd: remoteDir, stdio: 'ignore' });
      expect(detectRepoIdentifier(remoteDir)).toBe('owner/name');
    });
  });

  // ── mergeFromRepo ───────────────────────────────────────────

  describe('mergeFromRepo', () => {
    it('merges and qualifies remote nodes', async () => {
      // Populate remote graph
      await createEdge(remoteGraph, {
        source: 'spec:auth',
        target: 'module:auth',
        type: 'documents',
        confidence: 0.9,
        rationale: 'Auth spec docs',
      });

      const result = await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });
      expect(result.nodes).toBe(2);
      expect(result.edges).toBe(1);
      expect(result.repoName).toBe('other/repo');
      expect(result.dryRun).toBe(false);

      // Check local graph has qualified nodes
      const nodes = await localGraph.getNodes();
      expect(nodes).toContain('repo:other/repo:spec:auth');
      expect(nodes).toContain('repo:other/repo:module:auth');

      // Check qualified edges
      const edges = await localGraph.getEdges();
      expect(edges).toHaveLength(1);
      expect(edges[0].from).toBe('repo:other/repo:spec:auth');
      expect(edges[0].to).toBe('repo:other/repo:module:auth');
      expect(edges[0].label).toBe('documents');
    });

    it('preserves local data (additive only)', async () => {
      // Pre-populate local graph
      await createEdge(localGraph, {
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
      });

      // Populate remote
      await createEdge(remoteGraph, {
        source: 'crate:core',
        target: 'spec:api',
        type: 'implements',
      });

      await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });

      const nodes = await localGraph.getNodes();
      expect(nodes).toContain('task:a');
      expect(nodes).toContain('spec:b');
      expect(nodes).toContain('repo:other/repo:crate:core');

      const edges = await localGraph.getEdges();
      expect(edges).toHaveLength(2); // local + remote
    });

    it('preserves edge properties (confidence + rationale)', async () => {
      await createEdge(remoteGraph, {
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
        confidence: 0.7,
        rationale: 'Test rationale',
      });

      await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });

      const edges = await localGraph.getEdges();
      expect(edges[0].props.confidence).toBe(0.7);
      expect(edges[0].props.rationale).toBe('Test rationale');
    });

    it('handles dry-run mode', async () => {
      await createEdge(remoteGraph, {
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
      });

      const result = await mergeFromRepo(localGraph, remoteDir, {
        repoName: 'other/repo',
        dryRun: true,
      });

      expect(result.dryRun).toBe(true);
      expect(result.nodes).toBe(2);
      expect(result.edges).toBe(1);

      // Local graph should be unchanged
      const nodes = await localGraph.getNodes();
      expect(nodes).toHaveLength(0);
    });

    it('handles empty remote graph', async () => {
      const result = await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });
      expect(result.nodes).toBe(0);
      expect(result.edges).toBe(0);
    });

    it('is idempotent on re-merge', async () => {
      await createEdge(remoteGraph, {
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
      });

      await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });
      await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });

      const nodes = await localGraph.getNodes();
      // Should not duplicate
      const qualifiedNodes = nodes.filter(n => n.startsWith('repo:'));
      expect(qualifiedNodes).toHaveLength(2);

      const edges = await localGraph.getEdges();
      expect(edges).toHaveLength(1);
    });

    it('throws when repo name cannot be detected', async () => {
      await expect(mergeFromRepo(localGraph, remoteDir))
        .rejects.toThrow(/Could not detect repository identifier/);
    });

    it('copies node properties', async () => {
      const patch = await remoteGraph.createPatch();
      patch.addNode('task:a');
      patch.setProperty('task:a', 'status', 'active');
      patch.setProperty('task:a', 'priority', 'high');
      await patch.commit();

      await mergeFromRepo(localGraph, remoteDir, { repoName: 'other/repo' });

      const props = await localGraph.getNodeProps('repo:other/repo:task:a');
      expect(props.get('status')).toBe('active');
      expect(props.get('priority')).toBe('high');
    });
  });
});
