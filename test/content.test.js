/**
 * @module test/content
 * Tests for content-on-node: CAS storage, CLI commands, and schema contracts.
 */

import { describe, it, expect, beforeEach, afterEach, beforeAll } from 'vitest';
import { mkdtemp, rm, readFile, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execFileSync } from 'node:child_process';
import Ajv from 'ajv/dist/2020.js';
import { CONTENT_PROPERTY_KEY } from '@git-stunts/git-warp';
import { initGraph } from '../src/graph.js';
import { writeContent, readContent, getContentMeta, hasContent, deleteContent } from '../src/content.js';

/** Create a temp dir with an initialized git repo. */
async function setupGitRepo() {
  const dir = await mkdtemp(join(tmpdir(), 'gitmind-content-'));
  execFileSync('git', ['init'], { cwd: dir, stdio: 'ignore' });
  execFileSync('git', ['config', 'user.email', 'test@test.com'], { cwd: dir, stdio: 'ignore' });
  execFileSync('git', ['config', 'user.name', 'Test'], { cwd: dir, stdio: 'ignore' });
  return dir;
}

const BIN = join(import.meta.dirname, '..', 'bin', 'git-mind.js');
const SCHEMA_DIR = join(import.meta.dirname, '..', 'docs', 'contracts', 'cli');

function runCli(args, cwd) {
  return execFileSync(process.execPath, [BIN, ...args], {
    cwd,
    encoding: 'utf-8',
    timeout: 30_000,
    env: { ...process.env, NO_COLOR: '1' },
  });
}

function runCliJson(args, cwd) {
  return JSON.parse(runCli(args, cwd));
}

async function loadSchema(name) {
  return JSON.parse(await readFile(join(SCHEMA_DIR, name), 'utf-8'));
}

