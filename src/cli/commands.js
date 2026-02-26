/**
 * @module cli/commands
 * Command implementations for the git-mind CLI.
 */

import { execFileSync } from 'node:child_process';
import { writeFile, chmod, access, constants, readFile } from 'node:fs/promises';
import { join, extname } from 'node:path';
import { initGraph } from '../graph.js';
import { createEdge, removeEdge } from '../edges.js';
import { getNode, setNodeProperty, unsetNodeProperty } from '../nodes.js';
import { computeStatus } from '../status.js';
import { importFile } from '../import.js';
import { importFromMarkdown } from '../frontmatter.js';
import { exportGraph, serializeExport, exportToFile } from '../export.js';
import { qualifyNodeId } from '../remote.js';
import { mergeFromRepo } from '../merge.js';
import { renderView, listViews } from '../views.js';
import { listLenses } from '../lens.js';
import { processCommit } from '../hooks.js';
import { getEpochForRef } from '../epoch.js';
import { runDoctor, fixIssues } from '../doctor.js';
import { generateSuggestions } from '../suggest.js';
import { getPendingSuggestions, acceptSuggestion, rejectSuggestion, skipSuggestion, batchDecision } from '../review.js';
import { computeDiff } from '../diff.js';
import { DEFAULT_CONTEXT } from '../context-envelope.js';
import { loadExtension, registerExtension, removeExtension, listExtensions, validateExtension } from '../extension.js';
import { writeContent, readContent, getContentMeta, deleteContent } from '../content.js';
import { success, error, info, formatEdge, formatView, formatNode, formatNodeList, formatStatus, formatExportResult, formatImportResult, formatDoctorResult, formatSuggestions, formatReviewItem, formatDecisionSummary, formatAtStatus, formatDiff, formatExtensionList, formatContentMeta } from './format.js';
import { GmindError } from '../errors.js';

/**
 * Write structured JSON to stdout with schemaVersion and command fields.
 * CLI layer is authoritative — schemaVersion is always forced last.
 *
 * @param {string} command - Command name for downstream routing
 * @param {object} data - Payload from the source module
 */
function outputJson(command, data) {
  const out = { ...data, schemaVersion: 1, command };
  console.log(JSON.stringify(out, null, 2));
}

/**
 * Handle an error in a CLI command. In --json mode, emits a structured
 * error envelope. Otherwise, prints to stderr via format.error().
 *
 * @param {Error} err
 * @param {{ json?: boolean }} [opts]
 */
function handleCommandError(err, opts = {}) {
  if (err instanceof GmindError) {
    if (opts.json) {
      outputJson('error', err.toJSON());
    } else {
      console.error(error(err.message));
    }
    process.exitCode = err.exitCode;
  } else {
    if (opts.json) {
      const wrapped = new GmindError('GMIND_E_INTERNAL', err.message, { cause: err });
      outputJson('error', wrapped.toJSON());
    } else {
      console.error(error(err.message));
    }
    process.exitCode = 1;
  }
}

/**
 * Resolve a ContextEnvelope to a live graph-like object.
 *
 * This is the CLI boundary — context is resolved here so domain functions
 * (renderView, computeStatus, etc.) remain clean and context-unaware.
 *
 * Resolution order:
 * 1. Load base graph.
 * 2. If asOf != 'HEAD': resolve ref → epoch tick → materialize fresh instance.
 * 3. If observer != null: read observer config from graph, create filtered view.
 *
 * @param {string} cwd - Repository working directory
 * @param {import('../context-envelope.js').ContextEnvelope} envelope
 * @returns {Promise<{ graph: object, resolvedContext: object }>}
 */
