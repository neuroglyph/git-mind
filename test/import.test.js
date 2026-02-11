import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { importFile, parseImportFile, validateImportData } from '../src/import.js';

describe('import', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  /**
   * Helper: write a YAML file and return its path.
   */
  async function writeYaml(filename, content) {
    const path = join(tempDir, filename);
    await writeFile(path, content, 'utf-8');
    return path;
  }

  // ── Schema validation ─────────────────────────────────────────

  describe('schema validation', () => {
    it('rejects missing version field', async () => {
      const path = await writeYaml('bad.yaml', `
nodes:
  - id: "task:a"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/Missing required field.*version/);
    });

    it('rejects unsupported version', async () => {
      const path = await writeYaml('bad.yaml', `
version: 99
nodes:
  - id: "task:a"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/Unsupported version: 99/);
    });

    it('rejects non-object YAML', async () => {
      const path = await writeYaml('bad.yaml', `just a string`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/not an object/);
    });

    it('reports file not found', async () => {
      const result = await importFile(graph, '/nonexistent/file.yaml');
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/File not found/);
    });

    it('rejects non-array nodes field', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes: "not-an-array"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/"nodes" must be an array/));
    });

    it('rejects non-array edges field', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
edges: "not-an-array"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/"edges" must be an array/));
    });
  });

  // ── Node validation ───────────────────────────────────────────

  describe('node validation', () => {
    it('rejects invalid node IDs', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "bad id"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/nodes\[0\].*Invalid node ID/);
    });

    it('rejects nodes without id field', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - name: "oops"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors[0]).toMatch(/nodes\[0\].*missing required field "id"/);
    });

    it('warns on unknown prefix', async () => {
      const path = await writeYaml('warn.yaml', `
version: 1
nodes:
  - id: "custom:thing"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(true);
      expect(result.warnings[0]).toMatch(/prefix "custom" is not a canonical prefix/);
    });
  });

  // ── Edge validation ───────────────────────────────────────────

  describe('edge validation', () => {
    it('rejects invalid edge type', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "task:a"
  - id: "task:b"
edges:
  - source: "task:a"
    target: "task:b"
    type: "invalid-type"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/Unknown edge type/));
    });

    it('rejects invalid confidence', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "task:a"
  - id: "task:b"
edges:
  - source: "task:a"
    target: "task:b"
    type: "relates-to"
    confidence: 5.0
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/between 0\.0 and 1\.0/));
    });

    it('rejects self-edge for blocks', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "task:a"
edges:
  - source: "task:a"
    target: "task:a"
    type: "blocks"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/self-edge forbidden/));
    });

    it('rejects edges with missing required fields', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
edges:
  - source: "task:a"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/missing required field "target"/));
      expect(result.errors).toContainEqual(expect.stringMatching(/missing required field "type"/));
    });
  });

  // ── Reference validation ──────────────────────────────────────

  describe('reference validation', () => {
    it('rejects dangling edge references', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "task:a"
edges:
  - source: "task:a"
    target: "spec:missing"
    type: "implements"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);
      expect(result.errors).toContainEqual(expect.stringMatching(/"spec:missing" is not declared/));
    });

    it('allows edges to pre-existing graph nodes', async () => {
      // Pre-populate the graph
      await createEdge(graph, { source: 'spec:auth', target: 'doc:readme', type: 'documents' });

      const path = await writeYaml('ok.yaml', `
version: 1
nodes:
  - id: "file:auth.js"
edges:
  - source: "file:auth.js"
    target: "spec:auth"
    type: "implements"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(true);
      expect(result.stats.edges).toBe(1);
    });
  });

  // ── Successful imports ────────────────────────────────────────

  describe('successful import', () => {
    it('imports nodes and edges', async () => {
      const path = await writeYaml('graph.yaml', `
version: 1
nodes:
  - id: "spec:auth"
  - id: "file:src/auth.js"
edges:
  - source: "file:src/auth.js"
    target: "spec:auth"
    type: "implements"
    confidence: 0.9
    rationale: "Main auth module"
`);
      const result = await importFile(graph, path);

      expect(result.valid).toBe(true);
      expect(result.dryRun).toBe(false);
      expect(result.stats.nodes).toBe(2);
      expect(result.stats.edges).toBe(1);

      // Verify graph state
      const nodes = await graph.getNodes();
      expect(nodes).toContain('spec:auth');
      expect(nodes).toContain('file:src/auth.js');

      const edges = await graph.getEdges();
      expect(edges.length).toBe(1);
      expect(edges[0].from).toBe('file:src/auth.js');
      expect(edges[0].to).toBe('spec:auth');
      expect(edges[0].label).toBe('implements');
      expect(edges[0].props.confidence).toBe(0.9);
      expect(edges[0].props.rationale).toBe('Main auth module');
    });

    it('imports nodes with properties', async () => {
      const path = await writeYaml('graph.yaml', `
version: 1
nodes:
  - id: "task:auth"
    properties:
      status: "active"
      priority: "high"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(true);

      const props = await graph.getNodeProps('task:auth');
      expect(props.get('status')).toBe('active');
      expect(props.get('priority')).toBe('high');
    });

    it('handles version-only file with no nodes or edges', async () => {
      const path = await writeYaml('empty.yaml', `version: 1`);
      const result = await importFile(graph, path);

      expect(result.valid).toBe(true);
      expect(result.stats.nodes).toBe(0);
      expect(result.stats.edges).toBe(0);
    });
  });

  // ── Dry run ───────────────────────────────────────────────────

  describe('dry run', () => {
    it('validates without writing', async () => {
      const path = await writeYaml('graph.yaml', `
version: 1
nodes:
  - id: "task:a"
  - id: "task:b"
edges:
  - source: "task:a"
    target: "task:b"
    type: "blocks"
`);
      const result = await importFile(graph, path, { dryRun: true });

      expect(result.valid).toBe(true);
      expect(result.dryRun).toBe(true);
      expect(result.stats.nodes).toBe(2);
      expect(result.stats.edges).toBe(1);

      // Graph should be unchanged
      const nodes = await graph.getNodes();
      expect(nodes.length).toBe(0);
    });
  });

  // ── Idempotent re-import ──────────────────────────────────────

  describe('idempotency', () => {
    it('re-import produces same graph state', async () => {
      const path = await writeYaml('graph.yaml', `
version: 1
nodes:
  - id: "spec:auth"
  - id: "file:auth.js"
edges:
  - source: "file:auth.js"
    target: "spec:auth"
    type: "implements"
`);
      // Import twice
      await importFile(graph, path);
      await importFile(graph, path);

      const nodes = await graph.getNodes();
      const edges = await graph.getEdges();

      expect(nodes.length).toBe(2);
      expect(edges.length).toBe(1);
    });
  });

  // ── Atomicity ─────────────────────────────────────────────────

  describe('atomicity', () => {
    it('writes nothing if validation fails', async () => {
      const path = await writeYaml('bad.yaml', `
version: 1
nodes:
  - id: "task:a"
edges:
  - source: "task:a"
    target: "spec:missing"
    type: "implements"
`);
      const result = await importFile(graph, path);
      expect(result.valid).toBe(false);

      // Graph should be empty
      const nodes = await graph.getNodes();
      expect(nodes.length).toBe(0);
    });
  });

  // ── JSON structure ────────────────────────────────────────────

  describe('result structure', () => {
    it('returns correct structure for JSON serialization', async () => {
      const path = await writeYaml('graph.yaml', `
version: 1
nodes:
  - id: "task:a"
`);
      const result = await importFile(graph, path);
      const json = JSON.parse(JSON.stringify(result));

      expect(json).toHaveProperty('valid');
      expect(json).toHaveProperty('errors');
      expect(json).toHaveProperty('warnings');
      expect(json).toHaveProperty('stats.nodes');
      expect(json).toHaveProperty('stats.edges');
      expect(json).toHaveProperty('dryRun');
    });
  });
});
