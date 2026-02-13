/**
 * @module cli/commands
 * Command implementations for the git-mind CLI.
 */

import { execSync } from 'node:child_process';
import { writeFile, chmod, access, constants } from 'node:fs/promises';
import { join } from 'node:path';
import { initGraph, loadGraph } from '../graph.js';
import { createEdge, queryEdges, removeEdge, EDGE_TYPES } from '../edges.js';
import { getNodes, hasNode, getNode, getNodesByPrefix } from '../nodes.js';
import { computeStatus } from '../status.js';
import { importFile } from '../import.js';
import { importFromMarkdown } from '../frontmatter.js';
import { exportGraph, serializeExport, exportToFile } from '../export.js';
import { qualifyNodeId } from '../remote.js';
import { mergeFromRepo } from '../merge.js';
import { renderView, listViews } from '../views.js';
import { processCommit } from '../hooks.js';
import { getEpochForRef } from '../epoch.js';
import { runDoctor, fixIssues } from '../doctor.js';
import { generateSuggestions } from '../suggest.js';
import { getPendingSuggestions, acceptSuggestion, rejectSuggestion, skipSuggestion, batchDecision } from '../review.js';
import { success, error, info, warning, formatEdge, formatView, formatNode, formatNodeList, formatStatus, formatExportResult, formatImportResult, formatDoctorResult, formatSuggestions, formatReviewItem, formatDecisionSummary, formatAtStatus } from './format.js';

/**
 * Initialize a git-mind graph in the current repo.
 * @param {string} cwd
 */
export async function init(cwd) {
  try {
    const graph = await initGraph(cwd);
    console.log(success('Initialized git-mind graph'));
  } catch (err) {
    console.error(error(`Failed to initialize: ${err.message}`));
    process.exitCode = 1;
  }
}

/**
 * Create a link (edge) between two nodes.
 * @param {string} cwd
 * @param {string} source
 * @param {string} target
 * @param {{ type?: string, confidence?: number, remote?: string }} opts
 */
