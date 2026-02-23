/**
 * @module test/update-check
 * Tests for update-check: semver comparison, cache, notification, DI service.
 *
 * All tests use injected fakes — no network, no filesystem.
 */

import { describe, it, expect } from 'vitest';
import { isNewer, formatNotification, createUpdateChecker } from '../src/update-check.js';

// ── Helpers ─────────────────────────────────────────────────────

/** Build a fake port set for createUpdateChecker. */
function fakePorts(overrides = {}) {
  return {
    currentVersion: '1.0.0',
    packageName: '@test/pkg',
    cacheTtlMs: 1000,
    readCache: () => null,
    writeCache: () => {},
    fetchLatest: async () => null,
    ...overrides,
  };
}

// ── isNewer ─────────────────────────────────────────────────────

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

// ── formatNotification ──────────────────────────────────────────

describe('formatNotification()', () => {
  it('includes current and latest versions', () => {
    const note = formatNotification('2.0.0', '1.0.0', '@test/pkg');
    expect(note).toContain('1.0.0');
    expect(note).toContain('2.0.0');
  });

  it('includes install command with package name', () => {
    const note = formatNotification('2.0.0', '1.0.0', '@test/pkg');
    expect(note).toContain('npm install -g @test/pkg');
  });
});

// ── createUpdateChecker ─────────────────────────────────────────

describe('createUpdateChecker()', () => {
  describe('getNotification()', () => {
    it('returns null when cache is empty', () => {
      const checker = createUpdateChecker(fakePorts());
      expect(checker.getNotification()).toBeNull();
    });

    it('returns null when cache has same version', () => {
      const checker = createUpdateChecker(fakePorts({
        readCache: () => ({ latest: '1.0.0', checkedAt: Date.now() }),
      }));
      expect(checker.getNotification()).toBeNull();
    });

    it('returns notification when cache has newer version', () => {
      const checker = createUpdateChecker(fakePorts({
        readCache: () => ({ latest: '2.0.0', checkedAt: Date.now() }),
      }));
      const note = checker.getNotification();
      expect(note).toContain('2.0.0');
      expect(note).toContain('npm install -g @test/pkg');
    });

    it('returns null when cache has older version', () => {
      const checker = createUpdateChecker(fakePorts({
        readCache: () => ({ latest: '0.5.0', checkedAt: Date.now() }),
      }));
      expect(checker.getNotification()).toBeNull();
    });

    it('returns null when readCache throws', () => {
      const checker = createUpdateChecker(fakePorts({
        readCache: () => { throw new Error('corrupt'); },
      }));
      expect(checker.getNotification()).toBeNull();
    });
  });

  describe('triggerCheck()', () => {
    it('fetches and writes cache when no cache exists', async () => {
      let written = null;
      const checker = createUpdateChecker(fakePorts({
        fetchLatest: async () => '2.0.0',
        writeCache: (data) => { written = data; },
      }));

      checker.triggerCheck();
      // Wait for the fire-and-forget to settle
      await new Promise((r) => setTimeout(r, 50));

      expect(written).not.toBeNull();
      expect(written.latest).toBe('2.0.0');
      expect(written.checkedAt).toBeGreaterThan(0);
    });

    it('skips fetch when cache is fresh', async () => {
      let fetched = false;
      const checker = createUpdateChecker(fakePorts({
        readCache: () => ({ latest: '1.0.0', checkedAt: Date.now() }),
        fetchLatest: async () => { fetched = true; return '2.0.0'; },
      }));

      checker.triggerCheck();
      await new Promise((r) => setTimeout(r, 50));

      expect(fetched).toBe(false);
    });

    it('fetches when cache is stale', async () => {
      let fetched = false;
      const checker = createUpdateChecker(fakePorts({
        cacheTtlMs: 1000,
        readCache: () => ({ latest: '1.0.0', checkedAt: Date.now() - 5000 }),
        fetchLatest: async () => { fetched = true; return '2.0.0'; },
      }));

      checker.triggerCheck();
      await new Promise((r) => setTimeout(r, 50));

      expect(fetched).toBe(true);
    });

    it('does not write cache when fetchLatest returns null', async () => {
      let written = false;
      const checker = createUpdateChecker(fakePorts({
        fetchLatest: async () => null,
        writeCache: () => { written = true; },
      }));

      checker.triggerCheck();
      await new Promise((r) => setTimeout(r, 50));

      expect(written).toBe(false);
    });

    it('silently swallows fetch errors', async () => {
      const checker = createUpdateChecker(fakePorts({
        fetchLatest: async () => { throw new Error('network down'); },
      }));

      // Should not throw
      checker.triggerCheck();
      await new Promise((r) => setTimeout(r, 50));
    });

    it('passes signal to fetchLatest', async () => {
      let receivedSignal = null;
      const checker = createUpdateChecker(fakePorts({
        fetchLatest: async (signal) => { receivedSignal = signal; return '2.0.0'; },
      }));

      const controller = new AbortController();
      checker.triggerCheck(controller.signal);
      await new Promise((r) => setTimeout(r, 50));

      expect(receivedSignal).toBe(controller.signal);
    });

    it('respects abort signal', async () => {
      let fetched = false;
      const checker = createUpdateChecker(fakePorts({
        fetchLatest: async (signal) => {
          // Simulate slow fetch
          await new Promise((r) => setTimeout(r, 200));
          if (signal?.aborted) throw new Error('aborted');
          fetched = true;
          return '2.0.0';
        },
      }));

      const controller = new AbortController();
      checker.triggerCheck(controller.signal);
      controller.abort();
      await new Promise((r) => setTimeout(r, 300));

      expect(fetched).toBe(false);
    });
  });
});
