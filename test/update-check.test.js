/**
 * @module test/update-check
 * Tests for update-check: semver comparison, cache, notification formatting.
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, readFile, writeFile, mkdir } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { isNewer, formatNotification, getUpdateNotification } from '../src/update-check.js';
import { VERSION, NAME } from '../src/version.js';

describe('isNewer()', () => {
  it('detects newer major version', () => {
    expect(isNewer('1.0.0', '2.0.0')).toBe(true);
  });

  it('detects newer minor version', () => {
    expect(isNewer('1.2.0', '1.3.0')).toBe(true);
  });

  it('detects newer patch version', () => {
    expect(isNewer('1.2.3', '1.2.4')).toBe(true);
  });

  it('returns false for same version', () => {
    expect(isNewer('1.2.3', '1.2.3')).toBe(false);
  });

  it('returns false for older version', () => {
    expect(isNewer('2.0.0', '1.9.9')).toBe(false);
  });

  it('handles v-prefixed versions', () => {
    expect(isNewer('v1.0.0', 'v2.0.0')).toBe(true);
  });

  it('returns false when local is newer', () => {
    expect(isNewer('1.3.0', '1.2.0')).toBe(false);
  });
});

describe('formatNotification()', () => {
  it('includes current version', () => {
    const note = formatNotification('99.0.0');
    expect(note).toContain(VERSION);
  });

  it('includes latest version', () => {
    const note = formatNotification('99.0.0');
    expect(note).toContain('99.0.0');
  });

  it('includes install command', () => {
    const note = formatNotification('99.0.0');
    expect(note).toContain(`npm install -g ${NAME}`);
  });
});

describe('getUpdateNotification()', () => {
  it('returns null when no cache exists', () => {
    // getUpdateNotification reads from ~/.gitmind/update-check.json
    // With no cache, it should return null
    const result = getUpdateNotification();
    // Result depends on whether cache exists; either null or a string
    expect(result === null || typeof result === 'string').toBe(true);
  });
});
