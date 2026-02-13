import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm, writeFile, mkdir } from 'node:fs/promises';
import { join, dirname } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { parseFrontmatter, extractGraphData, findMarkdownFiles, importFromMarkdown } from '../src/frontmatter.js';
import { exportGraph } from '../src/export.js';

describe('frontmatter', () => {
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
   * Helper: write a markdown file and return its path.
   */
  async function writeMd(relativePath, content) {
    const fullPath = join(tempDir, relativePath);
    const dir = dirname(fullPath);
    await mkdir(dir, { recursive: true });
    await writeFile(fullPath, content, 'utf-8');
    return fullPath;
  }

  // ── parseFrontmatter ────────────────────────────────────────

  describe('parseFrontmatter', () => {
    it('parses valid frontmatter', () => {
      const content = `---
title: "Hello"
id: doc:hello
---
# Hello World`;
      const { frontmatter, body } = parseFrontmatter(content);
      expect(frontmatter).toEqual({ title: 'Hello', id: 'doc:hello' });
      expect(body).toContain('# Hello World');
    });

    it('returns null for missing frontmatter', () => {
      const { frontmatter, body } = parseFrontmatter('# Just a heading\nSome content');
      expect(frontmatter).toBeNull();
      expect(body).toContain('# Just a heading');
    });

    it('returns null for unclosed frontmatter', () => {
      const content = `---
title: "Open"
# No closing delimiter`;
      const { frontmatter } = parseFrontmatter(content);
      expect(frontmatter).toBeNull();
    });

    it('handles CRLF line endings', () => {
      const content = '---\r\ntitle: "Hello"\r\nid: doc:hello\r\n---\r\n# Hello World';
      const { frontmatter, body } = parseFrontmatter(content);
      expect(frontmatter).toEqual({ title: 'Hello', id: 'doc:hello' });
      expect(body).toContain('# Hello World');
    });

    it('returns null for invalid YAML in frontmatter', () => {
      const content = `---
: invalid: yaml: {{
---
Body`;
      const { frontmatter } = parseFrontmatter(content);
      expect(frontmatter).toBeNull();
    });
  });

  // ── extractGraphData ────────────────────────────────────────

  describe('extractGraphData', () => {
    it('uses explicit id from frontmatter', () => {
      const { node } = extractGraphData('docs/arch.md', { id: 'doc:architecture' });
      expect(node.id).toBe('doc:architecture');
    });

    it('auto-generates id from path', () => {
      const { node } = extractGraphData('docs/getting-started.md', { title: 'Guide' });
      expect(node.id).toBe('doc:docs/getting-started');
    });

    it('extracts title as node property', () => {
      const { node } = extractGraphData('readme.md', { title: 'README' });
      expect(node.properties).toEqual({ title: 'README' });
    });

    it('extracts implements edge', () => {
      const { edges } = extractGraphData('auth.md', {
        id: 'doc:auth',
        implements: 'spec:auth',
      });
      expect(edges).toHaveLength(1);
      expect(edges[0]).toEqual({ source: 'doc:auth', target: 'spec:auth', type: 'implements' });
    });

    it('extracts array of relates-to edges', () => {
      const { edges } = extractGraphData('arch.md', {
        id: 'doc:arch',
        'relates-to': ['spec:a', 'spec:b'],
      });
      expect(edges).toHaveLength(2);
      expect(edges[0].target).toBe('spec:a');
      expect(edges[1].target).toBe('spec:b');
    });

    it('strips extension from filename, not directory', () => {
      const { node } = extractGraphData('notes.md/index.md', { title: 'Notes' });
      expect(node.id).toBe('doc:notes.md/index');
    });

    it('extracts multiple edge types', () => {
      const { edges } = extractGraphData('design.md', {
        id: 'doc:design',
        implements: 'spec:auth',
        'depends-on': 'doc:getting-started',
        documents: 'module:auth',
      });
      expect(edges).toHaveLength(3);
      const types = edges.map(e => e.type).sort();
      expect(types).toEqual(['depends-on', 'documents', 'implements']);
    });
  });

  // ── findMarkdownFiles ───────────────────────────────────────

  describe('findMarkdownFiles', () => {
    it('finds .md files recursively', async () => {
      await writeMd('docs/a.md', '# A');
      await writeMd('docs/sub/b.md', '# B');
      await writeFile(join(tempDir, 'docs/c.txt'), 'not markdown');

      const files = await findMarkdownFiles(tempDir, 'docs/**/*.md');
      expect(files).toHaveLength(2);
      expect(files[0]).toContain('a.md');
      expect(files[1]).toContain('b.md');
    });

    it('returns empty for non-existent directory', async () => {
      const files = await findMarkdownFiles(tempDir, 'nonexistent/**/*.md');
      expect(files).toEqual([]);
    });

    it('throws on permission errors (not ENOENT)', async () => {
      // Create a directory with no read permissions
      const restrictedDir = join(tempDir, 'restricted');
      const { mkdir: mkdirFs, chmod: chmodFs } = await import('node:fs/promises');
      await mkdirFs(restrictedDir);
      await chmodFs(restrictedDir, 0o000);

      try {
        await expect(findMarkdownFiles(tempDir, 'restricted/**/*.md'))
          .rejects.toThrow();
      } finally {
        // Restore permissions for cleanup
        await chmodFs(restrictedDir, 0o755);
      }
    });
  });

  // ── importFromMarkdown ──────────────────────────────────────

  describe('importFromMarkdown', () => {
    it('imports nodes from frontmatter', async () => {
      await writeMd('docs/auth.md', `---
id: doc:auth
title: "Auth Guide"
---
# Authentication`);

      const result = await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      expect(result.valid).toBe(true);
      expect(result.stats.nodes).toBe(1);

      const nodes = await graph.getNodes();
      expect(nodes).toContain('doc:auth');
    });

    it('imports edges from frontmatter', async () => {
      // Create the target node first
      const patch = await graph.createPatch();
      patch.addNode('spec:auth');
      await patch.commit();

      await writeMd('docs/auth.md', `---
id: doc:auth
implements: spec:auth
---
# Auth`);

      const result = await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      expect(result.valid).toBe(true);
      expect(result.stats.edges).toBe(1);

      const edges = await graph.getEdges();
      expect(edges).toHaveLength(1);
      expect(edges[0].from).toBe('doc:auth');
      expect(edges[0].to).toBe('spec:auth');
    });

    it('supports dry-run mode', async () => {
      await writeMd('docs/test.md', `---
id: doc:test
title: "Test"
---
# Test`);

      const result = await importFromMarkdown(graph, tempDir, 'docs/**/*.md', { dryRun: true });
      expect(result.valid).toBe(true);
      expect(result.dryRun).toBe(true);
      expect(result.stats.nodes).toBe(1);

      // Graph should be unchanged
      const nodes = await graph.getNodes();
      expect(nodes).toHaveLength(0);
    });

    it('handles no matching files', async () => {
      const result = await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      expect(result.valid).toBe(true);
      expect(result.warnings[0]).toMatch(/No markdown files found/);
    });

    it('skips files without frontmatter', async () => {
      await writeMd('docs/plain.md', '# Just a heading\nNo frontmatter here.');
      await writeMd('docs/with-fm.md', `---
id: doc:with-fm
title: "Has FM"
---
# Content`);

      const result = await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      expect(result.valid).toBe(true);
      expect(result.stats.nodes).toBe(1);
    });

    it('is idempotent on re-import', async () => {
      await writeMd('docs/auth.md', `---
id: doc:auth
title: "Auth Guide"
---
# Authentication`);

      await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      await importFromMarkdown(graph, tempDir, 'docs/**/*.md');

      const nodes = await graph.getNodes();
      expect(nodes.filter(n => n === 'doc:auth')).toHaveLength(1);
    });

    it('round-trips with export', async () => {
      await writeMd('docs/auth.md', `---
id: doc:auth
title: "Auth Guide"
implements: spec:auth
---
# Authentication`);

      await importFromMarkdown(graph, tempDir, 'docs/**/*.md');
      const exported = await exportGraph(graph);

      expect(exported.nodes.map(n => n.id).sort()).toEqual(['doc:auth', 'spec:auth']);
      expect(exported.edges).toHaveLength(1);
      expect(exported.edges[0].type).toBe('implements');
    });
  });
});