export async function resolveContext(cwd, envelope) {
  const { asOf, observer, trustPolicy, extensionLock } = envelope;

  let resolvedTick = null;
  let graph;

  if (asOf === 'HEAD') {
    graph = await initGraph(cwd, { writerId: 'ctx-reader' });
  } else {
    // Time-travel: resolve git ref → Lamport tick → materialize
    // Use a separate resolver instance so we can materialize a fresh one.
    const resolver = await initGraph(cwd, { writerId: 'ctx-resolver' });
    const result = await getEpochForRef(resolver, cwd, asOf);
    if (!result) {
      throw new GmindError('GMIND_E_NOT_FOUND',
        `No epoch marker found for "${asOf}" or any ancestor`,
        { hint: 'Run "git mind process-commit" to record epoch markers' },
      );
    }
    resolvedTick = result.epoch.tick;
    // materialize({ ceiling }) is destructive — use a dedicated instance
    graph = await initGraph(cwd, { writerId: 'ctx-temporal' });
    await graph.materialize({ ceiling: resolvedTick });
  }

  // Apply observer filter (graph-stored configs — no separate registry file)
  if (observer !== null) {
    const observerId = `observer:${observer}`;
    const propsMap = await graph.getNodeProps(observerId);
    if (!propsMap) {
      throw new GmindError('GMIND_E_NOT_FOUND',
        `Observer '${observer}' not found`,
        { hint: `Define it with: git mind set observer:${observer} match 'prefix:*'` },
      );
    }
    const config = { match: propsMap.get('match') };
    const expose = propsMap.get('expose');
    const redact = propsMap.get('redact');
    if (expose !== undefined) config.expose = expose;
    if (redact !== undefined) config.redact = redact;
    graph = await graph.observer(observer, config);
  }

  return {
    graph,
    resolvedContext: {
      asOf,
      resolvedTick,
      observer,
      trustPolicy,
      extensionLock,
    },
  };
}

/**
 * Initialize a git-mind graph in the current repo.
 * @param {string} cwd
 */
