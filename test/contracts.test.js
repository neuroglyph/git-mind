/**
 * @module test/contracts
 * Schema contract unit tests — validates JSON Schema files compile,
 * require envelope fields, and accept/reject expected payloads.
 */

import { describe, it, expect, beforeAll } from 'vitest';
import { readdir, readFile } from 'node:fs/promises';
import { join, basename } from 'node:path';
import Ajv from 'ajv/dist/2020.js';

const SCHEMA_DIR = join(import.meta.dirname, '..', 'docs', 'contracts', 'cli');

/** Load all schema files from the contracts directory. */
async function loadSchemas() {
  const files = await readdir(SCHEMA_DIR);
  const schemas = [];
  for (const file of files.filter(f => f.endsWith('.schema.json'))) {
    const content = JSON.parse(await readFile(join(SCHEMA_DIR, file), 'utf-8'));
    schemas.push({ file, schema: content });
  }
  return schemas;
}

/** Sample valid payloads for each schema, keyed by filename. */
const VALID_SAMPLES = {
  'node-detail.schema.json': {
    schemaVersion: 1,
    command: 'nodes',
    id: 'task:build-ui',
    prefix: 'task',
    prefixClass: 'canonical',
    properties: { status: 'done' },
  },
  'node-list.schema.json': {
    schemaVersion: 1,
    command: 'nodes',
    nodes: ['task:a', 'task:b'],
  },
  'status.schema.json': {
    schemaVersion: 1,
    command: 'status',
    nodes: { total: 5, byPrefix: { task: 3, spec: 2 } },
    edges: { total: 4, byType: { implements: 2, 'relates-to': 2 } },
    health: { blockedItems: 0, lowConfidence: 1, orphanNodes: 0 },
  },
  'at.schema.json': {
    schemaVersion: 1,
    command: 'at',
    ref: 'HEAD~3',
    sha: 'abcdef12',
    fullSha: 'abcdef1234567890',
    tick: 42,
    nearest: false,
    recordedAt: '2026-02-13T00:00:00Z',
    status: {
      nodes: { total: 3, byPrefix: { task: 3 } },
      edges: { total: 1, byType: { implements: 1 } },
      health: { blockedItems: 0, lowConfidence: 0, orphanNodes: 0 },
    },
  },
  'import.schema.json': {
    schemaVersion: 1,
    command: 'import',
    valid: true,
    errors: [],
    warnings: [],
    stats: { nodes: 5, edges: 3 },
    dryRun: false,
  },
  'export-data.schema.json': {
    schemaVersion: 1,
    command: 'export',
    version: 1,
    nodes: [{ id: 'task:a' }, { id: 'task:b', properties: { status: 'done' } }],
    edges: [{ source: 'task:a', target: 'task:b', type: 'blocks', confidence: 1.0 }],
  },
  'export-file.schema.json': {
    schemaVersion: 1,
    command: 'export',
    stats: { nodes: 5, edges: 3 },
    path: '/tmp/export.yaml',
  },
  'merge.schema.json': {
    schemaVersion: 1,
    command: 'merge',
    nodes: 10,
    edges: 8,
    repoName: 'owner/repo',
    dryRun: false,
  },
  'doctor.schema.json': {
    schemaVersion: 1,
    command: 'doctor',
    issues: [
      {
        type: 'orphan-node',
        severity: 'info',
        message: 'Node task:orphan is not connected to any edge',
        affected: ['task:orphan'],
      },
    ],
    summary: { errors: 0, warnings: 0, info: 1 },
    clean: false,
  },
  'suggest.schema.json': {
    schemaVersion: 1,
    command: 'suggest',
    suggestions: [
      { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.8, rationale: 'test' },
    ],
    errors: [],
    prompt: 'Given the following graph...',
    rejectedCount: 0,
  },
  'review-list.schema.json': {
    schemaVersion: 1,
    command: 'review',
    pending: [
      { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.3 },
    ],
  },
  'review-batch.schema.json': {
    schemaVersion: 1,
    command: 'review',
    processed: 1,
    decisions: [
      {
        id: 'decision:1234-abcd',
        action: 'accept',
        source: 'task:a',
        target: 'spec:b',
        edgeType: 'implements',
        confidence: 1.0,
        timestamp: 1739500000,
      },
    ],
  },
  'diff.schema.json': {
    schemaVersion: 1,
    command: 'diff',
    from: { ref: 'HEAD~1', sha: 'aabbccdd', tick: 10, nearest: false },
    to: { ref: 'HEAD', sha: '11223344', tick: 20, nearest: false },
    nodes: { added: ['task:new'], removed: [], total: { before: 5, after: 6 } },
    edges: { added: [], removed: [], total: { before: 3, after: 3 } },
    summary: {
      nodesByPrefix: { task: { before: 5, after: 6 } },
      edgesByType: { implements: { before: 3, after: 3 } },
    },
    stats: {
      materializeMs: { a: 10, b: 12 },
      diffMs: 5,
      nodeCount: { a: 5, b: 6 },
      edgeCount: { a: 3, b: 3 },
    },
  },
};

