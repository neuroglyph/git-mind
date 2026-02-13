/**
 * @module frontmatter
 * Markdown frontmatter parsing and graph extraction.
 * Converts markdown files with YAML frontmatter into graph nodes and edges.
 */

import { readFile, readdir, stat } from 'node:fs/promises';
import { join, relative, extname } from 'node:path';
import yaml from 'js-yaml';
import { EDGE_TYPES } from './validators.js';
import { importData } from './import.js';

/** Edge type fields recognized in frontmatter. */
const EDGE_TYPE_SET = new Set(EDGE_TYPES);

/**
 * Parse YAML frontmatter from markdown content.
 *
 * @param {string} content - Raw markdown file content
 * @returns {{ frontmatter: Record<string, unknown> | null, body: string }}
 */
export function parseFrontmatter(content) {
  // Normalize CRLF to LF for consistent delimiter handling
  content = content.replace(/\r\n/g, '\n');

  if (!content.startsWith('---')) {
    return { frontmatter: null, body: content };
  }

  const firstNewline = content.indexOf('\n');
  if (firstNewline === -1) return { frontmatter: null, body: content };

  const endIdx = content.indexOf('\n---', firstNewline);
  if (endIdx === -1) {
    return { frontmatter: null, body: content };
  }

  const yamlBlock = content.slice(firstNewline + 1, endIdx);
  try {
    const parsed = yaml.load(yamlBlock);
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
      return { frontmatter: null, body: content.slice(endIdx + 4) };
    }
    return { frontmatter: parsed, body: content.slice(endIdx + 4) };
  } catch {
    return { frontmatter: null, body: content };
  }
}

/**
 * Extract graph node and edges from parsed frontmatter.
 *
 * @param {string} relativePath - File path relative to the base directory
 * @param {Record<string, unknown>} frontmatter - Parsed frontmatter
 * @returns {{ node: {id: string, properties?: Record<string, unknown>}, edges: Array<{source: string, target: string, type: string}> }}
 */
export function extractGraphData(relativePath, frontmatter) {
  // Determine node ID
  const pathWithoutExt = relativePath.replace(extname(relativePath), '');
  const id = typeof frontmatter.id === 'string' ? frontmatter.id : `doc:${pathWithoutExt}`;

  // Build node properties from non-edge frontmatter fields
  const properties = {};
  if (typeof frontmatter.title === 'string') {
    properties.title = frontmatter.title;
  }

  const node = { id };
  if (Object.keys(properties).length > 0) {
    node.properties = properties;
  }

  // Extract edges from edge-type fields
  const edges = [];
  for (const edgeType of EDGE_TYPE_SET) {
    const value = frontmatter[edgeType];
    if (!value) continue;

    const targets = Array.isArray(value) ? value : [value];
    for (const target of targets) {
      if (typeof target === 'string') {
        edges.push({ source: id, target, type: edgeType });
      }
    }
  }

  return { node, edges };
}

/**
 * Find markdown files matching a glob pattern.
 *
 * Supported patterns:
 * - `**\/*.md` — all .md files recursively from basePath
 * - `docs/**\/*.md` — all .md files recursively under docs/
 * - `*.md` — top-level .md files in basePath
 * - `docs/*.md` — top-level .md files in docs/
 *
 * Limitations:
 * - Exact file paths (e.g., "docs/README.md") are not matched directly
 * - Complex globs with character classes (e.g., "docs/202?/*.md") are not supported
 * - Returns empty results for non-existent start directories (no error thrown)
 *
 * @param {string} basePath - Root directory to search from
 * @param {string} pattern - Glob pattern (see supported patterns above)
 * @returns {Promise<string[]>} Sorted absolute paths of matching .md files
 */
export async function findMarkdownFiles(basePath, pattern) {
  const recursive = pattern.includes('**');
  const results = [];

  async function walk(dir) {
    const entries = await readdir(dir, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = join(dir, entry.name);
      if (entry.isDirectory()) {
        if (recursive) await walk(fullPath);
      } else if (entry.isFile() && entry.name.endsWith('.md')) {
        results.push(fullPath);
      }
    }
  }

  // Determine the starting directory from the pattern
  const parts = pattern.split('/');
  let startDir = basePath;
  for (const part of parts) {
    if (part === '**' || part.includes('*')) break;
    startDir = join(startDir, part);
  }

  try {
    const info = await stat(startDir);
    if (info.isDirectory()) {
      if (recursive) {
        await walk(startDir);
      } else {
        // Non-recursive: only top-level .md files
        const entries = await readdir(startDir, { withFileTypes: true });
        for (const entry of entries) {
          if (entry.isFile() && entry.name.endsWith('.md')) {
            results.push(join(startDir, entry.name));
          }
        }
      }
    }
  } catch {
    // Directory doesn't exist — return empty results
  }

  return results.sort();
}

/**
 * Import graph data from markdown frontmatter.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} cwd - Working directory for resolving paths
 * @param {string} pattern - Glob pattern for markdown files
 * @param {{ dryRun?: boolean }} [opts]
 * @returns {Promise<import('./import.js').ImportResult>}
 */
export async function importFromMarkdown(graph, cwd, pattern, opts = {}) {
  const files = await findMarkdownFiles(cwd, pattern);

  if (files.length === 0) {
    return {
      valid: true,
      errors: [],
      warnings: ['No markdown files found matching pattern'],
      stats: { nodes: 0, edges: 0 },
      dryRun: opts.dryRun ?? false,
    };
  }

  // Collect all nodes and edges from frontmatter
  const nodes = [];
  const edges = [];
  const warnings = [];

  for (const filePath of files) {
    const content = await readFile(filePath, 'utf-8');
    const { frontmatter } = parseFrontmatter(content);

    if (!frontmatter) {
      continue; // Skip files without frontmatter
    }

    const relPath = relative(cwd, filePath);
    const data = extractGraphData(relPath, frontmatter);
    nodes.push(data.node);

    // Ensure edge target nodes exist
    for (const edge of data.edges) {
      edges.push(edge);
    }
  }

  if (nodes.length === 0) {
    return {
      valid: true,
      errors: [],
      warnings: ['No markdown files with frontmatter found'],
      stats: { nodes: 0, edges: 0 },
      dryRun: opts.dryRun ?? false,
    };
  }

  // Collect target node IDs that aren't already sources
  const sourceIds = new Set(nodes.map(n => n.id));
  for (const edge of edges) {
    if (!sourceIds.has(edge.target)) {
      nodes.push({ id: edge.target });
      sourceIds.add(edge.target);
    }
  }

  // Build v1 import data and run through the validated pipeline
  const v1Data = { version: 1, nodes, edges };
  return importData(graph, v1Data, opts);
}