export async function init(cwd) {
  try {
    const _graph = await initGraph(cwd);
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
    const graph = await initGraph(cwd);
    await createEdge(graph, { source: src, target: tgt, type, confidence: opts.confidence });
    console.log(success(`${src} --[${type}]--> ${tgt}`));
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Render a named view, optionally with lens chaining via colon syntax.
 * @param {string} cwd
 * @param {string} viewSpec - e.g. 'roadmap', 'roadmap:incomplete', 'roadmap:incomplete:frontier'
 * @param {{ scope?: string, json?: boolean, context?: import('../context-envelope.js').ContextEnvelope }} opts
 */
export async function view(cwd, viewSpec, opts = {}) {
  if (!viewSpec) {
    const views = listViews().join(', ');
    const lenses = listLenses().join(', ');
    console.log(info(`Available views: ${views}`));
    console.log(info(`Available lenses: ${lenses}`));
    console.log(info('Compose with colons: git mind view roadmap:incomplete:frontier'));
    return;
  }

  try {
    // Split on colon — first segment is view, rest are lenses
    const segments = viewSpec.split(':');
    const viewName = segments[0];
    const lensNames = segments.slice(1).filter(s => s.length > 0);

    const envelope = opts.context ?? DEFAULT_CONTEXT;
    const { graph, resolvedContext } = await resolveContext(cwd, envelope);

    // Build view-specific options from CLI flags
    const viewOpts = {};
    if (opts.scope) {
      viewOpts.scope = opts.scope.split(',').map(s => s.trim());
    }
    if (lensNames.length > 0) {
      viewOpts.lenses = lensNames;
    }

    const result = await renderView(graph, viewName, viewOpts);

    if (opts.json) {
      const payload = { ...result, viewName };
      if (lensNames.length > 0) {
        payload.lenses = lensNames;
      }
      payload.resolvedContext = resolvedContext;
      outputJson('view', payload);
    } else {
      const displayName = lensNames.length > 0 ? viewSpec : viewName;
      console.log(formatView(displayName, result));
    }
  } catch (err) {
    handleCommandError(err, { json: opts.json });
  }
}

/**
 * List edges in the graph, optionally filtered.
 * @param {string} cwd
 * @param {{ type?: string, source?: string, target?: string }} [filter={}]
 */
export async function list(cwd, filter = {}) {
  try {
    const graph = await initGraph(cwd);
    const allEdges = await graph.getEdges();
    const edges = allEdges.filter(edge => {
      if (filter.source && edge.from !== filter.source) return false;
      if (filter.target && edge.to !== filter.target) return false;
      if (filter.type && edge.label !== filter.type) return false;
      return true;
    });

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
    const graph = await initGraph(cwd);
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
    execFileSync('git', ['rev-parse', '--verify', sha], { cwd, encoding: 'utf-8' });
    const message = execFileSync('git', ['log', '-1', '--format=%B', sha], { cwd, encoding: 'utf-8' });
    const graph = await initGraph(cwd);
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
 * @param {{ prefix?: string, id?: string, json?: boolean, context?: import('../context-envelope.js').ContextEnvelope }} opts
 */
export async function nodes(cwd, opts = {}) {
  try {
    const envelope = opts.context ?? DEFAULT_CONTEXT;
    const { graph, resolvedContext } = await resolveContext(cwd, envelope);

    // Single node detail
    if (opts.id) {
      const node = await getNode(graph, opts.id);
      if (!node) {
        handleCommandError(
          new GmindError('GMIND_E_NODE_NOT_FOUND', `Node not found: ${opts.id}`),
          { json: opts.json },
        );
        return;
      }
      if (opts.json) {
        outputJson('nodes', { ...node, resolvedContext });
      } else {
        console.log(formatNode(node));
      }
      return;
    }

    // List nodes (optionally filtered by prefix)
    const allNodes = await graph.getNodes();
    const nodeList = opts.prefix
      ? allNodes.filter(n => n.startsWith(opts.prefix + ':'))
      : allNodes;

    if (opts.json) {
      outputJson('nodes', { nodes: nodeList, resolvedContext });
      return;
    }

    if (nodeList.length === 0) {
      console.log(info('No nodes found'));
      return;
    }

    console.log(info(`${nodeList.length} node(s):`));
    console.log(formatNodeList(nodeList));
  } catch (err) {
    handleCommandError(err, { json: opts.json });
  }
}

/**
 * Show graph status dashboard.
 * @param {string} cwd
 * @param {{ json?: boolean, context?: import('../context-envelope.js').ContextEnvelope }} opts
 */
export async function status(cwd, opts = {}) {
  try {
    const envelope = opts.context ?? DEFAULT_CONTEXT;
    const { graph, resolvedContext } = await resolveContext(cwd, envelope);
    const result = await computeStatus(graph);

    if (opts.json) {
      outputJson('status', { ...result, resolvedContext });
    } else {
      console.log(formatStatus(result));
    }
  } catch (err) {
    handleCommandError(err, { json: opts.json });
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
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind at <ref>'), { json: opts.json });
    return;
  }

  try {
    const graph = await initGraph(cwd);
    const result = await getEpochForRef(graph, cwd, ref);

    if (!result) {
      handleCommandError(
        new GmindError('GMIND_E_NOT_FOUND', `No epoch marker found for "${ref}" or any of its ancestors`, {
          hint: 'Run "git mind process-commit" to record epoch markers',
        }),
        { json: opts.json },
      );
      return;
    }

    const { sha, epoch } = result;

    // Materialize the graph at the epoch's Lamport tick
    await graph.materialize({ ceiling: epoch.tick });

    const statusResult = await computeStatus(graph);

    if (opts.json) {
      outputJson('at', {
        ref,
        sha: sha.slice(0, 8),
        fullSha: sha,
        tick: epoch.tick,
        nearest: epoch.nearest ?? false,
        recordedAt: epoch.recordedAt,
        status: statusResult,
      });
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
    const graph = await initGraph(cwd);
    const result = await importFile(graph, filePath, { dryRun: opts.dryRun });

    if (opts.json) {
      outputJson('import', result);
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
    const graph = await initGraph(cwd);
    const result = await importFromMarkdown(graph, cwd, pattern, { dryRun: opts.dryRun });

    if (opts.json) {
      outputJson('import', result);
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
 * @param {{ file?: string, format?: string, prefix?: string, json?: boolean, context?: import('../context-envelope.js').ContextEnvelope }} opts
 */
export async function exportCmd(cwd, opts = {}) {
  try {
    const envelope = opts.context ?? DEFAULT_CONTEXT;
    const { graph, resolvedContext } = await resolveContext(cwd, envelope);
    const format = opts.format ?? 'yaml';

    if (opts.file) {
      const result = await exportToFile(graph, opts.file, { format, prefix: opts.prefix });

      if (opts.json) {
        outputJson('export', { ...result, resolvedContext });
      } else {
        console.log(formatExportResult(result));
      }
    } else {
      // stdout mode
      const data = await exportGraph(graph, { prefix: opts.prefix });

      if (opts.json) {
        outputJson('export', { ...data, resolvedContext });
      } else {
        const output = serializeExport(data, format);
        process.stdout.write(output);
      }
    }
  } catch (err) {
    handleCommandError(err, { json: opts.json });
  }
}

/**
 * Merge a remote repository's graph into the local graph.
 * @param {string} cwd
 * @param {{ from: string, repoName?: string, dryRun?: boolean, json?: boolean }} opts
 */
export async function mergeCmd(cwd, opts = {}) {
  if (!opts.from) {
    handleCommandError(
      new GmindError('GMIND_E_USAGE', 'Usage: git mind merge --from <repo-path> [--repo-name <owner/name>]'),
      { json: opts.json },
    );
    return;
  }

  try {
    const graph = await initGraph(cwd);
    const result = await mergeFromRepo(graph, opts.from, {
      repoName: opts.repoName,
      dryRun: opts.dryRun,
    });

    if (opts.json) {
      outputJson('merge', result);
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
 * @param {{ json?: boolean, fix?: boolean, context?: import('../context-envelope.js').ContextEnvelope }} opts
 */
export async function doctor(cwd, opts = {}) {
  try {
    const envelope = opts.context ?? DEFAULT_CONTEXT;
    const { graph, resolvedContext } = await resolveContext(cwd, envelope);
    const result = await runDoctor(graph);

    let fixResult;
    if (opts.fix && result.issues.length > 0) {
      fixResult = await fixIssues(graph, result.issues);
    }

    if (opts.json) {
      const payload = fixResult ? { ...result, fix: fixResult } : result;
      outputJson('doctor', { ...payload, resolvedContext });
    } else {
      console.log(formatDoctorResult(result, fixResult));
    }

    if (result.summary.errors > 0) {
      process.exitCode = 1;
    }
  } catch (err) {
    handleCommandError(err, { json: opts.json });
  }
}

/**
 * Generate AI-powered edge suggestions.
 * @param {string} cwd
 * @param {{ agent?: string, context?: string, json?: boolean }} opts
 */
export async function suggest(cwd, opts = {}) {
  try {
    const graph = await initGraph(cwd);
    const result = await generateSuggestions(cwd, graph, {
      agent: opts.agent,
      range: opts.context,
    });

    if (opts.json) {
      outputJson('suggest', result);
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
    const graph = await initGraph(cwd);

    // Batch mode
    if (opts.batch) {
      if (opts.batch !== 'accept' && opts.batch !== 'reject') {
        handleCommandError(
          new GmindError('GMIND_E_USAGE', '--batch must be "accept" or "reject"'),
          { json: opts.json },
        );
        return;
      }

      // Individual item by index
      if (opts.index !== undefined) {
        const pending = await getPendingSuggestions(graph);
        if (pending.length === 0) {
          handleCommandError(
            new GmindError('GMIND_E_NOT_FOUND', 'No pending suggestions to review'),
            { json: opts.json },
          );
          return;
        }
        const idx = opts.index - 1; // 1-indexed to 0-indexed
        if (idx < 0 || idx >= pending.length) {
          handleCommandError(
            new GmindError('GMIND_E_USAGE', `Index ${opts.index} out of range (1-${pending.length})`),
            { json: opts.json },
          );
          return;
        }
        const suggestion = pending[idx];
        const decision = opts.batch === 'accept'
          ? await acceptSuggestion(graph, suggestion)
          : await rejectSuggestion(graph, suggestion);
        const result = { processed: 1, decisions: [decision] };

        if (opts.json) {
          outputJson('review', result);
        } else {
          console.log(formatDecisionSummary(result));
        }
        return;
      }

      const result = await batchDecision(graph, opts.batch);

      if (opts.json) {
        outputJson('review', result);
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
      outputJson('review', { pending });
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

/**
 * Set a property on a node.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {string} key
 * @param {string} value
 * @param {{ json?: boolean }} opts
 */
export async function set(cwd, nodeId, key, value, opts = {}) {
  if (!nodeId || !key || value === undefined) {
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind set <nodeId> <key> <value>'), { json: opts.json });
    return;
  }

  try {
    const graph = await initGraph(cwd);
    const result = await setNodeProperty(graph, nodeId, key, value);

    if (opts.json) {
      outputJson('set', result);
    } else {
      if (result.changed) {
        console.log(success(`${nodeId}.${key} = ${value}`));
      } else {
        console.log(info(`${nodeId}.${key} = ${value} (unchanged)`));
      }
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Remove a property from a node.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {string} key
 * @param {{ json?: boolean }} opts
 */
export async function unsetCmd(cwd, nodeId, key, opts = {}) {
  if (!nodeId || !key) {
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind unset <nodeId> <key>'), { json: opts.json });
    return;
  }

  try {
    const graph = await initGraph(cwd);
    const result = await unsetNodeProperty(graph, nodeId, key);

    if (opts.json) {
      outputJson('unset', result);
    } else {
      if (result.removed) {
        console.log(success(`${nodeId}.${key} removed`));
      } else {
        console.log(info(`${nodeId}.${key} was not set`));
      }
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Show graph diff between two commits.
 * @param {string} cwd
 * @param {string} refA
 * @param {string} refB
 * @param {{ json?: boolean, prefix?: string }} opts
 */
export async function diff(cwd, refA, refB, opts = {}) {
  try {
    const result = await computeDiff(cwd, refA, refB, { prefix: opts.prefix });

    if (opts.json) {
      outputJson('diff', result);
    } else {
      console.log(formatDiff(result));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

// ── Content commands ─────────────────────────────────────────────

/** MIME type mapping from file extensions. */
const MIME_MAP = {
  '.md': 'text/markdown',
  '.markdown': 'text/markdown',
  '.txt': 'text/plain',
  '.json': 'application/json',
  '.yaml': 'application/yaml',
  '.yml': 'application/yaml',
  '.html': 'text/html',
  '.xml': 'application/xml',
  '.csv': 'text/csv',
  '.css': 'text/css',
  '.svg': 'image/svg+xml',
};

/**
 * Attach content to a graph node from a file.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {string} filePath
 * @param {{ mime?: string, json?: boolean }} opts
 */
export async function contentSet(cwd, nodeId, filePath, opts = {}) {
  try {
    const buf = await readFile(filePath);
    const mime = opts.mime ?? MIME_MAP[extname(filePath).toLowerCase()] ?? 'application/octet-stream';

    const graph = await initGraph(cwd);
    const result = await writeContent(graph, nodeId, buf, { mime });

    if (opts.json) {
      outputJson('content-set', result);
    } else {
      console.log(success(`Content attached to ${nodeId}`));
      console.log(formatContentMeta(result));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Show content attached to a graph node.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {{ raw?: boolean, json?: boolean }} opts
 */
export async function contentShow(cwd, nodeId, opts = {}) {
  try {
    const graph = await initGraph(cwd);
    const { content, meta } = await readContent(graph, nodeId);

    if (opts.json) {
      outputJson('content-show', { nodeId, content, ...meta });
      return;
    }

    if (opts.raw) {
      process.stdout.write(content);
    } else {
      console.log(formatContentMeta({ nodeId, ...meta }));
      console.log('');
      console.log(content);
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Show content metadata for a graph node.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {{ json?: boolean }} opts
 */
export async function contentMeta(cwd, nodeId, opts = {}) {
  try {
    const graph = await initGraph(cwd);
    const meta = await getContentMeta(graph, nodeId);

    if (!meta) {
      if (opts.json) {
        outputJson('content-meta', { nodeId, hasContent: false });
      } else {
        console.log(info(`No content attached to ${nodeId}`));
      }
      return;
    }

    if (opts.json) {
      outputJson('content-meta', { nodeId, hasContent: true, ...meta });
    } else {
      console.log(formatContentMeta({ nodeId, ...meta }));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Delete content from a graph node.
 * @param {string} cwd
 * @param {string} nodeId
 * @param {{ json?: boolean }} opts
 */
export async function contentDelete(cwd, nodeId, opts = {}) {
  try {
    const graph = await initGraph(cwd);
    const result = await deleteContent(graph, nodeId);

    if (opts.json) {
      outputJson('content-delete', result);
    } else if (result.removed) {
      console.log(success(`Content removed from ${nodeId}`));
    } else {
      console.log(info(`No content to remove from ${nodeId}`));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

// ── Extension commands ───────────────────────────────────────────

/**
 * List all registered extensions.
 * @param {string} _cwd
 * @param {{ json?: boolean }} opts
 */
export function extensionList(_cwd, opts = {}) {
  const extensions = listExtensions();
  if (opts.json) {
    outputJson('extension-list', {
      extensions: extensions.map(ext => ({
        name: ext.name,
        version: ext.version,
        description: ext.description,
        builtin: ext.builtin,
        views: ext.views,
        lenses: ext.lenses,
      })),
    });
    return;
  }
  if (extensions.length === 0) {
    console.log(info('No extensions registered.'));
    return;
  }
  console.log(formatExtensionList(extensions));
}

/**
 * Validate an extension manifest file without registering it.
 * @param {string} _cwd
 * @param {string} manifestPath
 * @param {{ json?: boolean }} opts
 */
export async function extensionValidate(_cwd, manifestPath, opts = {}) {
  if (!manifestPath) {
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind extension validate <manifest-path>'), { json: opts.json });
    return;
  }
  const result = await validateExtension(manifestPath);
  if (opts.json) {
    outputJson('extension-validate', result);
    return;
  }
  if (result.valid) {
    console.log(success(`Valid extension manifest: ${result.record.name} v${result.record.version}`));
  } else {
    console.error(error(`Invalid manifest: ${result.errors.join('; ')}`));
    process.exitCode = 1;
  }
}

/**
 * Load and register an extension from a manifest file.
 * @param {string} _cwd
 * @param {string} manifestPath
 * @param {{ json?: boolean }} opts
 */
export async function extensionAdd(_cwd, manifestPath, opts = {}) {
  if (!manifestPath) {
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind extension add <manifest-path>'), { json: opts.json });
    return;
  }
  try {
    const record = await loadExtension(manifestPath);
    registerExtension(record);
    if (opts.json) {
      outputJson('extension-add', {
        name: record.name,
        version: record.version,
        views: record.views.map(v => v.name),
        lenses: record.lenses,
      });
    } else {
      console.log(success(`Registered extension: ${record.name} v${record.version}`));
      if (record.views.length > 0) {
        console.log(info(`Views declared: ${record.views.map(v => v.name).join(', ')}`));
      }
      if (record.lenses.length > 0) {
        console.log(info(`Lenses available: ${record.lenses.join(', ')}`));
      }
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}

/**
 * Remove a registered extension by name.
 * @param {string} _cwd
 * @param {string} name
 * @param {{ json?: boolean }} opts
 */
export function extensionRemove(_cwd, name, opts = {}) {
  if (!name) {
    handleCommandError(new GmindError('GMIND_E_USAGE', 'Usage: git mind extension remove <name>'), { json: opts.json });
    return;
  }
  try {
    const record = removeExtension(name);
    if (opts.json) {
      outputJson('extension-remove', {
        name: record.name,
        version: record.version,
      });
    } else {
      console.log(success(`Removed extension: ${record.name} v${record.version}`));
    }
  } catch (err) {
    console.error(error(err.message));
    process.exitCode = 1;
  }
}