describe('CLI JSON Schema contracts', () => {
  let schemas;
  let ajv;
  /** @type {Map<string, import('ajv').ValidateFunction>} */
  let validators;

  beforeAll(async () => {
    schemas = await loadSchemas();
    ajv = new Ajv({ strict: true, allErrors: true });
    validators = new Map();
    for (const { file, schema } of schemas) {
      validators.set(file, ajv.compile(schema));
    }
  });

  it('every schema file has a valid sample', () => {
    expect(schemas.length).toBeGreaterThan(0);
    for (const { file } of schemas) {
      expect(VALID_SAMPLES[file], `missing VALID_SAMPLES entry for ${file}`).toBeDefined();
    }
  });

  describe('schema compilation', () => {
    it('every .schema.json compiles as valid JSON Schema', () => {
      for (const { file } of schemas) {
        expect(validators.get(file), `${file} failed to compile`).toBeDefined();
      }
    });
  });

  describe('envelope requirements', () => {
    it('every schema requires schemaVersion', () => {
      for (const { file, schema } of schemas) {
        expect(schema.required, `${file} missing required array`).toContain('schemaVersion');
      }
    });

    it('every schema requires command', () => {
      for (const { file, schema } of schemas) {
        expect(schema.required, `${file} missing command in required`).toContain('command');
      }
    });

    it('every schema defines schemaVersion as integer const 1', () => {
      for (const { file, schema } of schemas) {
        const sv = schema.properties?.schemaVersion;
        expect(sv, `${file} missing schemaVersion property`).toBeDefined();
        expect(sv.type, `${file} schemaVersion type`).toBe('integer');
        expect(sv.const, `${file} schemaVersion const`).toBe(1);
      }
    });
  });

  describe('sample validation', () => {
    it('valid sample passes each schema', () => {
      for (const { file } of schemas) {
        const sample = VALID_SAMPLES[file];
        expect(sample, `missing valid sample for ${file}`).toBeDefined();
        const validate = validators.get(file);
        const valid = validate(structuredClone(sample));
        expect(valid, `${file}: ${JSON.stringify(validate.errors)}`).toBe(true);
      }
    });

    it('missing schemaVersion rejected by every schema', () => {
      for (const { file } of schemas) {
        const sample = structuredClone(VALID_SAMPLES[file]);
        delete sample.schemaVersion;
        const validate = validators.get(file);
        expect(validate(sample), `${file} should reject missing schemaVersion`).toBe(false);
      }
    });

    it('missing command rejected by every schema', () => {
      for (const { file } of schemas) {
        const sample = structuredClone(VALID_SAMPLES[file]);
        delete sample.command;
        const validate = validators.get(file);
        expect(validate(sample), `${file} should reject missing command`).toBe(false);
      }
    });

    it('wrong schemaVersion rejected by every schema', () => {
      for (const { file } of schemas) {
        const sample = structuredClone(VALID_SAMPLES[file]);
        sample.schemaVersion = 99;
        const validate = validators.get(file);
        expect(validate(sample), `${file} should reject schemaVersion 99`).toBe(false);
      }
    });
  });

  describe('optional fields', () => {
    it('doctor schema accepts output with fix field', () => {
      const sample = structuredClone(VALID_SAMPLES['doctor.schema.json']);
      sample.fix = { fixed: 1, skipped: 0, details: ['Removed dangling edge'] };
      const validate = validators.get('doctor.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('doctor schema accepts output without fix field', () => {
      const sample = structuredClone(VALID_SAMPLES['doctor.schema.json']);
      const validate = validators.get('doctor.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('at schema accepts null recordedAt', () => {
      const sample = structuredClone(VALID_SAMPLES['at.schema.json']);
      sample.recordedAt = null;
      const validate = validators.get('at.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('at schema accepts missing recordedAt', () => {
      const sample = structuredClone(VALID_SAMPLES['at.schema.json']);
      delete sample.recordedAt;
      const validate = validators.get('at.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('diff schema accepts sameTick in stats', () => {
      const sample = structuredClone(VALID_SAMPLES['diff.schema.json']);
      sample.stats.sameTick = true;
      const validate = validators.get('diff.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('import schema allows missing dryRun (optional)', () => {
      const sample = structuredClone(VALID_SAMPLES['import.schema.json']);
      delete sample.dryRun;
      const validate = validators.get('import.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('review-list schema accepts optional rationale and createdAt', () => {
      const sample = structuredClone(VALID_SAMPLES['review-list.schema.json']);
      sample.pending[0].rationale = 'test rationale';
      sample.pending[0].createdAt = '2026-01-01T00:00:00Z';
      const validate = validators.get('review-list.schema.json');
      expect(validate(sample)).toBe(true);
    });

    it('suggest schema accepts null prompt', () => {
      const sample = structuredClone(VALID_SAMPLES['suggest.schema.json']);
      sample.prompt = null;
      const validate = validators.get('suggest.schema.json');
      expect(validate(sample)).toBe(true);
    });
  });
});
