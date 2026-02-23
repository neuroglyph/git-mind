/**
 * @module update-check
 * Non-blocking update check against the npm registry.
 *
 * Hexagonal design:
 * - createUpdateChecker(ports) — pure domain logic, no I/O at module level
 * - Ports: fetchLatest, readCache, writeCache (injected)
 * - defaultPorts() — real adapters: fs for cache, fetch + @git-stunts/alfred for registry
 * - Tests inject fakes; CLI wires real adapters
 *
 * Flow:
 * - triggerCheck(signal?) fires a background fetch (never awaited)
 * - getNotification() reads the PREVIOUS run's cache (sync)
 * - Cache lives at ~/.gitmind/update-check.json with 24h TTL
 */

import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { homedir } from 'node:os';

const CACHE_DIR = join(homedir(), '.gitmind');
const CACHE_FILE = join(CACHE_DIR, 'update-check.json');
const DEFAULT_CACHE_TTL_MS = 24 * 60 * 60 * 1000; // 24 hours
const DEFAULT_FETCH_TIMEOUT_MS = 3000;

// ── Pure domain logic (no I/O) ──────────────────────────────────

/**
 * Compare two semver strings. Returns true if remote > local.
 * Naive comparison — handles major.minor.patch only.
 * @param {string} local
 * @param {string} remote
 * @returns {boolean}
 */
export function isNewer(local, remote) {
  const parse = (v) => v.replace(/^v/, '').split('.').map(Number);
  const l = parse(local);
  const r = parse(remote);
  for (let i = 0; i < 3; i++) {
    if ((r[i] || 0) > (l[i] || 0)) return true;
    if ((r[i] || 0) < (l[i] || 0)) return false;
  }
  return false;
}

/**
 * Format the update notification banner.
 * @param {string} latest
 * @param {string} currentVersion
 * @param {string} packageName
 * @returns {string}
 */
export function formatNotification(latest, currentVersion, packageName) {
  return `\n  Update available: ${currentVersion} → ${latest}\n  Run \`npm install -g ${packageName}\` to update\n`;
}

/**
 * @typedef {object} UpdateCheckPorts
 * @property {(signal?: AbortSignal) => Promise<string|null>} fetchLatest  Fetch latest version string from registry
 * @property {() => {latest: string, checkedAt: number}|null} readCache    Read cached check result (sync)
 * @property {(data: {latest: string, checkedAt: number}) => void} writeCache  Write check result to cache
 * @property {string} currentVersion  Current installed version
 * @property {string} packageName     Package name (for notification)
 * @property {number} [cacheTtlMs]    Cache TTL in ms (default 24h)
 */

/**
 * Create an update checker service.
 * @param {UpdateCheckPorts} ports
 * @returns {{ getNotification: () => string|null, triggerCheck: (signal?: AbortSignal) => void }}
 */
export function createUpdateChecker(ports) {
  const { fetchLatest, readCache, writeCache, currentVersion, packageName,
    cacheTtlMs = DEFAULT_CACHE_TTL_MS } = ports;

  function getNotification() {
    try {
      const data = readCache();
      if (!data?.latest || !isNewer(currentVersion, data.latest)) return null;
      return formatNotification(data.latest, currentVersion, packageName);
    } catch {
      return null;
    }
  }

  function triggerCheck(signal) {
    _doCheck(signal).catch(() => {});
  }

  async function _doCheck(signal) {
    // Skip if cache is fresh
    try {
      const data = readCache();
      if (data && Date.now() - data.checkedAt < cacheTtlMs) return;
    } catch {
      // No cache or corrupt — proceed with fetch
    }

    const latest = await fetchLatest(signal);
    if (!latest) return;
    writeCache({ latest, checkedAt: Date.now() });
  }

  return { getNotification, triggerCheck };
}

// ── Default adapters (real I/O) ─────────────────────────────────

/**
 * Build real adapters for production use.
 * Alfred is lazy-loaded only when a fetch is needed.
 * @param {string} registryUrl  npm registry URL for the package
 * @param {object} [opts]
 * @param {number} [opts.timeoutMs]  Fetch timeout (default 3000)
 * @returns {Pick<UpdateCheckPorts, 'fetchLatest' | 'readCache' | 'writeCache'>}
 */
export function defaultPorts(registryUrl, opts = {}) {
  const { timeoutMs = DEFAULT_FETCH_TIMEOUT_MS } = opts;

  /** @type {import('@git-stunts/alfred').Policy|null} */
  let policy = null;

  return {
    readCache() {
      try {
        return JSON.parse(readFileSync(CACHE_FILE, 'utf-8'));
      } catch {
        return null;
      }
    },

    writeCache(data) {
      mkdirSync(CACHE_DIR, { recursive: true });
      writeFileSync(CACHE_FILE, JSON.stringify(data));
    },

    async fetchLatest(signal) {
      // Lazy-load Alfred on first fetch
      const { Policy } = await import('@git-stunts/alfred');

      // External signal goes on retry options for cancellation;
      // timeout's internal signal is passed to fn for fetch abort.
      const registryPolicy = Policy.timeout(timeoutMs)
        .wrap(Policy.retry({
          retries: 1,
          delay: 200,
          backoff: 'exponential',
          jitter: 'decorrelated',
          signal,
        }));

      const body = await registryPolicy.execute(
        (timeoutSignal) =>
          fetch(registryUrl, {
            signal: timeoutSignal,
            headers: { Accept: 'application/json' },
          }).then((res) => {
            if (!res.ok) throw new Error(`registry ${res.status}`);
            return res.json();
          }),
      );

      return body.version || null;
    },
  };
}
