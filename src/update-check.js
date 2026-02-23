/**
 * @module update-check
 * Non-blocking update check against the npm registry.
 *
 * Design:
 * - triggerUpdateCheck() fires a fetch in the background (never awaited)
 * - getUpdateNotification() reads the PREVIOUS run's cache (sync)
 * - Cache lives at ~/.gitmind/update-check.json with 24h TTL
 * - All errors silently swallowed — must never break any command
 */

import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { homedir } from 'node:os';
import { NAME, VERSION } from './version.js';

const CACHE_DIR = join(homedir(), '.gitmind');
const CACHE_FILE = join(CACHE_DIR, 'update-check.json');
const CACHE_TTL_MS = 24 * 60 * 60 * 1000; // 24 hours
const FETCH_TIMEOUT_MS = 3000;
const REGISTRY_URL = `https://registry.npmjs.org/${NAME}/latest`;

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
 * Read cached update check result (sync).
 * Returns a notification string if a newer version is available, null otherwise.
 * @returns {string|null}
 */
export function getUpdateNotification() {
  try {
    const data = JSON.parse(readFileSync(CACHE_FILE, 'utf-8'));
    if (!data.latest || !isNewer(VERSION, data.latest)) return null;
    return formatNotification(data.latest);
  } catch {
    return null;
  }
}

/**
 * Format the update notification banner.
 * @param {string} latest
 * @returns {string}
 */
export function formatNotification(latest) {
  // Dynamic imports would be async; chalk and figures are already deps so
  // we can import them at module level, but to keep this module light for
  // the sync path, we do a simple string.
  return `\n  Update available: ${VERSION} → ${latest}\n  Run \`npm install -g ${NAME}\` to update\n`;
}

/**
 * Fire-and-forget: fetch latest version from npm registry and write cache.
 * Never throws — all errors silently swallowed.
 */
export function triggerUpdateCheck() {
  _doCheck().catch(() => {});
}

/** @internal */
async function _doCheck() {
  // Skip if cache is fresh
  try {
    const data = JSON.parse(readFileSync(CACHE_FILE, 'utf-8'));
    if (Date.now() - data.checkedAt < CACHE_TTL_MS) return;
  } catch {
    // No cache or corrupt — proceed with fetch
  }

  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);

  try {
    const res = await fetch(REGISTRY_URL, {
      signal: controller.signal,
      headers: { Accept: 'application/json' },
    });
    if (!res.ok) return;
    const body = await res.json();
    const latest = body.version;
    if (!latest) return;

    mkdirSync(CACHE_DIR, { recursive: true });
    writeFileSync(CACHE_FILE, JSON.stringify({ latest, checkedAt: Date.now() }));
  } finally {
    clearTimeout(timer);
  }
}
