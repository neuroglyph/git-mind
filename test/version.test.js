/**
 * @module test/version
 * Tests for version module and --version CLI flag.
 */

import { describe, it, expect } from 'vitest';
import { execFileSync } from 'node:child_process';
import { join } from 'node:path';
import { VERSION, NAME } from '../src/version.js';

const BIN = join(import.meta.dirname, '..', 'bin', 'git-mind.js');

describe('version module', () => {
  it('VERSION is a valid semver string', () => {
    expect(VERSION).toMatch(/^\d+\.\d+\.\d+/);
  });

  it('NAME is the scoped package name', () => {
    expect(NAME).toBe('@neuroglyph/git-mind');
  });
});

describe('--version CLI flag', () => {
  it('prints version and exits with --version', () => {
    const out = execFileSync(process.execPath, [BIN, '--version'], {
      encoding: 'utf-8',
    }).trim();
    expect(out).toBe(VERSION);
  });

  it('prints version and exits with -v', () => {
    const out = execFileSync(process.execPath, [BIN, '-v'], {
      encoding: 'utf-8',
    }).trim();
    expect(out).toBe(VERSION);
  });
});
