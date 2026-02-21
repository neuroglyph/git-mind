/**
 * @module test/extension.integration
 * Integration tests for built-in extension manifests.
 * Reads actual files in extensions/ and validates them against the schema.
 */

import { describe, it, expect, beforeAll, beforeEach, afterEach } from 'vitest';
import { fileURLToPath } from 'node:url';
import { join } from 'node:path';
import {
  loadExtension,
  registerExtension,
  validateExtension,
  resetExtensions,
  registerBuiltinExtensions,
  listExtensions,
  getExtension,
  _resetBuiltInsForTest,
} from '../src/extension.js';
import { listLenses } from '../src/lens.js';
import { listViews, resetViews } from '../src/views.js';

const ROOT = fileURLToPath(new URL('..', import.meta.url));
const ROADMAP_MANIFEST = join(ROOT, 'extensions', 'roadmap', 'extension.yaml');
const ARCHITECTURE_MANIFEST = join(ROOT, 'extensions', 'architecture', 'extension.yaml');

afterEach(() => {
  _resetBuiltInsForTest();
  resetExtensions();
  resetViews();
});

// ── Schema validation ─────────────────────────────────────────────

describe('built-in manifests pass schema validation', () => {
  it('roadmap manifest validates cleanly', async () => {
    const result = await validateExtension(ROADMAP_MANIFEST);
    expect(result.valid).toBe(true);
    expect(result.errors).toHaveLength(0);
    expect(result.record).not.toBeNull();
  });

  it('architecture manifest validates cleanly', async () => {
    const result = await validateExtension(ARCHITECTURE_MANIFEST);
    expect(result.valid).toBe(true);
    expect(result.errors).toHaveLength(0);
    expect(result.record).not.toBeNull();
  });
});

// ── Manifest content ──────────────────────────────────────────────

describe('roadmap manifest content', () => {
  let record;
  beforeAll(async () => { record = (await validateExtension(ROADMAP_MANIFEST)).record; });

  it('has correct name and version', () => {
    expect(record.name).toBe('roadmap');
    expect(record.version).toBe('1.0.0');
  });

  it('declares expected domain prefixes', () => {
    expect(record.domain.prefixes).toContain('phase');
    expect(record.domain.prefixes).toContain('milestone');
    expect(record.domain.prefixes).toContain('task');
    expect(record.domain.prefixes).toContain('feature');
  });

  it('declares roadmap, backlog, milestone, and progress views', () => {
    const names = record.views.map(v => v.name);
    expect(names).toContain('roadmap');
    expect(names).toContain('backlog');
    expect(names).toContain('milestone');
    expect(names).toContain('progress');
  });

  it('references only built-in lenses', () => {
    const available = listLenses();
    for (const lens of record.lenses) {
      expect(available).toContain(lens);
    }
  });
});

describe('architecture manifest content', () => {
  let record;
  beforeAll(async () => { record = (await validateExtension(ARCHITECTURE_MANIFEST)).record; });

  it('has correct name and version', () => {
    expect(record.name).toBe('architecture');
    expect(record.version).toBe('1.0.0');
  });

  it('declares expected domain prefixes', () => {
    expect(record.domain.prefixes).toContain('crate');
    expect(record.domain.prefixes).toContain('module');
    expect(record.domain.prefixes).toContain('pkg');
  });

  it('declares architecture, traceability, coverage, and onboarding views', () => {
    const names = record.views.map(v => v.name);
    expect(names).toContain('architecture');
    expect(names).toContain('traceability');
    expect(names).toContain('coverage');
    expect(names).toContain('onboarding');
  });

  it('references only built-in lenses', () => {
    const available = listLenses();
    for (const lens of record.lenses) {
      expect(available).toContain(lens);
    }
  });
});

// ── Registration ──────────────────────────────────────────────────

describe('registering built-in manifests in non-builtin mode', () => {
  it('registers roadmap and declares its views without error', async () => {
    const record = await loadExtension(ROADMAP_MANIFEST);
    expect(() => registerExtension(record)).not.toThrow();
  });

  it('roadmap views are declared after registration', async () => {
    const record = await loadExtension(ROADMAP_MANIFEST);
    registerExtension(record);
    expect(listViews()).toContain('roadmap');
    expect(listViews()).toContain('backlog');
  });

  it('architecture view is declared after registration', async () => {
    const record = await loadExtension(ARCHITECTURE_MANIFEST);
    registerExtension(record);
    expect(listViews()).toContain('architecture');
  });
});

// ── registerBuiltinExtensions ─────────────────────────────────────

describe('registerBuiltinExtensions', () => {
  it('loads both built-in extensions', async () => {
    await registerBuiltinExtensions();
    const exts = listExtensions();
    const names = exts.map(e => e.name);
    expect(names).toContain('roadmap');
    expect(names).toContain('architecture');
  });

  it('built-in extensions have builtin=true', async () => {
    await registerBuiltinExtensions();
    expect(getExtension('roadmap').builtin).toBe(true);
    expect(getExtension('architecture').builtin).toBe(true);
  });

  it('built-in extensions survive resetExtensions()', async () => {
    await registerBuiltinExtensions();
    resetExtensions();
    expect(getExtension('roadmap')).toBeDefined();
    expect(getExtension('architecture')).toBeDefined();
  });

  it('built-in views are available (pre-registered by views.js)', async () => {
    await registerBuiltinExtensions();
    const views = listViews();
    expect(views).toContain('roadmap');
    expect(views).toContain('backlog');
    expect(views).toContain('architecture');
  });
});
