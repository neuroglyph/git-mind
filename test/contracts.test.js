/**
 * @module test/contracts
 * Schema contract unit tests — validates JSON Schema files compile,
 * require envelope fields, and accept/reject expected payloads.
 *
 * Covers two schema families:
 * - CLI schemas (docs/contracts/cli/*.schema.json) — all require schemaVersion + command
 * - BLP schemas (docs/contracts/*.schema.json) — structural schemas, no command field
 */

import { describe, it, expect, beforeAll } from 'vitest';
import { readdir, readFile } from 'node:fs/promises';
import { join, basename } from 'node:path';
import Ajv from 'ajv/dist/2020.js';

const SCHEMA_DIR = join(import.meta.dirname, '..', 'docs', 'contracts', 'cli');
const BLP_SCHEMA_DIR = join(import.meta.dirname, '..', 'docs', 'contracts');

/** Load all schema files from the given directory (non-recursive). */
async function loadSchemas(dir = SCHEMA_DIR) {
  const files = await readdir(dir);
  const schemas = [];
  for (const file of files.filter(f => f.endsWith('.schema.json'))) {
    const content = JSON.parse(await readFile(join(dir, file), 'utf-8'));
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
  'set.schema.json': {
    schemaVersion: 1,
    command: 'set',
    id: 'task:BDK-001',
    key: 'status',
    value: 'done',
    previous: null,
    changed: true,
  },
  'unset.schema.json': {
    schemaVersion: 1,
    command: 'unset',
    id: 'task:BDK-001',
    key: 'status',
    previous: 'done',
    removed: true,
  },
  'view-lens.schema.json': {
    schemaVersion: 1,
    command: 'view',
    viewName: 'backlog',
    lenses: ['blocked'],
    nodes: ['task:b'],
    edges: [{ from: 'task:a', to: 'task:b', label: 'blocks' }],
    meta: { lens: 'blocked' },
  },
  'view-progress.schema.json': {
    schemaVersion: 1,
    command: 'view',
    viewName: 'progress',
    nodes: ['task:a', 'task:b'],
    edges: [{ from: 'task:a', to: 'task:b', label: 'blocks' }],
    meta: {
      byStatus: {
        'done': ['task:a'],
        'in-progress': [],
        'todo': ['task:b'],
        'blocked': [],
        'unknown': [],
      },
      summary: {
        total: 2,
        done: 1,
        'in-progress': 0,
        todo: 1,
        blocked: 0,
        unknown: 0,
        pct: 50,
        ratio: '1/2',
        remaining: 1,
      },
    },
  },
  'extension-list.schema.json': {
    schemaVersion: 1,
    command: 'extension-list',
    extensions: [
      {
        name: 'roadmap',
        version: '1.0.0',
        description: 'Project roadmap domain',
        builtin: true,
        views: [{ name: 'roadmap', prefixes: ['phase', 'task'] }],
        lenses: ['incomplete', 'frontier'],
      },
    ],
  },
  'extension-validate.schema.json': {
    schemaVersion: 1,
    command: 'extension-validate',
    valid: true,
    errors: [],
    record: { name: 'test-ext', version: '1.0.0' },
  },
  'extension-add.schema.json': {
    schemaVersion: 1,
    command: 'extension-add',
    name: 'test-ext',
    version: '1.0.0',
    views: ['widgets'],
    lenses: ['incomplete'],
  },
  'extension-remove.schema.json': {
    schemaVersion: 1,
    command: 'extension-remove',
    name: 'test-ext',
    version: '1.0.0',
  },
  'content-set.schema.json': {
    schemaVersion: 1,
    command: 'content-set',
    nodeId: 'doc:readme',
    sha: 'a'.repeat(40),
    mime: 'text/markdown',
    size: 42,
  },
  'content-show.schema.json': {
    schemaVersion: 1,
    command: 'content-show',
    nodeId: 'doc:readme',
    content: '# Hello World\n',
    sha: 'a'.repeat(40),
    mime: 'text/markdown',
    size: 15,
  },
  'content-meta.schema.json': {
    schemaVersion: 1,
    command: 'content-meta',
    nodeId: 'doc:readme',
    hasContent: true,
    sha: 'a'.repeat(40),
    mime: 'text/markdown',
    size: 15,
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

    it('diff schema accepts skipped diff with null totals', () => {
      const sample = structuredClone(VALID_SAMPLES['diff.schema.json']);
      sample.stats.sameTick = true;
      sample.stats.skipped = true;
      sample.nodes.total = null;
      sample.edges.total = null;
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

// ── BLP Schemas (docs/contracts/*.schema.json) ───────────────────────────────
// Structural schemas for extension manifests, context envelopes, content
// objects, materialization specs, provenance, and trust policies.
// These are NOT CLI output schemas — they do not require schemaVersion+command.

/** Sample valid payloads for each BLP schema, keyed by filename. */
const BLP_VALID_SAMPLES = {
  'extension-manifest.schema.json': {
    name: 'roadmap',
    version: '1.0.0',
    description: 'Roadmap domain extension',
    domain: {
      prefixes: ['milestone', 'phase', 'task'],
      edgeTypes: ['belongs-to', 'blocks'],
      statusValues: ['todo', 'in-progress', 'done'],
    },
  },
  'context-envelope.schema.json': {
    asOf: 'HEAD',
    observer: null,
    trustPolicy: 'open',
  },
  'node-content.schema.json': {
    nodeId: 'doc:readme',
    mime: 'text/markdown',
    encoding: 'utf-8',
    body: '# Project README\n\nContent here.',
  },
  'materialization-spec.schema.json': {
    target: 'roadmap',
    format: 'markdown',
    outputPath: 'docs/published/roadmap.md',
  },
  'projection-result.schema.json': {
    schemaVersion: 1,
    target: 'roadmap',
    nodes: ['milestone:m1', 'task:t1'],
    edges: [{ source: 'task:t1', target: 'milestone:m1', type: 'belongs-to' }],
  },
  'provenance-envelope.schema.json': {
    schemaVersion: 1,
    artifactHash: 'abc123def456abc123def456abc123def456abc123def456abc123def456abc1',
    inputHash: 'def456abc123def456abc123def456abc123def456abc123def456abc123def4',
    graphFrontier: { 'local': 42 },
    renderedAt: '2026-02-17T00:00:00Z',
  },
  'artifact-bundle.schema.json': {
    schemaVersion: 1,
    artifactClass: 'ephemeral',
    spec: { target: 'roadmap', format: 'markdown' },
    provenance: {
      schemaVersion: 1,
      artifactHash: 'abc123def456abc123def456abc123def456abc123def456abc123def456abc1',
      inputHash: 'def456abc123def456abc123def456abc123def456abc123def456abc123def4',
      graphFrontier: { 'local': 42 },
      renderedAt: '2026-02-17T00:00:00Z',
    },
  },
  'trust-policy.schema.json': {
    schemaVersion: 1,
    mode: 'open',
  },
};

/** Invalid samples for BLP schemas to verify rejection. */
const BLP_INVALID_SAMPLES = {
  'extension-manifest.schema.json': { version: '1.0.0' }, // missing name + domain
  'context-envelope.schema.json': { trustPolicy: 'open' }, // missing asOf + observer
  'node-content.schema.json': { mime: 'text/markdown' }, // missing nodeId
  'materialization-spec.schema.json': { target: 'roadmap' }, // missing format
  'projection-result.schema.json': { target: 'roadmap', nodes: [] }, // missing schemaVersion + edges
  'provenance-envelope.schema.json': { artifactHash: 'abc' }, // missing multiple required
  'artifact-bundle.schema.json': { artifactClass: 'invalid-class', spec: {}, provenance: {} }, // invalid enum
  'trust-policy.schema.json': { mode: 'invalid-mode' }, // missing schemaVersion + invalid enum
};

describe('BLP JSON Schema contracts', () => {
  let schemas;
  let ajv;
  /** @type {Map<string, import('ajv').ValidateFunction>} */
  let validators;

  beforeAll(async () => {
    schemas = await loadSchemas(BLP_SCHEMA_DIR);
    // AJV strict mode is disabled for BLP schemas: they use $ref URIs and
    // draft-2020-12 features like if/then that require allowUnionTypes etc.
    ajv = new Ajv({ strict: false, allErrors: true });
    validators = new Map();
    for (const { file, schema } of schemas) {
      validators.set(file, ajv.compile(schema));
    }
  });

  it('BLP schema directory contains expected schemas', () => {
    const files = schemas.map(s => s.file).sort();
    const expected = Object.keys(BLP_VALID_SAMPLES).sort();
    for (const name of expected) {
      expect(files, `expected ${name} in docs/contracts/`).toContain(name);
    }
  });

  describe('schema compilation', () => {
    it('every .schema.json compiles as valid JSON Schema', () => {
      expect(schemas.length).toBeGreaterThan(0);
      for (const { file } of schemas) {
        expect(validators.get(file), `${file} failed to compile`).toBeDefined();
      }
    });
  });

  describe('valid sample acceptance', () => {
    it('valid sample passes each BLP schema', () => {
      for (const { file } of schemas) {
        const sample = BLP_VALID_SAMPLES[file];
        expect(sample, `missing BLP_VALID_SAMPLES entry for ${file}`).toBeDefined();
        const validate = validators.get(file);
        const valid = validate(structuredClone(sample));
        expect(valid, `${file}: ${JSON.stringify(validate.errors)}`).toBe(true);
      }
    });
  });

  describe('invalid sample rejection', () => {
    it('invalid sample fails each BLP schema', () => {
      for (const { file } of schemas) {
        const sample = BLP_INVALID_SAMPLES[file];
        expect(sample, `missing BLP_INVALID_SAMPLES entry for ${file}`).toBeDefined();
        const validate = validators.get(file);
        expect(validate(structuredClone(sample)), `${file} should reject invalid sample`).toBe(false);
      }
    });
  });

  describe('BLP schema spot checks', () => {
    it('extension-manifest accepts optional views/lenses/rules', () => {
      const sample = structuredClone(BLP_VALID_SAMPLES['extension-manifest.schema.json']);
      sample.views = [{ name: 'roadmap', prefixes: ['milestone', 'task'] }];
      sample.lenses = ['incomplete'];
      sample.rules = [{ name: 'orphan-node', severity: 'info' }];
      const validate = validators.get('extension-manifest.schema.json');
      expect(validate(sample), JSON.stringify(validate.errors)).toBe(true);
    });

    it('context-envelope accepts non-HEAD asOf', () => {
      const sample = { asOf: 'main~10', observer: 'security-team', trustPolicy: 'approved-only' };
      const validate = validators.get('context-envelope.schema.json');
      expect(validate(sample), JSON.stringify(validate.errors)).toBe(true);
    });

    it('node-content accepts casRef instead of body', () => {
      const sample = { nodeId: 'doc:spec', mime: 'text/markdown', casRef: 'abc123def456' };
      const validate = validators.get('node-content.schema.json');
      expect(validate(sample), JSON.stringify(validate.errors)).toBe(true);
    });

    it('trust-policy accepts approved-only with approvedWriters', () => {
      const sample = {
        schemaVersion: 1,
        mode: 'approved-only',
        approvedWriters: ['alice', 'bob'],
        delegations: [{ grantor: 'alice', grantee: 'carol', scope: 'task:*' }],
      };
      const validate = validators.get('trust-policy.schema.json');
      expect(validate(sample), JSON.stringify(validate.errors)).toBe(true);
    });

    it('artifact-bundle attested class accepts attestations', () => {
      const sample = structuredClone(BLP_VALID_SAMPLES['artifact-bundle.schema.json']);
      sample.artifactClass = 'attested';
      sample.attestations = [{ writerId: 'alice', signature: 'sig-abc123', timestamp: '2026-02-17T00:00:00Z' }];
      const validate = validators.get('artifact-bundle.schema.json');
      expect(validate(sample), JSON.stringify(validate.errors)).toBe(true);
    });
  });
});
