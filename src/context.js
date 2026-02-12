/**
 * @module context
 * Git context extraction for AI-powered suggestions.
 * Builds structured prompts from repository state and graph data.
 */

import { execSync } from 'node:child_process';
import { EDGE_TYPES, CANONICAL_PREFIXES } from './validators.js';

/**
 * @typedef {object} FileContext
 * @property {string} path - File path relative to repo root
 * @property {string} language - Inferred language from extension
 */

/**
 * @typedef {object} CommitContext
 * @property {string} sha - Short SHA
 * @property {string} message - First line of commit message
 * @property {string[]} files - Changed files in this commit
 */

/**
 * @typedef {object} GraphContext
 * @property {string[]} nodes - Existing node IDs
 * @property {Array<{from: string, to: string, label: string}>} edges - Existing edges
 */

/**
 * @typedef {object} SuggestContext
 * @property {FileContext[]} files
 * @property {CommitContext[]} commits
 * @property {GraphContext} graph
 * @property {string} prompt - Assembled LLM prompt
 */

const LANGUAGE_MAP = {
  '.js': 'javascript',
  '.mjs': 'javascript',
  '.cjs': 'javascript',
  '.ts': 'typescript',
  '.tsx': 'typescript',
  '.jsx': 'javascript',
  '.py': 'python',
  '.rb': 'ruby',
  '.rs': 'rust',
  '.go': 'go',
  '.java': 'java',
  '.c': 'c',
  '.cpp': 'cpp',
  '.h': 'c',
  '.hpp': 'cpp',
  '.cs': 'csharp',
  '.sh': 'shell',
  '.bash': 'shell',
  '.zsh': 'shell',
  '.md': 'markdown',
  '.yaml': 'yaml',
  '.yml': 'yaml',
  '.json': 'json',
  '.toml': 'toml',
  '.css': 'css',
  '.html': 'html',
  '.sql': 'sql',
};

/**
 * Infer language from file extension.
 * @param {string} filePath
 * @returns {string}
 */
function inferLanguage(filePath) {
  const dot = filePath.lastIndexOf('.');
  if (dot === -1) return 'unknown';
  const ext = filePath.slice(dot).toLowerCase();
  return LANGUAGE_MAP[ext] ?? 'unknown';
}

/**
 * Extract tracked file paths from git.
 *
 * @param {string} cwd - Repository root
 * @param {{ limit?: number }} [opts={}]
 * @returns {FileContext[]}
 */
export function extractFileContext(cwd, opts = {}) {
  const limit = opts.limit ?? 200;
  try {
    const output = execSync('git ls-files', { cwd, encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] });
    const files = output.trim().split('\n').filter(Boolean).slice(0, limit);
    return files.map(path => ({ path, language: inferLanguage(path) }));
  } catch {
    return [];
  }
}

/**
 * Extract recent commit context from git log.
 *
 * @param {string} cwd - Repository root
 * @param {{ range?: string, limit?: number }} [opts={}]
 * @returns {CommitContext[]}
 */
