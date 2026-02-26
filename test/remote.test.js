import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import {
  parseCrossRepoId, buildCrossRepoId, isCrossRepoId,
  extractRepo, qualifyNodeId, CROSS_REPO_ID_REGEX,
} from '../src/remote.js';
import {
  validateNodeId, extractPrefix, classifyPrefix,
} from '../src/validators.js';

describe('remote', () => {
  // ── parseCrossRepoId ────────────────────────────────────────

  describe('parseCrossRepoId', () => {
    it('parses a valid cross-repo ID', () => {
      const result = parseCrossRepoId('repo:neuroglyph/echo:crate:echo-core');
      expect(result).toEqual({
        repo: 'neuroglyph/echo',
        prefix: 'crate',
        identifier: 'echo-core',
        local: 'crate:echo-core',
      });
    });

    it('parses cross-repo ID with dots in repo name', () => {
      const result = parseCrossRepoId('repo:my.org/my-repo:task:BDK-001');
      expect(result).toEqual({
        repo: 'my.org/my-repo',
        prefix: 'task',
        identifier: 'BDK-001',
        local: 'task:BDK-001',
      });
    });

    it('returns null for standard node ID', () => {
      expect(parseCrossRepoId('task:a')).toBeNull();
    });

    it('returns null for non-string input', () => {
      expect(parseCrossRepoId(null)).toBeNull();
      expect(parseCrossRepoId(42)).toBeNull();
    });

    it('returns null for malformed cross-repo ID', () => {
      expect(parseCrossRepoId('repo:noslash:task:a')).toBeNull();
    });
  });

  // ── buildCrossRepoId ────────────────────────────────────────

  describe('buildCrossRepoId', () => {
    it('builds a cross-repo ID', () => {
      expect(buildCrossRepoId('neuroglyph/echo', 'crate:echo-core'))
        .toBe('repo:neuroglyph/echo:crate:echo-core');
    });

    it('throws on invalid localId', () => {
      expect(() => buildCrossRepoId('owner/name', 'nocolon')).toThrow(/prefix:identifier/);
    });
  });

  // ── isCrossRepoId ──────────────────────────────────────────

  describe('isCrossRepoId', () => {
    it('returns true for cross-repo IDs', () => {
      expect(isCrossRepoId('repo:owner/name:task:a')).toBe(true);
    });

    it('returns false for standard IDs', () => {
      expect(isCrossRepoId('task:a')).toBe(false);
    });

    it('returns false for non-strings', () => {
      expect(isCrossRepoId(undefined)).toBe(false);
    });
  });

  // ── extractRepo ─────────────────────────────────────────────

  describe('extractRepo', () => {
    it('extracts repo from cross-repo ID', () => {
      expect(extractRepo('repo:neuroglyph/echo:crate:echo-core')).toBe('neuroglyph/echo');
    });

    it('returns null for standard ID', () => {
      expect(extractRepo('task:a')).toBeNull();
    });
  });

  // ── qualifyNodeId ───────────────────────────────────────────

  describe('qualifyNodeId', () => {
    it('qualifies a local ID', () => {
      expect(qualifyNodeId('crate:echo-core', 'neuroglyph/echo'))
        .toBe('repo:neuroglyph/echo:crate:echo-core');
    });

    it('returns cross-repo ID unchanged', () => {
      const id = 'repo:neuroglyph/echo:crate:echo-core';
      expect(qualifyNodeId(id, 'other/repo')).toBe(id);
    });

    it('throws a clear error for non-prefixed local IDs', () => {
      expect(() => qualifyNodeId('readme', 'owner/name'))
        .toThrow(/not a valid node ID.*prefix:identifier/);
    });

    it('throws a clear error for multi-colon local IDs', () => {
      expect(() => qualifyNodeId('a:b:c', 'owner/name'))
        .toThrow(/not a valid node ID/);
    });
  });

  // ── Validator integration ───────────────────────────────────

  describe('validator integration', () => {
    it('validateNodeId accepts cross-repo IDs', () => {
      const result = validateNodeId('repo:neuroglyph/echo:crate:echo-core');
      expect(result.valid).toBe(true);
    });

    it('validateNodeId still rejects invalid IDs', () => {
      const result = validateNodeId('repo bad spaces');
      expect(result.valid).toBe(false);
    });

    it('extractPrefix returns inner prefix for cross-repo IDs', () => {
      expect(extractPrefix('repo:neuroglyph/echo:crate:echo-core')).toBe('crate');
    });

    it('classifyPrefix recognizes repo as system', () => {
      expect(classifyPrefix('repo')).toBe('system');
    });
  });

  // ── Graph integration ───────────────────────────────────────

  describe('graph integration', () => {
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

    it('creates edge with cross-repo endpoint', async () => {
      await createEdge(graph, {
        source: 'file:src/auth.js',
        target: 'repo:neuroglyph/echo:spec:auth',
        type: 'implements',
      });

      const edges = await graph.getEdges();
      expect(edges).toHaveLength(1);
      expect(edges[0].to).toBe('repo:neuroglyph/echo:spec:auth');
    });

    it('queries edges with cross-repo nodes', async () => {
      await createEdge(graph, {
        source: 'repo:neuroglyph/echo:crate:echo-core',
        target: 'module:auth',
        type: 'depends-on',
      });

      const edges = (await graph.getEdges()).filter(e => e.from === 'repo:neuroglyph/echo:crate:echo-core');
      expect(edges).toHaveLength(1);
      expect(edges[0].label).toBe('depends-on');
    });
  });
});
