import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { queryEdges } from '../src/edges.js';
import { parseDirectives, processCommit } from '../src/hooks.js';

describe('hooks', () => {
  describe('parseDirectives', () => {
    it('parses IMPLEMENTS directive', () => {
      const result = parseDirectives('fix auth flow\n\nIMPLEMENTS: docs/auth-spec.md');
      expect(result).toEqual([
        { type: 'implements', target: 'docs/auth-spec.md' },
      ]);
    });

    it('parses multiple directives', () => {
      const msg = `refactor auth module

IMPLEMENTS: docs/auth-spec.md
AUGMENTS: docs/security.md
RELATES-TO: src/session.js`;

      const result = parseDirectives(msg);
      expect(result.length).toBe(3);
      expect(result[0]).toEqual({ type: 'implements', target: 'docs/auth-spec.md' });
      expect(result[1]).toEqual({ type: 'augments', target: 'docs/security.md' });
      expect(result[2]).toEqual({ type: 'relates-to', target: 'src/session.js' });
    });

    it('is case-insensitive for directives', () => {
      const result = parseDirectives('implements: foo.md');
      expect(result.length).toBe(1);
      expect(result[0].type).toBe('implements');
    });

    it('returns empty for no directives', () => {
      const result = parseDirectives('just a normal commit message');
      expect(result).toEqual([]);
    });

    it('handles BLOCKS and DEPENDS-ON', () => {
      const result = parseDirectives('BLOCKS: task:deploy\nDEPENDS-ON: module:auth');
      expect(result.length).toBe(2);
      expect(result[0]).toEqual({ type: 'blocks', target: 'task:deploy' });
      expect(result[1]).toEqual({ type: 'depends-on', target: 'module:auth' });
    });

    it('handles DOCUMENTS directive', () => {
      const result = parseDirectives('DOCUMENTS: api/endpoints.md');
      expect(result).toEqual([
        { type: 'documents', target: 'api/endpoints.md' },
      ]);
    });
  });

  describe('processCommit', () => {
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

    it('creates edges from commit directives', async () => {
      const directives = await processCommit(graph, {
        sha: 'abc123def456',
        message: 'add login\n\nIMPLEMENTS: docs/auth.md',
      });

      expect(directives.length).toBe(1);

      const edges = await queryEdges(graph);
      expect(edges.length).toBe(1);
      expect(edges[0].from).toBe('commit:abc123def456');
      expect(edges[0].to).toBe('docs/auth.md');
      expect(edges[0].label).toBe('implements');
      expect(edges[0].props.confidence).toBe(0.8);
    });

    it('returns empty for commits without directives', async () => {
      const directives = await processCommit(graph, {
        sha: 'abc123',
        message: 'fix typo',
      });

      expect(directives).toEqual([]);
      const edges = await queryEdges(graph);
      expect(edges.length).toBe(0);
    });
  });
});