/** Validate that a string is safe for use as a git command argument. */
function sanitizeGitArg(value) {
  if (/[;&|`$(){}!#<>\n\r]/.test(value)) {
    throw new Error(`Unsafe characters in git argument: ${value}`);
  }
  return value;
}

export function extractCommitContext(cwd, opts = {}) {
  const limit = Math.max(1, Math.min(Number.parseInt(opts.limit ?? 10, 10) || 10, 100));
  const range = sanitizeGitArg(opts.range ?? `HEAD~${limit}..HEAD`);

  try {
    // Get commits with short sha and first line of message
    const logOutput = execSync(
      `git log --format="%h %s" ${range} 2>/dev/null || git log --format="%h %s" -${limit}`,
      { cwd, encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] }
    );
    const lines = logOutput.trim().split('\n').filter(Boolean);

    return lines.map(line => {
      const spaceIdx = line.indexOf(' ');
      const sha = line.slice(0, spaceIdx);
      const message = line.slice(spaceIdx + 1);

      // Validate sha is a hex string before interpolation
      if (!/^[0-9a-f]+$/.test(sha)) return { sha, message, files: [] };

      // Get changed files for this commit
      let files = [];
      try {
        const filesOutput = execSync(
          `git diff-tree --no-commit-id --name-only -r ${sha}`,
          { cwd, encoding: 'utf-8', stdio: ['pipe', 'pipe', 'pipe'] }
        );
        files = filesOutput.trim().split('\n').filter(Boolean);
      } catch {
        // Ignore — might be initial commit
      }

      return { sha, message, files };
    });
  } catch {
    return [];
  }
}

/**
 * Extract graph context (existing nodes and edges, optionally filtered by file paths).
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string[]} [filePaths]
 * @returns {Promise<GraphContext>}
 */
export async function extractGraphContext(graph, filePaths) {
  const nodes = await graph.getNodes();
  const edges = await graph.getEdges();

  if (!filePaths || filePaths.length === 0) {
    return { nodes, edges };
  }

  // Find nodes related to the given file paths
  const fileNodeSet = new Set();
  for (const fp of filePaths) {
    for (const node of nodes) {
      if (node === `file:${fp}` || (node.startsWith('file:') && node.endsWith(`/${fp}`))) {
        fileNodeSet.add(node);
      }
    }
  }

  // Include nodes connected to file nodes
  const relatedNodes = new Set(fileNodeSet);
  for (const e of edges) {
    if (fileNodeSet.has(e.from)) relatedNodes.add(e.to);
    if (fileNodeSet.has(e.to)) relatedNodes.add(e.from);
  }

  const filteredEdges = edges.filter(
    e => relatedNodes.has(e.from) || relatedNodes.has(e.to)
  );

  return {
    nodes: nodes.filter(n => relatedNodes.has(n)),
    edges: filteredEdges,
  };
}

/**
 * Build an LLM prompt from extracted context.
 *
 * @param {SuggestContext} context
 * @param {{ maxLength?: number }} [opts={}]
 * @returns {string}
 */
export function buildPrompt(context, opts = {}) {
  const maxLength = opts.maxLength ?? 4000;
  const parts = [];
  const graph = context.graph ?? { nodes: [], edges: [] };
  const commits = context.commits ?? [];
  const files = context.files ?? [];

  parts.push('You are a knowledge graph assistant for a software project.');
  parts.push('Suggest new semantic edges for the project knowledge graph.');
  parts.push('');
  parts.push('## Graph Schema');
  parts.push(`Node prefixes: ${CANONICAL_PREFIXES.join(', ')}`);
  parts.push(`Edge types: ${EDGE_TYPES.join(', ')}`);
  parts.push('Node IDs use the format: prefix:identifier (e.g., file:src/auth.js, spec:auth-flow)');
  parts.push('');

  // Existing graph
  if (graph.nodes.length > 0) {
    parts.push('## Existing Nodes');
    const nodeSlice = graph.nodes.slice(0, 50);
    parts.push(nodeSlice.join(', '));
    if (graph.nodes.length > 50) {
      parts.push(`... and ${graph.nodes.length - 50} more`);
    }
    parts.push('');
  }

  if (graph.edges.length > 0) {
    parts.push('## Existing Edges');
    const edgeSlice = graph.edges.slice(0, 30);
    for (const e of edgeSlice) {
      parts.push(`  ${e.from} --[${e.label}]--> ${e.to}`);
    }
    if (graph.edges.length > 30) {
      parts.push(`... and ${graph.edges.length - 30} more`);
    }
    parts.push('');
  }

  // Recent commits
  if (commits.length > 0) {
    parts.push('## Recent Commits');
    for (const c of commits) {
      parts.push(`  ${c.sha} ${c.message}`);
      if (c.files.length > 0) {
        parts.push(`    files: ${c.files.slice(0, 5).join(', ')}${c.files.length > 5 ? ' ...' : ''}`);
      }
    }
    parts.push('');
  }

  // Files
  if (files.length > 0) {
    parts.push('## Tracked Files');
    const fileSlice = files.slice(0, 30);
    parts.push(fileSlice.map(f => f.path).join(', '));
    if (files.length > 30) {
      parts.push(`... and ${files.length - 30} more`);
    }
    parts.push('');
  }

  parts.push('## Instructions');
  parts.push('Respond with a JSON array of suggested edges. Each suggestion:');
  parts.push('```json');
  parts.push('[');
  parts.push('  { "source": "prefix:id", "target": "prefix:id", "type": "edge-type", "confidence": 0.7, "rationale": "why" }');
  parts.push(']');
  parts.push('```');
  parts.push('');
  parts.push('Rules:');
  parts.push('- Use existing node IDs when possible, or suggest new ones with valid prefixes');
  parts.push('- Confidence 0.0-1.0 (higher = more certain)');
  parts.push('- Do NOT duplicate existing edges');
  parts.push('- Focus on edges that capture meaningful project relationships');

  let prompt = parts.join('\n');

  // Truncate if too long
  if (prompt.length > maxLength) {
    prompt = prompt.slice(0, maxLength - 20) + '\n\n[truncated]';
  }

  return prompt;
}

/**
 * Orchestrator: extract all context from git state and graph.
 *
 * @param {string} cwd - Repository root
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {{ range?: string, limit?: number }} [opts={}]
 * @returns {Promise<SuggestContext>}
 */
export async function extractContext(cwd, graph, opts = {}) {
  const files = extractFileContext(cwd, opts);
  const commits = extractCommitContext(cwd, opts);
  const filePaths = files.map(f => f.path);
  const graphCtx = await extractGraphContext(graph, filePaths);

  const context = { files, commits, graph: graphCtx };
  const prompt = buildPrompt(context, opts);

  return { ...context, prompt };
}
