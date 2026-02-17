/**
 * @module test/contracts.integration
 * CLI canary tests — execute the real CLI binary and validate output
 * against JSON Schema contracts.
 */

import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { existsSync } from 'node:fs';
import { mkdtemp, rm, readFile, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execFileSync } from 'node:child_process';
import Ajv from 'ajv/dist/2020.js';

const BIN = join(import.meta.dirname, '..', 'bin', 'git-mind.js');
const SCHEMA_DIR = join(import.meta.dirname, '..', 'docs', 'contracts', 'cli');
const FIXTURE = join(import.meta.dirname, 'fixtures', 'echo-seed.yaml');

/** Load a schema by filename. */
async function loadSchema(name) {
  return JSON.parse(await readFile(join(SCHEMA_DIR, name), 'utf-8'));
}

/** Run the CLI and return parsed JSON output. */
function runCli(args, cwd) {
  const stdout = execFileSync(process.execPath, [BIN, ...args], {
    cwd,
    encoding: 'utf-8',
    timeout: 30_000,
    env: { ...process.env, NO_COLOR: '1' },
  });
  try {
    return JSON.parse(stdout);
  } catch (err) {
    throw new Error(`Failed to parse CLI output for [${args.join(' ')}]:\n${stdout}`, { cause: err });
  }
}

/** Create a git commit in a repo and return the commit SHA. */
function gitCommit(cwd, filename, message) {
  execFileSync('git', ['add', filename], { cwd, stdio: 'ignore' });
  execFileSync('git', ['commit', '-m', message], { cwd, stdio: 'ignore' });
  return execFileSync('git', ['rev-parse', 'HEAD'], { cwd, encoding: 'utf-8' }).trim();
}