describe('content store core', () => {
  let tempDir, graph;

  beforeEach(async () => {
    tempDir = await setupGitRepo();
    graph = await initGraph(tempDir);

    // Create a test node
    const patch = await graph.createPatch();
    patch.addNode('doc:readme');
    patch.setProperty('doc:readme', 'title', 'README');
    await patch.commit();
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('writeContent stores blob and sets properties', async () => {
    const result = await writeContent(graph, 'doc:readme', '# Hello World\n', {
      mime: 'text/markdown',
    });

    expect(result.nodeId).toBe('doc:readme');
    expect(result.sha).toMatch(/^[0-9a-f]{40,64}$/);
    expect(result.mime).toBe('text/markdown');
    expect(result.size).toBe(Buffer.from('# Hello World\n').length);
    expect(result.encoding).toBeUndefined();
  });

  it('readContent retrieves correct content', async () => {
    const body = '# Hello World\n\nThis is a test document.\n';
    await writeContent(graph, 'doc:readme', body, { mime: 'text/markdown' });

    const { content, meta } = await readContent(graph, 'doc:readme');
    expect(content).toBe(body);
    expect(meta.mime).toBe('text/markdown');
  });

  it('readContent throws when blob is missing from object store', async () => {
    await writeContent(graph, 'doc:readme', 'original', { mime: 'text/plain' });

    // Point to a valid-looking SHA that doesn't exist in the object store
    const patch = await graph.createPatch();
    patch.setProperty('doc:readme', CONTENT_PROPERTY_KEY, 'deadbeefdeadbeefdeadbeefdeadbeefdeadbeef');
    await patch.commit();

    await expect(readContent(graph, 'doc:readme')).rejects.toThrow(/not found in git object store/);
  });

  it('getContentMeta returns correct metadata', async () => {
    await writeContent(graph, 'doc:readme', 'test', { mime: 'text/plain' });

    const meta = await getContentMeta(graph, 'doc:readme');
    expect(meta).not.toBeNull();
    expect(meta.sha).toMatch(/^[0-9a-f]{40,64}$/);
    expect(meta.mime).toBe('text/plain');
    expect(meta.size).toBe(4);
    expect(meta.encoding).toBeUndefined();
  });

  it('getContentMeta returns null for node without content', async () => {
    const meta = await getContentMeta(graph, 'doc:readme');
    expect(meta).toBeNull();
  });

  it('hasContent returns true for node with content', async () => {
    await writeContent(graph, 'doc:readme', 'test', { mime: 'text/plain' });
    expect(await hasContent(graph, 'doc:readme')).toBe(true);
  });

  it('hasContent returns false for node without content', async () => {
    expect(await hasContent(graph, 'doc:readme')).toBe(false);
  });

  it('hasContent returns false for non-existent node', async () => {
    expect(await hasContent(graph, 'doc:nonexistent')).toBe(false);
  });

  it('deleteContent removes properties', async () => {
    await writeContent(graph, 'doc:readme', 'test', { mime: 'text/plain' });
    const result = await deleteContent(graph, 'doc:readme');

    expect(result.removed).toBe(true);
    expect(result.previousSha).toMatch(/^[0-9a-f]{40,64}$/);
    expect(await hasContent(graph, 'doc:readme')).toBe(false);
  });

  it('deleteContent is idempotent on node without content', async () => {
    const result = await deleteContent(graph, 'doc:readme');
    expect(result.removed).toBe(false);
    expect(result.previousSha).toBeNull();
  });

  it('writeContent fails on non-existent node', async () => {
    await expect(
      writeContent(graph, 'doc:nonexistent', 'test', { mime: 'text/plain' }),
    ).rejects.toThrow(/Node not found/);
  });

  it('readContent fails on node without content', async () => {
    await expect(
      readContent(graph, 'doc:readme'),
    ).rejects.toThrow(/No content attached/);
  });

  it('getContentMeta fails on non-existent node', async () => {
    await expect(
      getContentMeta(graph, 'doc:nonexistent'),
    ).rejects.toThrow(/Node not found/);
  });

  it('deleteContent fails on non-existent node', async () => {
    await expect(
      deleteContent(graph, 'doc:nonexistent'),
    ).rejects.toThrow(/Node not found/);
  });

  it('overwrite replaces content cleanly', async () => {
    await writeContent(graph, 'doc:readme', 'version 1', { mime: 'text/plain' });
    await writeContent(graph, 'doc:readme', 'version 2', { mime: 'text/markdown' });

    const { content, meta } = await readContent(graph, 'doc:readme');
    expect(content).toBe('version 2');
    expect(meta.mime).toBe('text/markdown');
  });

  it('handles Buffer input', async () => {
    const buf = Buffer.from('binary-safe content', 'utf-8');
    await writeContent(graph, 'doc:readme', buf, { mime: 'application/octet-stream' });

    const { content } = await readContent(graph, 'doc:readme');
    expect(content).toBe('binary-safe content');
  });

  it('stores empty string content', async () => {
    const result = await writeContent(graph, 'doc:readme', '', { mime: 'text/plain' });

    expect(result.size).toBe(0);
    const meta = await getContentMeta(graph, 'doc:readme');
    expect(meta.size).toBe(0);

    const { content } = await readContent(graph, 'doc:readme');
    expect(content).toBe('');
  });
});

describe('content CLI commands', () => {
  let tempDir;

  beforeEach(async () => {
    tempDir = await setupGitRepo();

    // Init graph and add a node
    runCli(['init'], tempDir);
    const graph = await initGraph(tempDir);
    const patch = await graph.createPatch();
    patch.addNode('doc:test');
    patch.setProperty('doc:test', 'title', 'Test Document');
    await patch.commit();

    // Create a test file to attach
    await writeFile(join(tempDir, 'test.md'), '# Test\n\nHello world.\n');
    await writeFile(join(tempDir, 'data.json'), '{"key": "value"}');
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('content set --from writes and reports success', () => {
    const output = runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md')], tempDir);
    expect(output).toContain('Content attached to doc:test');
  });

  it('content set --json outputs valid JSON', () => {
    const result = runCliJson(
      ['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md'), '--json'],
      tempDir,
    );
    expect(result.command).toBe('content-set');
    expect(result.nodeId).toBe('doc:test');
    expect(result.sha).toMatch(/^[0-9a-f]{40,64}$/);
    expect(result.mime).toBe('text/markdown');
  });

  it('content set detects MIME from file extension', () => {
    const result = runCliJson(
      ['content', 'set', 'doc:test', '--from', join(tempDir, 'data.json'), '--json'],
      tempDir,
    );
    expect(result.mime).toBe('application/json');
  });

  it('content set --mime overrides detection', () => {
    const result = runCliJson(
      ['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md'), '--mime', 'text/plain', '--json'],
      tempDir,
    );
    expect(result.mime).toBe('text/plain');
  });

  it('content show retrieves correct content', () => {
    runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md')], tempDir);
    const output = runCli(['content', 'show', 'doc:test', '--raw'], tempDir);
    expect(output).toBe('# Test\n\nHello world.\n');
  });

  it('content show --json outputs full payload', () => {
    runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md')], tempDir);
    const result = runCliJson(['content', 'show', 'doc:test', '--json'], tempDir);
    expect(result.command).toBe('content-show');
    expect(result.content).toBe('# Test\n\nHello world.\n');
    expect(result.sha).toMatch(/^[0-9a-f]{40,64}$/);
  });

  it('content meta --json returns metadata', () => {
    runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md')], tempDir);
    const result = runCliJson(['content', 'meta', 'doc:test', '--json'], tempDir);
    expect(result.command).toBe('content-meta');
    expect(result.hasContent).toBe(true);
    expect(result.mime).toBe('text/markdown');
  });

  it('content meta --json for node without content', () => {
    const result = runCliJson(['content', 'meta', 'doc:test', '--json'], tempDir);
    expect(result.hasContent).toBe(false);
    expect(result.sha).toBeUndefined();
  });

  it('content delete removes content', () => {
    runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'test.md')], tempDir);
    const output = runCli(['content', 'delete', 'doc:test'], tempDir);
    expect(output).toContain('Content removed from doc:test');

    // Verify gone
    const meta = runCliJson(['content', 'meta', 'doc:test', '--json'], tempDir);
    expect(meta.hasContent).toBe(false);
  });

  it('content delete on node without content', () => {
    const output = runCli(['content', 'delete', 'doc:test'], tempDir);
    expect(output).toContain('No content to remove');
  });

  it('content set --from nonexistent file throws with file error', () => {
    expect(() => {
      runCli(['content', 'set', 'doc:test', '--from', join(tempDir, 'nonexistent.md')], tempDir);
    }).toThrow(/ENOENT|no such file/i);
  });

  it('content show on node without content throws with no-content error', () => {
    expect(() => {
      runCli(['content', 'show', 'doc:test'], tempDir);
    }).toThrow(/No content attached/);
  });

  it('content show on non-existent node throws with not-found error', () => {
    expect(() => {
      runCli(['content', 'show', 'doc:nonexistent'], tempDir);
    }).toThrow(/Node not found/);
  });

  it('content delete on non-existent node throws with not-found error', () => {
    expect(() => {
      runCli(['content', 'delete', 'doc:nonexistent'], tempDir);
    }).toThrow(/Node not found/);
  });
});

