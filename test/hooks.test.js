import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, readFile, stat, writeFile, mkdir } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { queryEdges } from '../src/edges.js';
import { parseDirectives, processCommit } from '../src/hooks.js';
import { installHooks } from '../src/cli/commands.js';

describe('hooks', () => {
  describe('parseDirectives', () => {
    it('parses IMPLEMENTS directive', () => {
      const result = parseDirectives('fix auth flow\n\nIMPLEMENTS: spec:auth');
      expect(result).toEqual([
        { type: 'implements', target: 'spec:auth' },
      ]);
    });

    it('parses multiple directives', () => {
      const msg = `refactor auth module

IMPLEMENTS: spec:auth
AUGMENTS: module:security
RELATES-TO: module:session`;

      const result = parseDirectives(msg);
      expect(result.length).toBe(3);
      expect(result[0]).toEqual({ type: 'implements', target: 'spec:auth' });
      expect(result[1]).toEqual({ type: 'augments', target: 'module:security' });
      expect(result[2]).toEqual({ type: 'relates-to', target: 'module:session' });
    });

    it('is case-insensitive for directives', () => {
      const result = parseDirectives('implements: spec:foo');
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
      const result = parseDirectives('DOCUMENTS: doc:api-endpoints');
      expect(result).toEqual([
        { type: 'documents', target: 'doc:api-endpoints' },
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
        message: 'add login\n\nIMPLEMENTS: spec:auth',
      });

      expect(directives.length).toBe(1);

      const edges = await queryEdges(graph);
      expect(edges.length).toBe(1);
      expect(edges[0].from).toBe('commit:abc123def456');
      expect(edges[0].to).toBe('spec:auth');
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

  describe('installHooks', () => {
    let tempDir;

    beforeEach(async () => {
      tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
      execSync('git init', { cwd: tempDir, stdio: 'ignore' });
      await mkdir(join(tempDir, '.git', 'hooks'), { recursive: true });
    });

    afterEach(async () => {
      await rm(tempDir, { recursive: true, force: true });
    });

    it('creates pre-push hook', async () => {
      await installHooks(tempDir);

      const prePush = await readFile(join(tempDir, '.git', 'hooks', 'pre-push'), 'utf-8');
      expect(prePush).toContain('git-mind suggest');
      expect(prePush).toContain('process-commit');
    });

    it('makes hook executable', async () => {
      await installHooks(tempDir);

      const prePushStat = await stat(join(tempDir, '.git', 'hooks', 'pre-push'));
      expect(prePushStat.mode & 0o111).toBeTruthy();
    });

    it('does not overwrite existing hook', async () => {
      const existingContent = '#!/bin/sh\necho "existing hook"';
      await writeFile(join(tempDir, '.git', 'hooks', 'pre-push'), existingContent);

      await installHooks(tempDir);

      const prePush = await readFile(join(tempDir, '.git', 'hooks', 'pre-push'), 'utf-8');
      expect(prePush).toBe(existingContent);
    });

    it('hook runs suggest only when GITMIND_AGENT is set', async () => {
      await installHooks(tempDir);

      const prePush = await readFile(join(tempDir, '.git', 'hooks', 'pre-push'), 'utf-8');
      expect(prePush).toContain('GITMIND_AGENT');
      expect(prePush).toMatch(/if \[ -n "\$GITMIND_AGENT" \]/);
    });

    it('hook always exits 0', async () => {
      await installHooks(tempDir);

      const prePush = await readFile(join(tempDir, '.git', 'hooks', 'pre-push'), 'utf-8');
      expect(prePush).toContain('exit 0');
      expect(prePush).toContain('|| true');
    });

    it('hook processes directives unconditionally', async () => {
      await installHooks(tempDir);

      const prePush = await readFile(join(tempDir, '.git', 'hooks', 'pre-push'), 'utf-8');
      // process-commit runs outside the GITMIND_AGENT guard
      const agentGuardIndex = prePush.indexOf('if [ -n "$GITMIND_AGENT" ]');
      const processCommitIndex = prePush.indexOf('process-commit');
      expect(processCommitIndex).toBeLessThan(agentGuardIndex);
    });
  });
});
