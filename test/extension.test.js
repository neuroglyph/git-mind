/**
 * @module test/extension
 * Unit tests for the extension runtime (src/extension.js).
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, writeFile, mkdir } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { fileURLToPath } from 'node:url';
import {
  loadExtension,
  registerExtension,
  listExtensions,
  getExtension,
  validateExtension,
  resetExtensions,
  captureBuiltIns,
  registerBuiltinExtensions,
  _resetBuiltInsForTest,
} from '../src/extension.js';
import { listLenses } from '../src/lens.js';
import { listViews, resetViews } from '../src/views.js';

const VALID_YAML = `
name: test-ext
version: 1.0.0
description: A test extension

domain:
  prefixes: [widget, gadget]
  edgeTypes: [uses]
  statusValues: [active, deprecated]

views:
  - name: widgets
    description: Widget nodes
    prefixes: [widget]

lenses: [incomplete, frontier]
`.trimStart();

const VALID_JSON = JSON.stringify({
  name: 'test-json',
  version: '2.0.0',
  domain: { prefixes: ['item'] },
});

const BAD_SCHEMA_YAML = `
name: 123
version: not-semver
domain: {}
`.trimStart();

const MISSING_REQUIRED_YAML = `
name: no-domain
version: 1.0.0
`.trimStart();

let tempDir;

beforeEach(async () => {
  tempDir = await mkdtemp(join(tmpdir(), 'gitmind-ext-'));
});

afterEach(async () => {
  _resetBuiltInsForTest();
  resetExtensions();
  resetViews();
  await rm(tempDir, { recursive: true, force: true });
});

// ── loadExtension ─────────────────────────────────────────────────

describe('loadExtension', () => {
  it('loads a valid YAML manifest', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    expect(record.name).toBe('test-ext');
    expect(record.version).toBe('1.0.0');
    expect(record.domain.prefixes).toContain('widget');
    expect(record.manifestPath).toBe(path);
    expect(record.builtin).toBe(false);
  });

  it('loads a valid JSON manifest', async () => {
    const path = join(tempDir, 'extension.json');
    await writeFile(path, VALID_JSON);
    const record = await loadExtension(path);
    expect(record.name).toBe('test-json');
    expect(record.version).toBe('2.0.0');
  });

  it('throws on missing file', async () => {
    await expect(loadExtension(join(tempDir, 'nope.yaml')))
      .rejects.toThrow('not found');
  });

  it('throws on malformed YAML', async () => {
    const path = join(tempDir, 'bad.yaml');
    await writeFile(path, 'name: [unclosed');
    await expect(loadExtension(path))
      .rejects.toThrow('parse');
  });

  it('throws on schema violation (bad name type)', async () => {
    const path = join(tempDir, 'bad-schema.yaml');
    await writeFile(path, BAD_SCHEMA_YAML);
    await expect(loadExtension(path))
      .rejects.toThrow('schema violation');
  });

  it('throws when required domain field is missing', async () => {
    const path = join(tempDir, 'no-domain.yaml');
    await writeFile(path, MISSING_REQUIRED_YAML);
    await expect(loadExtension(path))
      .rejects.toThrow('schema violation');
  });

  it('normalizes optional arrays to empty arrays when absent', async () => {
    const minimal = `name: minimal\nversion: 1.0.0\ndomain:\n  prefixes: [x]\n`;
    const path = join(tempDir, 'minimal.yaml');
    await writeFile(path, minimal);
    const record = await loadExtension(path);
    expect(Array.isArray(record.views)).toBe(true);
    expect(record.views).toHaveLength(0);
    expect(Array.isArray(record.lenses)).toBe(true);
    expect(record.lenses).toHaveLength(0);
    expect(Array.isArray(record.rules)).toBe(true);
    expect(Array.isArray(record.syncAdapters)).toBe(true);
    expect(Array.isArray(record.materializers)).toBe(true);
  });
});

// ── registerExtension ─────────────────────────────────────────────

describe('registerExtension', () => {
  it('registers an extension and makes it findable', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    registerExtension(record);
    expect(getExtension('test-ext')).toBeDefined();
    expect(listExtensions().some(e => e.name === 'test-ext')).toBe(true);
  });

  it('calls declareView for each view in the manifest', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    const viewsBefore = listViews().length;
    registerExtension(record);
    expect(listViews()).toContain('widgets');
    expect(listViews().length).toBeGreaterThan(viewsBefore);
  });

  it('throws on unknown lens reference', async () => {
    const yaml = `name: bad-lens\nversion: 1.0.0\ndomain:\n  prefixes: [x]\nlenses: [nonexistent-lens]\n`;
    const path = join(tempDir, 'bad-lens.yaml');
    await writeFile(path, yaml);
    const record = await loadExtension(path);
    expect(() => registerExtension(record))
      .toThrow('unknown lens');
  });

  it('skips declareView when builtin=true', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    record.builtin = true;
    const viewsBefore = listViews().length;
    registerExtension(record);
    // No new views declared, but extension is registered
    expect(listViews().length).toBe(viewsBefore);
    expect(getExtension('test-ext')).toBeDefined();
  });

  it('is idempotent — re-registering same name overwrites', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    registerExtension(record);
    registerExtension(record); // second call
    const matches = listExtensions().filter(e => e.name === 'test-ext');
    expect(matches).toHaveLength(1);
  });

  it('throws on prefix collision with another extension', async () => {
    const yamlA = `name: ext-a\nversion: 1.0.0\ndomain:\n  prefixes: [widget]\n`;
    const yamlB = `name: ext-b\nversion: 1.0.0\ndomain:\n  prefixes: [widget]\n`;
    const pathA = join(tempDir, 'ext-a.yaml');
    const pathB = join(tempDir, 'ext-b.yaml');
    await writeFile(pathA, yamlA);
    await writeFile(pathB, yamlB);
    const recA = await loadExtension(pathA);
    const recB = await loadExtension(pathB);
    registerExtension(recA);
    expect(() => registerExtension(recB)).toThrow(/prefix.*widget.*already owned by.*ext-a/i);
  });

  it('allows disjoint prefixes without collision', async () => {
    const yamlA = `name: ext-a\nversion: 1.0.0\ndomain:\n  prefixes: [alpha]\n`;
    const yamlB = `name: ext-b\nversion: 1.0.0\ndomain:\n  prefixes: [beta]\n`;
    const pathA = join(tempDir, 'ext-a.yaml');
    const pathB = join(tempDir, 'ext-b.yaml');
    await writeFile(pathA, yamlA);
    await writeFile(pathB, yamlB);
    const recA = await loadExtension(pathA);
    const recB = await loadExtension(pathB);
    registerExtension(recA);
    expect(() => registerExtension(recB)).not.toThrow();
  });

  it('built-in roadmap + architecture have no prefix collisions', async () => {
    await registerBuiltinExtensions();
    const exts = listExtensions();
    expect(exts.length).toBeGreaterThanOrEqual(2);
    // If we got here without throw, no collisions
  });
});

// ── validateExtension ──────────────────────────────────────────────

describe('validateExtension', () => {
  it('returns valid=true for a correct manifest', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const result = await validateExtension(path);
    expect(result.valid).toBe(true);
    expect(result.errors).toHaveLength(0);
    expect(result.record).toBeDefined();
    expect(result.record.name).toBe('test-ext');
  });

  it('returns valid=false with errors for invalid manifest', async () => {
    const path = join(tempDir, 'bad.yaml');
    await writeFile(path, BAD_SCHEMA_YAML);
    const result = await validateExtension(path);
    expect(result.valid).toBe(false);
    expect(result.errors.length).toBeGreaterThan(0);
    expect(result.record).toBeNull();
  });

  it('does not register the extension (no side effects)', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    await validateExtension(path);
    expect(getExtension('test-ext')).toBeUndefined();
  });

  it('returns errors for missing file', async () => {
    const result = await validateExtension(join(tempDir, 'missing.yaml'));
    expect(result.valid).toBe(false);
    expect(result.errors.length).toBeGreaterThan(0);
  });
});

// ── Registry ops ───────────────────────────────────────────────────

describe('listExtensions / getExtension', () => {
  it('getExtension returns undefined for unknown name', () => {
    expect(getExtension('no-such-extension')).toBeUndefined();
  });

  it('listExtensions returns empty array when nothing registered', () => {
    // resetExtensions() was called in afterEach, no builtins captured
    expect(listExtensions()).toEqual([]);
  });
});

// ── resetExtensions / captureBuiltIns ──────────────────────────────

describe('resetExtensions / captureBuiltIns', () => {
  it('resetExtensions removes non-builtins', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    registerExtension(record);
    expect(getExtension('test-ext')).toBeDefined();

    resetExtensions();
    expect(getExtension('test-ext')).toBeUndefined();
  });

  it('captureBuiltIns makes registered extensions survive reset', async () => {
    const path = join(tempDir, 'extension.yaml');
    await writeFile(path, VALID_YAML);
    const record = await loadExtension(path);
    record.builtin = true;
    registerExtension(record);
    captureBuiltIns();

    // Register another non-builtin
    const path2 = join(tempDir, 'ext2.yaml');
    await writeFile(path2, `name: ephemeral\nversion: 1.0.0\ndomain:\n  prefixes: [x]\n`);
    const record2 = await loadExtension(path2);
    registerExtension(record2);

    expect(getExtension('ephemeral')).toBeDefined();
    resetExtensions();
    expect(getExtension('ephemeral')).toBeUndefined();
    expect(getExtension('test-ext')).toBeDefined(); // survived reset
  });
});

// ── registerBuiltinExtensions memoization ──────────────────────────

describe('registerBuiltinExtensions memoization', () => {
  it('calling twice does not create duplicate registrations', async () => {
    await registerBuiltinExtensions();
    const countAfterFirst = listExtensions().length;
    await registerBuiltinExtensions();
    expect(listExtensions().length).toBe(countAfterFirst);
  });

  it('_resetBuiltInsForTest allows re-loading', async () => {
    await registerBuiltinExtensions();
    expect(listExtensions().length).toBeGreaterThan(0);
    _resetBuiltInsForTest();
    resetExtensions();
    expect(listExtensions()).toHaveLength(0);
    await registerBuiltinExtensions();
    expect(listExtensions().length).toBeGreaterThan(0);
  });
});
