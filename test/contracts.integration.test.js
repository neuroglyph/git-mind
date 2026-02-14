/**
 * @module test/contracts.integration
 * CLI canary tests — execute the real CLI binary and validate output
 * against JSON Schema contracts.
 */

import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { mkdtemp, rm, readFile, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync, execFileSync } from 'node:child_process';
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
  return JSON.parse(stdout);
}

describe('CLI schema contract canaries', () => {
  let tempDir;
  let ajv;

  beforeAll(async () => {
    ajv = new Ajv({ strict: true, allErrors: true });

    // Create a temp repo with seeded graph data
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-contract-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.email "test@test.com"', { cwd: tempDir, stdio: 'ignore' });
    execSync('git config user.name "Test"', { cwd: tempDir, stdio: 'ignore' });

    // Initialize graph
    execFileSync(process.execPath, [BIN, 'init'], { cwd: tempDir, stdio: 'ignore' });

    // Import the echo-seed fixture
    execFileSync(process.execPath, [BIN, 'import', FIXTURE], { cwd: tempDir, stdio: 'ignore' });
  });

  afterAll(async () => {
    await rm(tempDir, { recursive: true, force: true });
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
    const knownId = listOutput.nodes[0];
    expect(knownId).toBeDefined();

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
});