describe('CLI schema contract canaries', () => {
  let tempDir;
  let mergeDir;
  let ajv;

  beforeAll(async () => {
    ajv = new Ajv({ strict: true, allErrors: true });

    // Create a temp repo with seeded graph data
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-contract-'));
    execFileSync('git', ['init'], { cwd: tempDir, stdio: 'ignore' });
    execFileSync('git', ['config', 'user.email', 'test@test.com'], { cwd: tempDir, stdio: 'ignore' });
    execFileSync('git', ['config', 'user.name', 'Test'], { cwd: tempDir, stdio: 'ignore' });

    // Initialize graph
    execFileSync(process.execPath, [BIN, 'init'], { cwd: tempDir, stdio: 'ignore' });

    // Import the echo-seed fixture
    if (!existsSync(FIXTURE)) {
      throw new Error(`Fixture not found: ${FIXTURE}`);
    }
    execFileSync(process.execPath, [BIN, 'import', FIXTURE], { cwd: tempDir, stdio: 'ignore' });

    // Create first commit + epoch marker (needed for at/diff tests)
    await writeFile(join(tempDir, 'marker-1.txt'), 'epoch-1');
    const sha1 = gitCommit(tempDir, 'marker-1.txt', 'first epoch commit');
    execFileSync(process.execPath, [BIN, 'process-commit', sha1], { cwd: tempDir, stdio: 'ignore' });

    // Add a new edge so the diff has something to show
    execFileSync(process.execPath, [BIN, 'link', 'task:echo-app', 'spec:api-layer', '--type', 'implements'], { cwd: tempDir, stdio: 'ignore' });

    // Create second commit + epoch marker
    await writeFile(join(tempDir, 'marker-2.txt'), 'epoch-2');
    const sha2 = gitCommit(tempDir, 'marker-2.txt', 'second epoch commit');
    execFileSync(process.execPath, [BIN, 'process-commit', sha2], { cwd: tempDir, stdio: 'ignore' });

    // Create a second repo for merge testing
    mergeDir = await mkdtemp(join(tmpdir(), 'gitmind-merge-'));
    execFileSync('git', ['init'], { cwd: mergeDir, stdio: 'ignore' });
    execFileSync('git', ['config', 'user.email', 'test@test.com'], { cwd: mergeDir, stdio: 'ignore' });
    execFileSync('git', ['config', 'user.name', 'Test'], { cwd: mergeDir, stdio: 'ignore' });
    execFileSync(process.execPath, [BIN, 'init'], { cwd: mergeDir, stdio: 'ignore' });
    execFileSync(process.execPath, [BIN, 'import', FIXTURE], { cwd: mergeDir, stdio: 'ignore' });
  }, 30_000);

  afterAll(async () => {
    if (tempDir) await rm(tempDir, { recursive: true, force: true });
    if (mergeDir) await rm(mergeDir, { recursive: true, force: true });
  });

  it('status --json validates against status.schema.json', async () => {
    const schema = await loadSchema('status.schema.json');
    const output = runCli(['status', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('status');

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('nodes --json validates against node-list.schema.json', async () => {
    const schema = await loadSchema('node-list.schema.json');
    const output = runCli(['nodes', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('nodes');
    expect(Array.isArray(output.nodes)).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('nodes --id <known> --json validates against node-detail.schema.json', async () => {
    // Get a known node from the list first
    const listOutput = runCli(['nodes', '--json'], tempDir);
    expect(listOutput.nodes.length, 'seed fixture should produce at least one node — check echo-seed.yaml import').toBeGreaterThan(0);
    const knownId = listOutput.nodes[0];

    const schema = await loadSchema('node-detail.schema.json');
    const output = runCli(['nodes', '--id', knownId, '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('nodes');
    expect(output.id).toBe(knownId);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('doctor --json validates against doctor.schema.json', async () => {
    const schema = await loadSchema('doctor.schema.json');
    const output = runCli(['doctor', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('doctor');

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('export --json validates against export-data.schema.json', async () => {
    const schema = await loadSchema('export-data.schema.json');
    const output = runCli(['export', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('export');
    expect(output.version).toBe(1);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('export --file <path> --json validates against export-file.schema.json', async () => {
    const schema = await loadSchema('export-file.schema.json');
    const outPath = join(tempDir, 'export-test.yaml');
    const output = runCli(['export', '--file', outPath, '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('export');
    expect(output.path).toBe(outPath);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('import --dry-run --json validates against import.schema.json', async () => {
    const schema = await loadSchema('import.schema.json');
    const output = runCli(['import', '--dry-run', FIXTURE, '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('import');
    expect(output.valid).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('review --json validates against review-list.schema.json', async () => {
    const schema = await loadSchema('review-list.schema.json');
    const output = runCli(['review', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('review');
    expect(Array.isArray(output.pending)).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('at HEAD --json validates against at.schema.json', async () => {
    const schema = await loadSchema('at.schema.json');
    const output = runCli(['at', 'HEAD', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('at');
    expect(output.tick).toBeGreaterThanOrEqual(0);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('diff HEAD~1..HEAD --json validates against diff.schema.json', async () => {
    const schema = await loadSchema('diff.schema.json');
    const output = runCli(['diff', 'HEAD~1..HEAD', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('diff');
    expect(output.from).toBeDefined();
    expect(output.to).toBeDefined();

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  // NOTE: mutates review state (accepts all pending) — keep after review --json test
  it('review --batch accept --json validates against review-batch.schema.json', async () => {
    const schema = await loadSchema('review-batch.schema.json');
    const output = runCli(['review', '--batch', 'accept', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('review');
    expect(typeof output.processed).toBe('number');
    expect(Array.isArray(output.decisions)).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('merge --dry-run --json validates against merge.schema.json', async () => {
    const schema = await loadSchema('merge.schema.json');
    const output = runCli(['merge', '--from', mergeDir, '--repo-name', 'test/merge-repo', '--dry-run', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('merge');
    expect(output.dryRun).toBe(true);
    expect(output.repoName).toBe('test/merge-repo');

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('set --json validates against set.schema.json', async () => {
    // First, find a known node to set a property on
    const listOutput = runCli(['nodes', '--json'], tempDir);
    expect(listOutput.nodes.length).toBeGreaterThan(0);
    const knownId = listOutput.nodes[0];

    const schema = await loadSchema('set.schema.json');
    const output = runCli(['set', knownId, 'status', 'done', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('set');
    expect(output.id).toBe(knownId);
    expect(output.key).toBe('status');
    expect(output.value).toBe('done');
    expect(typeof output.changed).toBe('boolean');

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('set --json returns changed: false on idempotent re-set', async () => {
    const listOutput = runCli(['nodes', '--json'], tempDir);
    const knownId = listOutput.nodes[0];

    // First set (may or may not change depending on previous test)
    runCli(['set', knownId, 'status', 'done', '--json'], tempDir);
    // Second set — should be idempotent
    const output = runCli(['set', knownId, 'status', 'done', '--json'], tempDir);

    expect(output.changed).toBe(false);
    expect(output.previous).toBe('done');
  });

  it('unset --json validates against unset.schema.json', async () => {
    const listOutput = runCli(['nodes', '--json'], tempDir);
    const knownId = listOutput.nodes[0];

    // Ensure the property is set first
    runCli(['set', knownId, 'test-key', 'test-value', '--json'], tempDir);

    const schema = await loadSchema('unset.schema.json');
    const output = runCli(['unset', knownId, 'test-key', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('unset');
    expect(output.id).toBe(knownId);
    expect(output.key).toBe('test-key');
    expect(output.previous).toBe('test-value');
    expect(output.removed).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('view progress --json validates against view-progress.schema.json', async () => {
    // Set a status on a known node so progress has data
    const listOutput = runCli(['nodes', '--json'], tempDir);
    const taskNode = listOutput.nodes.find(n => n.startsWith('task:'));
    expect(taskNode, 'seed fixture must include at least one task: node').toBeDefined();
    runCli(['set', taskNode, 'status', 'done', '--json'], tempDir);

    const schema = await loadSchema('view-progress.schema.json');
    const output = runCli(['view', 'progress', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('view');
    expect(output.viewName).toBe('progress');
    expect(output.meta.summary.ratio).toMatch(/^\d+\/\d+$/);
    expect(typeof output.meta.summary.remaining).toBe('number');

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  it('view backlog:blocked --json validates against view-lens.schema.json', async () => {
    const schema = await loadSchema('view-lens.schema.json');
    const output = runCli(['view', 'backlog:blocked', '--json'], tempDir);

    expect(output.schemaVersion).toBe(1);
    expect(output.command).toBe('view');
    expect(output.viewName).toBe('backlog');
    expect(output.lenses).toEqual(['blocked']);
    expect(Array.isArray(output.nodes)).toBe(true);
    expect(Array.isArray(output.edges)).toBe(true);

    const validate = ajv.compile(schema);
    expect(validate(output), JSON.stringify(validate.errors)).toBe(true);
  });

  // Note: suggest --json is not tested here because it requires a configured
  // GITMIND_AGENT (LLM command) which is unavailable in CI/test environments.
});