export async function link(cwd, source, target, opts = {}) {
  const type = opts.type ?? 'relates-to';

  // Qualify node IDs with remote repo if --remote is specified
  const src = opts.remote ? qualifyNodeId(source, opts.remote) : source;
  const tgt = opts.remote ? qualifyNodeId(target, opts.remote) : target;

  try {
    const graph = await loadGraph(cwd);
    await createEdge(graph, { source: src, target: tgt, type, confidence: opts.confidence });
    console.log(success(`${src} --[${type}]--> ${tgt}`));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Render a named view.
 * @param {string} cwd
 * @param {string} viewName
 */
export async function view(cwd, viewName) {
  if (!viewName) {
    console.log(info(`Available views: ${listViews().join(', ')}`));
    return;
  }

  try {
    const graph = await loadGraph(cwd);
    const result = await renderView(graph, viewName);
    console.log(formatView(viewName, result));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * List edges in the graph, optionally filtered.
 * @param {string} cwd
 * @param {{ type?: string, source?: string, target?: string }} [filter={}]
 */
export async function list(cwd, filter = {}) {
  try {
    const graph = await loadGraph(cwd);
    const edges = await queryEdges(graph, filter);

    if (edges.length === 0) {
      console.log(info('No edges found'));
      return;
    }

    console.log(info(`${edges.length} edge(s):`));
    for (const edge of edges) {
      console.log(formatEdge(edge));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Remove an edge between two nodes.
 * @param {string} cwd
 * @param {string} source
 * @param {string} target
 * @param {{ type?: string }} opts
 */
export async function remove(cwd, source, target, opts = {}) {
  const type = opts.type ?? 'relates-to';

  try {
    const graph = await loadGraph(cwd);
    await removeEdge(graph, source, target, type);
    console.log(success(`Removed: ${source} --[${type}]--> ${target}`));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Install a post-commit Git hook that processes directives.
 * @param {string} cwd
 */
export async function installHooks(cwd) {
  const hookPath = join(cwd, '.git', 'hooks', 'post-commit');

  const hookScript = `#!/bin/sh
# git-mind post-commit hook
# Parses commit directives and creates edges automatically

SHA=$(git rev-parse HEAD)
MSG=$(git log -1 --format=%B "$SHA")

# Only run if git-mind is available
command -v npx >/dev/null 2>&1 || exit 0

npx git-mind process-commit "$SHA" 2>/dev/null || true
`;

  try {
    // Check if a hook already exists
    let exists = false;
    try {
      await access(hookPath, constants.F_OK);
      exists = true;
    } catch { /* doesn't exist */ }

    if (exists) {
      console.error(error(`Post-commit hook already exists at ${hookPath}`));
      console.error(info('Remove it manually or append git-mind to the existing hook'));
      process.exitCode = 1;
      return;
    }

    await writeFile(hookPath, hookScript);
    await chmod(hookPath, 0o755);
    console.log(success('Installed post-commit hook'));
  } catch (err) {
    console.error(error(`Failed to install hook: ${err.message}`));
    process.exitCode = 1;
  }
}

/**
 * Process a commit's directives (called by post-commit hook).
 * @param {string} cwd
 * @param {string} sha
 */
export async function processCommitCmd(cwd, sha) {
  try {
    const message = execSync(`git log -1 --format=%B ${sha}`, { cwd, encoding: 'utf-8' });
    const graph = await loadGraph(cwd);
    const directives = await processCommit(graph, { sha, message });

    if (directives.length > 0) {
      for (const d of directives) {
        console.log(success(`commit:${sha.slice(0, 8)} --[${d.type}]--> ${d.target}`));
      }
    }
  } catch (err) {
    console.error(error(`Failed to process commit: ${err.message}`));
    process.exitCode = 1;
  }
}

/**
 * List and inspect nodes in the graph.
 * @param {string} cwd
 * @param {{ prefix?: string, id?: string, json?: boolean }} opts
 */
export async function nodes(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);

    // Single node detail
    if (opts.id) {
      const node = await getNode(graph, opts.id);
      if (!node) {
        console.error(error(`Node not found: ${opts.id}`));
        process.exitCode = 1;
        return;
      }
      if (opts.json) {
        console.log(JSON.stringify(node, null, 2));
      } else {
        console.log(formatNode(node));
      }
      return;
    }

    // List nodes (optionally filtered by prefix)
    const nodeList = opts.prefix
      ? await getNodesByPrefix(graph, opts.prefix)
      : await getNodes(graph);

    if (opts.json) {
      console.log(JSON.stringify(nodeList, null, 2));
      return;
    }

    if (nodeList.length === 0) {
      console.log(info('No nodes found'));
      return;
    }

    console.log(info(`${nodeList.length} node(s):`));
    console.log(formatNodeList(nodeList));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Show graph status dashboard.
 * @param {string} cwd
 * @param {{ json?: boolean }} opts
 */
export async function status(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const result = await computeStatus(graph);

    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
    } else {
      console.log(formatStatus(result));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Show the graph at a historical point in time via epoch markers.
 * @param {string} cwd
 * @param {string} ref - Git ref (HEAD, HEAD~5, branch name, SHA, etc.)
 * @param {{ json?: boolean }} opts
 */
export async function at(cwd, ref, opts = {}) {
  if (!ref) {
    console.error(error('Usage: git mind at <ref>'));
    process.exitCode = 1;
    return;
  }

  try {
    const graph = await loadGraph(cwd);
    const result = await getEpochForRef(graph, cwd, ref);

    if (!result) {
      console.error(error(`No epoch marker found for "${ref}" or any of its ancestors`));
      process.exitCode = 1;
      return;
    }

    const { sha, epoch } = result;

    // Materialize the graph at the epoch's Lamport tick
    await graph.materialize({ ceiling: epoch.tick });

    const statusResult = await computeStatus(graph);

    if (opts.json) {
      console.log(JSON.stringify({
        ref,
        sha: sha.slice(0, 8),
        fullSha: sha,
        tick: epoch.tick,
        nearest: epoch.nearest ?? false,
        recordedAt: epoch.recordedAt,
        status: statusResult,
      }, null, 2));
    } else {
      console.log(formatAtStatus(ref, sha, epoch, statusResult));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Import a YAML file into the graph.
 * @param {string} cwd
 * @param {string} filePath
 * @param {{ dryRun?: boolean, json?: boolean }} opts
 */
export async function importCmd(cwd, filePath, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const result = await importFile(graph, filePath, { dryRun: opts.dryRun });

    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
    } else {
      console.log(formatImportResult(result));
    }

    if (!result.valid) {
      process.exitCode = 1;
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Import nodes and edges from markdown frontmatter.
 * @param {string} cwd
 * @param {string} pattern - Glob pattern for markdown files
 * @param {{ dryRun?: boolean, json?: boolean }} opts
 */
export async function importMarkdownCmd(cwd, pattern, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const result = await importFromMarkdown(graph, cwd, pattern, { dryRun: opts.dryRun });

    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
    } else {
      console.log(formatImportResult(result));
    }

    if (!result.valid) {
      process.exitCode = 1;
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Export the graph to a file or stdout.
 * @param {string} cwd
 * @param {{ file?: string, format?: string, prefix?: string, json?: boolean }} opts
 */
export async function exportCmd(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const format = opts.format ?? 'yaml';

    if (opts.file) {
      const result = await exportToFile(graph, opts.file, { format, prefix: opts.prefix });

      if (opts.json) {
        console.log(JSON.stringify(result, null, 2));
      } else {
        console.log(formatExportResult(result));
      }
    } else {
      // stdout mode
      const data = await exportGraph(graph, { prefix: opts.prefix });

      if (opts.json) {
        console.log(JSON.stringify(data, null, 2));
      } else {
        const output = serializeExport(data, format);
        process.stdout.write(output);
      }
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Merge a remote repository's graph into the local graph.
 * @param {string} cwd
 * @param {{ from: string, repoName?: string, dryRun?: boolean, json?: boolean }} opts
 */
export async function mergeCmd(cwd, opts = {}) {
  if (!opts.from) {
    console.error(error('Usage: git mind merge --from <repo-path> [--repo-name <owner/name>]'));
    process.exitCode = 1;
    return;
  }

  try {
    const graph = await loadGraph(cwd);
    const result = await mergeFromRepo(graph, opts.from, {
      repoName: opts.repoName,
      dryRun: opts.dryRun,
    });

    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
    } else {
      if (result.dryRun) {
        console.log(info(`Dry run: would merge ${result.nodes} node(s), ${result.edges} edge(s) from ${result.repoName}`));
      } else {
        console.log(success(`Merged ${result.nodes} node(s), ${result.edges} edge(s) from ${result.repoName}`));
      }
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Run graph integrity checks.
 * @param {string} cwd
 * @param {{ json?: boolean, fix?: boolean }} opts
 */
export async function doctor(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const result = await runDoctor(graph);

    let fixResult;
    if (opts.fix && result.issues.length > 0) {
      fixResult = await fixIssues(graph, result.issues);
    }

    if (opts.json) {
      console.log(JSON.stringify(fixResult ? { ...result, fix: fixResult } : result, null, 2));
    } else {
      console.log(formatDoctorResult(result, fixResult));
    }

    if (result.summary.errors > 0) {
      process.exitCode = 1;
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Generate AI-powered edge suggestions.
 * @param {string} cwd
 * @param {{ agent?: string, context?: string, json?: boolean }} opts
 */
export async function suggest(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);
    const result = await generateSuggestions(cwd, graph, {
      agent: opts.agent,
      range: opts.context,
    });

    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
    } else {
      console.log(formatSuggestions(result));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Review pending suggestions interactively or in batch.
 * @param {string} cwd
 * @param {{ batch?: string, json?: boolean }} opts
 */
export async function review(cwd, opts = {}) {
  try {
    const graph = await loadGraph(cwd);

    // Batch mode
    if (opts.batch) {
      if (opts.batch !== 'accept' && opts.batch !== 'reject') {
        console.error(error('--batch must be "accept" or "reject"'));
        process.exitCode = 1;
        return;
      }

      // Individual item by index
      if (opts.index !== undefined) {
        const pending = await getPendingSuggestions(graph);
        const idx = opts.index - 1; // 1-indexed to 0-indexed
        if (idx < 0 || idx >= pending.length) {
          console.error(error(`Index ${opts.index} out of range (1-${pending.length})`));
          process.exitCode = 1;
          return;
        }
        const suggestion = pending[idx];
        const decision = opts.batch === 'accept'
          ? await acceptSuggestion(graph, suggestion)
          : await rejectSuggestion(graph, suggestion);
        const result = { processed: 1, decisions: [decision] };

        if (opts.json) {
          console.log(JSON.stringify(result, null, 2));
        } else {
          console.log(formatDecisionSummary(result));
        }
        return;
      }

      const result = await batchDecision(graph, opts.batch);

      if (opts.json) {
        console.log(JSON.stringify(result, null, 2));
      } else {
        console.log(formatDecisionSummary(result));
      }
      return;
    }

    // Interactive mode
    const pending = await getPendingSuggestions(graph);

    if (pending.length === 0) {
      console.log(info('No pending suggestions to review'));
      return;
    }

    if (opts.json) {
      console.log(JSON.stringify(pending, null, 2));
      return;
    }

    const { createInterface } = await import('node:readline');
    const rl = createInterface({ input: process.stdin, output: process.stdout });
    const ask = (q) => new Promise(resolve => rl.question(q, resolve));

    const decisions = [];

    try {
      for (let i = 0; i < pending.length; i++) {
        const item = pending[i];
        console.log('');
        console.log(formatReviewItem(item, i, pending.length));

        const answer = await ask('  [a]ccept / [r]eject / [s]kip ? ');
        const choice = answer.trim().toLowerCase();

        if (choice === 'a' || choice === 'accept') {
          const d = await acceptSuggestion(graph, item);
          decisions.push(d);
          console.log(success('Accepted'));
        } else if (choice === 'r' || choice === 'reject') {
          const d = await rejectSuggestion(graph, item);
          decisions.push(d);
          console.log(success('Rejected'));
        } else {
          const d = skipSuggestion(item);
          decisions.push(d);
          console.log(info('Skipped'));
        }
      }
    } finally {
      rl.close();
    }

    console.log('');
    console.log(formatDecisionSummary({ processed: decisions.length, decisions }));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}