describe('content CLI schema contracts', () => {
  let tempDir;
  let validateSet, validateShow, validateMeta, validateDelete;

  beforeAll(async () => {
    const ajv = new Ajv({ strict: true, allErrors: true });
    validateSet = ajv.compile(await loadSchema('content-set.schema.json'));
    validateShow = ajv.compile(await loadSchema('content-show.schema.json'));
    validateMeta = ajv.compile(await loadSchema('content-meta.schema.json'));
    validateDelete = ajv.compile(await loadSchema('content-delete.schema.json'));
  });

  beforeEach(async () => {
    tempDir = await setupGitRepo();

    runCli(['init'], tempDir);
    const graph = await initGraph(tempDir);
    const patch = await graph.createPatch();
    patch.addNode('doc:schema-test');
    patch.setProperty('doc:schema-test', 'title', 'Schema Test');
    await patch.commit();

    await writeFile(join(tempDir, 'test.md'), '# Schema Test\n');
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('content set --json validates against content-set.schema.json', () => {
    const result = runCliJson(
      ['content', 'set', 'doc:schema-test', '--from', join(tempDir, 'test.md'), '--json'],
      tempDir,
    );
    expect(validateSet(result), JSON.stringify(validateSet.errors)).toBe(true);
  });

  it('content show --json validates against content-show.schema.json', () => {
    runCli(['content', 'set', 'doc:schema-test', '--from', join(tempDir, 'test.md')], tempDir);
    const result = runCliJson(['content', 'show', 'doc:schema-test', '--json'], tempDir);
    expect(validateShow(result), JSON.stringify(validateShow.errors)).toBe(true);
  });

  it('content meta --json (with content) validates against content-meta.schema.json', () => {
    runCli(['content', 'set', 'doc:schema-test', '--from', join(tempDir, 'test.md')], tempDir);
    const result = runCliJson(['content', 'meta', 'doc:schema-test', '--json'], tempDir);
    expect(validateMeta(result), JSON.stringify(validateMeta.errors)).toBe(true);
    expect(result.hasContent).toBe(true);
    expect(result.sha).toBeDefined();
    expect(result.mime).toBeDefined();
  });

  it('content meta --json (no content) validates against content-meta.schema.json', () => {
    const result = runCliJson(['content', 'meta', 'doc:schema-test', '--json'], tempDir);
    expect(validateMeta(result), JSON.stringify(validateMeta.errors)).toBe(true);
    expect(result.hasContent).toBe(false);
  });

  it('content delete --json validates against content-delete.schema.json', () => {
    runCli(['content', 'set', 'doc:schema-test', '--from', join(tempDir, 'test.md')], tempDir);
    const result = runCliJson(['content', 'delete', 'doc:schema-test', '--json'], tempDir);
    expect(validateDelete(result), JSON.stringify(validateDelete.errors)).toBe(true);
    expect(result.removed).toBe(true);
    expect(result.previousSha).toMatch(/^[0-9a-f]{40,64}$/);
  });

  it('content delete --json (no content) validates against content-delete.schema.json', () => {
    const result = runCliJson(['content', 'delete', 'doc:schema-test', '--json'], tempDir);
    expect(validateDelete(result), JSON.stringify(validateDelete.errors)).toBe(true);
    expect(result.removed).toBe(false);
    expect(result.previousSha).toBeNull();
  });
});
