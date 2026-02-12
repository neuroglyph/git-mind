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
  let importResult;

  beforeAll(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-proving-ground-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
    importResult = await importFile(graph, SEED_PATH);
  });

  afterAll(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── PRV-002: Seed import ─────────────────────────────────────

  describe('seed import', () => {
    it('imports the Echo seed without errors', () => {
      expect(importResult.valid).toBe(true);
      expect(importResult.errors).toEqual([]);
      expect(importResult.dryRun).toBe(false);
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
        .sort((a, b) => a - b);
      expect(confidences).toEqual([0.2, 0.3, 0.3, 0.4]);
    });
  });

  // ── PRV-004: Complexity verification ──────────────────────────

  describe('complexity', () => {
    /**
     * Generate a synthetic graph with ~N nodes and ~0.7N edges.
     * Uses the same prefixes/types as echo-seed so views exercise real code paths.
     */
    async function generateGraph(nodeCount) {
      const dir = await mkdtemp(join(tmpdir(), 'gitmind-complexity-'));
      execSync('git init', { cwd: dir, stdio: 'ignore' });
      const g = await initGraph(dir);

      const patch = await g.createPatch();

      // Distribute nodes across 6 prefixes
      const nMilestones = Math.floor(nodeCount / 5);
      const nTasks = Math.floor(nodeCount / 5);
      const nSpecs = Math.floor(nodeCount / 5);
      const nCrates = Math.floor(nodeCount / 5);
      const nDocs = Math.floor(nodeCount / 10);
      const nIssues = Math.floor(nodeCount / 10);

      // Create nodes
      for (let i = 0; i < nMilestones; i++) {
        patch.addNode(`milestone:M${i}`);
        patch.setProperty(`milestone:M${i}`, 'title', `Milestone ${i}`);
        patch.setProperty(`milestone:M${i}`, 'status', i % 3 === 0 ? 'complete' : 'planned');
      }
      for (let i = 0; i < nTasks; i++) {
        patch.addNode(`task:T${i}`);
        patch.setProperty(`task:T${i}`, 'title', `Task ${i}`);
      }
      for (let i = 0; i < nSpecs; i++) {
        patch.addNode(`spec:S${i}`);
        patch.setProperty(`spec:S${i}`, 'title', `Spec ${i}`);
      }
      for (let i = 0; i < nCrates; i++) {
        patch.addNode(`crate:C${i}`);
        patch.setProperty(`crate:C${i}`, 'description', `Crate ${i}`);
      }
      for (let i = 0; i < nDocs; i++) {
        patch.addNode(`doc:D${i}`);
        patch.setProperty(`doc:D${i}`, 'title', `Doc ${i}`);
      }
      for (let i = 0; i < nIssues; i++) {
        patch.addNode(`issue:I${i}`);
        patch.setProperty(`issue:I${i}`, 'title', `Issue ${i}`);
      }

      // belongs-to: tasks → milestones
      for (let i = 0; i < nTasks; i++) {
        patch.addEdge(`task:T${i}`, `milestone:M${i % nMilestones}`, 'belongs-to');
      }
      // implements: crates → specs
      for (let i = 0; i < Math.min(nCrates, nSpecs); i++) {
        patch.addEdge(`crate:C${i}`, `spec:S${i}`, 'implements');
      }
      // depends-on: crate → crate chain
      for (let i = 1; i < nCrates; i++) {
        patch.addEdge(`crate:C${i}`, `crate:C${i - 1}`, 'depends-on');
      }
      // depends-on: doc → doc chain (for onboarding view)
      for (let i = 1; i < nDocs; i++) {
        patch.addEdge(`doc:D${i}`, `doc:D${i - 1}`, 'depends-on');
      }
      // 4 low-confidence relates-to edges (for suggestions view)
      if (nIssues >= 2 && nCrates >= 1) {
        patch.addEdge(`issue:I0`, `issue:I1`, 'relates-to');
        patch.setEdgeProperty(`issue:I0`, `issue:I1`, 'relates-to', 'confidence', 0.2);
        patch.addEdge(`issue:I1`, `crate:C0`, 'relates-to');
        patch.setEdgeProperty(`issue:I1`, `crate:C0`, 'relates-to', 'confidence', 0.3);
      }
      if (nIssues >= 4) {
        patch.addEdge(`issue:I2`, `issue:I3`, 'relates-to');
        patch.setEdgeProperty(`issue:I2`, `issue:I3`, 'relates-to', 'confidence', 0.4);
        patch.addEdge(`issue:I3`, `issue:I0`, 'relates-to');
        patch.setEdgeProperty(`issue:I3`, `issue:I0`, 'relates-to', 'confidence', 0.1);
      }

      await patch.commit();
      return { graph: g, dir };
    }

    it('all 5 views scale sub-quadratically (O(N+E))', async () => {
      const sizes = [100, 500, 2500, 12500];
      const timings = [];

      for (const size of sizes) {
        const { graph: g, dir } = await generateGraph(size);
        try {
          const start = performance.now();
          await renderView(g, 'milestone');
          await renderView(g, 'traceability');
          await renderView(g, 'coverage');
          await renderView(g, 'onboarding');
          await renderView(g, 'suggestions');
          timings.push(performance.now() - start);
        } finally {
          await rm(dir, { recursive: true, force: true });
        }
      }

      // Check growth factors between consecutive 5x size steps.
      // Linear (O(N+E)) ≈ 5x growth. Quadratic (O(N²)) = 25x growth.
      // Threshold of 15x catches O(N²) with margin for constant-factor overhead.
      for (let i = 1; i < timings.length; i++) {
        const growthFactor = timings[i] / Math.max(timings[i - 1], 1);
        expect(growthFactor).toBeLessThan(15);
      }
    }, 120_000);
  });
});
