/**
 * @module test/error-propagation
 * Tests that GmindError exit codes and --json envelopes propagate correctly
 * through the CLI boundary (#207).
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execFileSync, spawnSync } from 'node:child_process';

const BIN = join(import.meta.dirname, '..', 'bin', 'git-mind.js');

/** Run CLI and return { status, stdout, stderr }. Does NOT throw on failure. */
function runCli(args, cwd) {
  const result = spawnSync(process.execPath, [BIN, ...args], {
    cwd,
    encoding: 'utf-8',
    timeout: 30_000,
    env: { ...process.env, NO_COLOR: '1' },
  });
  return {
    status: result.status,
    stdout: result.stdout,
    stderr: result.stderr,
  };
}

async function setupGitRepo() {
  const dir = await mkdtemp(join(tmpdir(), 'gitmind-errprop-'));
  execFileSync('git', ['init'], { cwd: dir, stdio: 'ignore' });
  execFileSync('git', ['config', 'user.email', 'test@test.com'], { cwd: dir, stdio: 'ignore' });
  execFileSync('git', ['config', 'user.name', 'Test'], { cwd: dir, stdio: 'ignore' });
  return dir;
}

describe('unknown command --json error envelope (#207)', () => {
  let tempDir;

  beforeEach(async () => {
    tempDir = await setupGitRepo();
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('git mind <bad-cmd> --json outputs a structured error envelope', () => {
    const { status, stdout } = runCli(['boguscmd', '--json'], tempDir);
    expect(status).toBe(2); // ExitCode.USAGE
    const parsed = JSON.parse(stdout);
    expect(parsed.errorCode).toBe('GMIND_E_UNKNOWN_CMD');
    expect(parsed.exitCode).toBe(2);
    expect(parsed.error).toMatch(/Unknown command/);
    expect(parsed.schemaVersion).toBe(1);
  });

  it('git mind <bad-cmd> without --json outputs plain text to stderr', () => {
    const { status, stderr } = runCli(['boguscmd'], tempDir);
    expect(status).toBe(2); // ExitCode.USAGE
    expect(stderr).toMatch(/Unknown command/);
  });

  it('git mind content <bad-sub> --json outputs structured envelope', () => {
    const { status, stdout } = runCli(['content', 'bogussub', '--json'], tempDir);
    expect(status).toBe(2);
    const parsed = JSON.parse(stdout);
    expect(parsed.errorCode).toBe('GMIND_E_UNKNOWN_CMD');
    expect(parsed.error).toMatch(/Unknown content subcommand/);
  });

  it('git mind extension <bad-sub> --json outputs structured envelope', () => {
    const { status, stdout } = runCli(['extension', 'bogussub', '--json'], tempDir);
    expect(status).toBe(2);
    const parsed = JSON.parse(stdout);
    expect(parsed.errorCode).toBe('GMIND_E_UNKNOWN_CMD');
    expect(parsed.error).toMatch(/Unknown extension subcommand/);
  });
});

describe('resolveContext error propagation (#207)', () => {
  let tempDir;

  beforeEach(async () => {
    tempDir = await setupGitRepo();
    // Initialize a graph so init doesn't fail
    execFileSync(process.execPath, [BIN, 'init'], { cwd: tempDir, stdio: 'ignore' });
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  it('view --observer <missing> returns exit code 4 (NOT_FOUND)', () => {
    const { status, stderr } = runCli(['view', 'roadmap', '--observer', 'ghost'], tempDir);
    expect(status).toBe(4); // ExitCode.NOT_FOUND
    expect(stderr).toMatch(/Observer.*not found/);
  });

  it('view --observer <missing> --json returns structured NOT_FOUND envelope', () => {
    const { status, stdout } = runCli(['view', 'roadmap', '--observer', 'ghost', '--json'], tempDir);
    expect(status).toBe(4);
    const parsed = JSON.parse(stdout);
    expect(parsed.errorCode).toBe('GMIND_E_NOT_FOUND');
    expect(parsed.exitCode).toBe(4);
  });

  it('nodes --observer <missing> returns exit code 4', () => {
    const { status } = runCli(['nodes', '--observer', 'ghost'], tempDir);
    expect(status).toBe(4);
  });

  it('status --observer <missing> returns exit code 4', () => {
    const { status } = runCli(['status', '--observer', 'ghost'], tempDir);
    expect(status).toBe(4);
  });

  it('doctor --observer <missing> returns exit code 4', () => {
    const { status } = runCli(['doctor', '--observer', 'ghost'], tempDir);
    expect(status).toBe(4);
  });
});
