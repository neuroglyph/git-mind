/**
 * PROVING GROUND — Integration tests for dogfood validation.
 *
 * Imports the Echo ecosystem seed YAML and answers 5 real project
 * management questions against the resulting graph. Each question
 * maps to an existing view or API.
 *
 * Ground truth is baked into the seed design (test/fixtures/echo-seed.yaml).
 */

import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join, resolve } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { initGraph } from '../src/graph.js';
import { importFile } from '../src/import.js';
import { renderView } from '../src/views.js';
import { computeStatus } from '../src/status.js';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const SEED_PATH = resolve(__dirname, 'fixtures', 'echo-seed.yaml');

describe('PROVING GROUND', () => {
  let tempDir;
  let graph;

  beforeAll(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-proving-ground-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterAll(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── PRV-002: Seed import ─────────────────────────────────────

  describe('seed import', () => {
    it('imports the Echo seed without errors', async () => {
      const result = await importFile(graph, SEED_PATH);
      expect(result.valid).toBe(true);
      expect(result.errors).toEqual([]);
      expect(result.dryRun).toBe(false);
    });

    it('produces expected node count', async () => {
      const nodes = await graph.getNodes();
      expect(nodes.length).toBe(55);
    });

    it('produces expected edge count', async () => {
      const edges = await graph.getEdges();
      expect(edges.length).toBe(70);
    });
  });

  // ── PRV-003: Five dogfood questions ──────────────────────────

  describe('Q1: What blocks M2?', () => {
    it('milestone view identifies issue:E-003 and issue:E-004 as M2 blockers', async () => {
      const result = await renderView(graph, 'milestone');
      const m2 = result.meta.milestoneStats['milestone:M2'];

      expect(m2).toBeDefined();
      expect(m2.blockers).toHaveLength(2);
      expect(m2.blockers).toContain('issue:E-003');
      expect(m2.blockers).toContain('issue:E-004');
    });

    it('M2 has 2 children and 0 done', async () => {
      const result = await renderView(graph, 'milestone');
      const m2 = result.meta.milestoneStats['milestone:M2'];

      expect(m2.total).toBe(2);
      expect(m2.done).toBe(0);
      expect(m2.pct).toBe(0);
    });
  });

  describe('Q2: Which ADRs lack implementation?', () => {
    it('traceability view finds adr:003 and adr:004 as gaps', async () => {
      const result = await renderView(graph, 'traceability');
      const adrGaps = result.meta.gaps.filter(g => g.startsWith('adr:'));

      expect(adrGaps).toHaveLength(2);
      expect(adrGaps).toContain('adr:003-encryption-at-rest');
      expect(adrGaps).toContain('adr:004-rest-vs-grpc');
    });

    it('3 of 5 ADRs are implemented', async () => {
      const result = await renderView(graph, 'traceability');
      const adrCovered = result.meta.covered.filter(c => c.startsWith('adr:'));

      expect(adrCovered).toHaveLength(3);
      expect(adrCovered).toContain('adr:001-event-sourcing');
      expect(adrCovered).toContain('adr:002-postgres');
      expect(adrCovered).toContain('adr:005-crate-structure');
    });
  });

  describe('Q3: Which crates are unlinked to specs?', () => {
    it('coverage view finds 9 unlinked crates', async () => {
      const result = await renderView(graph, 'coverage');

      expect(result.meta.unlinked).toHaveLength(9);
      expect(result.meta.unlinked).toContain('crate:echo-crypto');
      expect(result.meta.unlinked).toContain('crate:echo-cli');
      expect(result.meta.unlinked).toContain('crate:echo-config');
      expect(result.meta.unlinked).toContain('crate:echo-log');
      expect(result.meta.unlinked).toContain('crate:echo-test-utils');
      expect(result.meta.unlinked).toContain('crate:echo-bench');
      expect(result.meta.unlinked).toContain('crate:echo-migrate');
      expect(result.meta.unlinked).toContain('crate:echo-plugin');
      expect(result.meta.unlinked).toContain('crate:echo-sdk');
    });

    it('6 crates are linked', async () => {
      const result = await renderView(graph, 'coverage');

      expect(result.meta.linked).toHaveLength(6);
      expect(result.meta.coveragePct).toBe(40);
    });
  });

  describe('Q4: What should a new engineer read first?', () => {
    it('onboarding view puts doc:getting-started before doc:architecture-overview', async () => {
      const result = await renderView(graph, 'onboarding');
      const order = result.meta.readingOrder;

      const gsIdx = order.indexOf('doc:getting-started');
      const aoIdx = order.indexOf('doc:architecture-overview');

      expect(gsIdx).toBeGreaterThanOrEqual(0);
      expect(aoIdx).toBeGreaterThan(gsIdx);
    });

    it('doc:architecture-overview appears before doc:api-reference', async () => {
      const result = await renderView(graph, 'onboarding');
      const order = result.meta.readingOrder;

      const aoIdx = order.indexOf('doc:architecture-overview');
      const arIdx = order.indexOf('doc:api-reference');

      expect(arIdx).toBeGreaterThan(aoIdx);
    });
  });

  describe('Q5: What is low-confidence?', () => {
    it('suggestions view finds exactly 4 low-confidence edges', async () => {
      const result = await renderView(graph, 'suggestions');
      expect(result.edges).toHaveLength(4);
    });

    it('computeStatus reports 4 low-confidence edges', async () => {
      const status = await computeStatus(graph);
      expect(status.health.lowConfidence).toBe(4);
    });

    it('low-confidence edges have expected confidence values', async () => {
      const result = await renderView(graph, 'suggestions');
      const confidences = result.edges
        .map(e => e.props?.confidence)
        .sort();
      expect(confidences).toEqual([0.2, 0.3, 0.3, 0.4]);
    });
  });

  // ── PRV-004: Timing ──────────────────────────────────────────

  describe('timing', () => {
    it('all 5 queries complete in under 60s total', async () => {
      const start = performance.now();

      await renderView(graph, 'milestone');
      await renderView(graph, 'traceability');
      await renderView(graph, 'coverage');
      await renderView(graph, 'onboarding');
      await renderView(graph, 'suggestions');

      const elapsed = performance.now() - start;
      expect(elapsed).toBeLessThan(60_000);
    });
  });
});
