/**
 * @module cli/format
 * Terminal output formatting for git-mind CLI.
 */

import chalk from 'chalk';
import figures from 'figures';

/**
 * Render a sorted key→count table (shared by formatStatus and formatAtStatus).
 * @param {Record<string, number>} entries - Key→count map
 * @param {string[]} lines - Output array to push into
 * @param {{ pct?: number }} [opts] - If pct is provided, show percentage based on total
 */
function renderCountTable(entries, lines, opts = {}) {
  const sorted = Object.entries(entries).sort(([, a], [, b]) => b - a);
  for (const [key, count] of sorted) {
    let suffix = '';
    if (opts.pct !== undefined && opts.pct > 0) {
      suffix = `  ${chalk.dim(`(${Math.round((count / opts.pct) * 100)}%)`)}`;
    }
    lines.push(`  ${chalk.yellow(key.padEnd(14))} ${String(count).padStart(3)}${suffix}`);
  }
}

/**
 * Format a success message.
 * @param {string} msg
 * @returns {string}
 */
export function success(msg) {
  return `${chalk.green(figures.tick)} ${msg}`;
}

/**
 * Format an error message.
 * @param {string} msg
 * @returns {string}
 */
export function error(msg) {
  return `${chalk.red(figures.cross)} ${msg}`;
}

/**
 * Format an info message.
 * @param {string} msg
 * @returns {string}
 */
export function info(msg) {
  return `${chalk.blue(figures.info)} ${msg}`;
}

/**
 * Format a warning message.
 * @param {string} msg
 * @returns {string}
 */
export function warning(msg) {
  return `${chalk.yellow(figures.warning)} ${msg}`;
}

/**
 * Format an edge for terminal display.
 * @param {{from: string, to: string, label: string, props?: object}} edge
 * @returns {string}
 */
export function formatEdge(edge) {
  const conf = edge.props?.confidence;
  const confStr = typeof conf === 'number' ? ` ${chalk.dim(`(${(conf * 100).toFixed(0)}%)`)}` : '';
  return `  ${chalk.cyan(edge.from)} ${chalk.dim('--[')}${chalk.yellow(edge.label)}${chalk.dim(']-->')} ${chalk.cyan(edge.to)}${confStr}`;
}

/**
 * Format a view result for terminal display.
 * @param {string} viewName
 * @param {{nodes: string[], edges: Array<{from: string, to: string, label: string, props?: object}>}} result
 * @returns {string}
 */
export function formatView(viewName, result) {
  // Progress view has its own formatter
  if (result.meta?.summary && typeof result.meta.summary.pct === 'number') {
    return formatProgressMeta(result.meta);
  }

  const lines = [];
  lines.push(chalk.bold(`View: ${viewName}`));
  lines.push(`${chalk.dim(`${result.nodes.length} nodes, ${result.edges.length} edges`)}`);
  lines.push('');

  if (result.edges.length === 0 && result.nodes.length === 0) {
    lines.push(chalk.dim('  (empty)'));
  } else {
    for (const edge of result.edges) {
      lines.push(formatEdge(edge));
    }
    // Show orphan nodes (no edges)
    const connectedNodes = new Set();
    for (const e of result.edges) {
      connectedNodes.add(e.from);
      connectedNodes.add(e.to);
    }
    const orphans = result.nodes.filter(n => !connectedNodes.has(n));
    if (orphans.length > 0) {
      lines.push('');
      lines.push(chalk.dim('  Unconnected:'));
      for (const n of orphans) {
        lines.push(`    ${chalk.cyan(n)}`);
      }
    }
  }

  return lines.join('\n');
}

/**
 * Format a single node for terminal display.
 * @param {import('../nodes.js').NodeInfo} node
 * @returns {string}
 */
export function formatNode(node) {
  const lines = [];
  lines.push(`  ${chalk.cyan(node.id)}`);
  lines.push(`    prefix: ${chalk.yellow(node.prefix)} ${chalk.dim(`(${node.prefixClass})`)}`);

  const propKeys = Object.keys(node.properties);
  if (propKeys.length > 0) {
    lines.push(`    properties:`);
    for (const key of propKeys) {
      lines.push(`      ${chalk.dim(key + ':')} ${node.properties[key]}`);
    }
  }

  return lines.join('\n');
}

/**
 * Format a list of node IDs for terminal display.
 * @param {string[]} nodes
 * @returns {string}
 */
export function formatNodeList(nodes) {
  if (nodes.length === 0) return chalk.dim('  (none)');
  return nodes.map(n => `  ${chalk.cyan(n)}`).join('\n');
}

/**
 * Format a graph status summary for terminal display.
 * @param {import('../status.js').GraphStatus} status
 * @returns {string}
 */
export function formatStatus(status) {
  const lines = [];

  // Header
  lines.push(chalk.bold('Graph Status'));
  lines.push(chalk.dim('═'.repeat(32)));
  lines.push('');

  // Nodes section
  lines.push(`${chalk.bold('Nodes:')} ${status.nodes.total}`);
  renderCountTable(status.nodes.byPrefix, lines, { pct: status.nodes.total });
  lines.push('');

  // Edges section
  lines.push(`${chalk.bold('Edges:')} ${status.edges.total}`);
  renderCountTable(status.edges.byType, lines);
  lines.push('');

  // Health section
  lines.push(chalk.bold('Health'));
  const { blockedItems, lowConfidence, orphanNodes } = status.health;

  if (blockedItems > 0) {
    lines.push(`  ${chalk.yellow(figures.warning)} ${blockedItems} blocked item(s)`);
  } else {
    lines.push(`  ${chalk.green(figures.tick)} No blocked items`);
  }

  if (lowConfidence > 0) {
    lines.push(`  ${chalk.yellow(figures.warning)} ${lowConfidence} low-confidence edge(s)`);
  } else {
    lines.push(`  ${chalk.green(figures.tick)} No low-confidence edges`);
  }

  if (orphanNodes > 0) {
    lines.push(`  ${chalk.yellow(figures.warning)} ${orphanNodes} orphan node(s)`);
  } else {
    lines.push(`  ${chalk.green(figures.tick)} No orphan nodes`);
  }

  return lines.join('\n');
}

/**
 * Format a doctor result for terminal display.
 * @param {import('../doctor.js').DoctorResult} result
 * @param {{ fixed?: number, skipped?: number, details?: string[] }} [fixResult]
 * @returns {string}
 */
export function formatDoctorResult(result, fixResult) {
  const lines = [];

  lines.push(chalk.bold('Doctor'));
  lines.push(chalk.dim('═'.repeat(32)));
  lines.push('');

  if (result.clean) {
    lines.push(`${chalk.green(figures.tick)} Graph is healthy — no issues found`);
  } else {
    for (const issue of result.issues) {
      const icon = issue.severity === 'error'
        ? chalk.red(figures.cross)
        : issue.severity === 'warning'
          ? chalk.yellow(figures.warning)
          : chalk.blue(figures.info);
      lines.push(`${icon} ${issue.message}`);
    }

    lines.push('');
    lines.push(chalk.dim(
      `${result.summary.errors} error(s), ${result.summary.warnings} warning(s), ${result.summary.info} info`
    ));
  }

  if (fixResult) {
    lines.push('');
    lines.push(chalk.bold('Fix Results'));
    lines.push(`  ${chalk.green(figures.tick)} Fixed: ${fixResult.fixed}`);
    lines.push(`  ${chalk.dim('Skipped:')} ${fixResult.skipped}`);
    for (const detail of fixResult.details ?? []) {
      lines.push(`  ${chalk.dim('·')} ${detail}`);
    }
  }

  return lines.join('\n');
}

/**
 * Format a list of suggestions for terminal display.
 * @param {import('../suggest.js').SuggestResult} result
 * @returns {string}
 */
export function formatSuggestions(result) {
  const lines = [];

  lines.push(chalk.bold('Suggestions'));
  lines.push(chalk.dim('═'.repeat(32)));
  lines.push('');

  if (result.suggestions.length === 0) {
    lines.push(chalk.dim('  No suggestions generated'));
  } else {
    for (const s of result.suggestions) {
      const confStr = chalk.dim(`(${(s.confidence * 100).toFixed(0)}%)`);
      lines.push(`  ${chalk.cyan(s.source)} ${chalk.dim('--[')}${chalk.yellow(s.type)}${chalk.dim(']-->')} ${chalk.cyan(s.target)} ${confStr}`);
      if (s.rationale) {
        lines.push(`    ${chalk.dim(s.rationale)}`);
      }
    }
  }

  if (result.errors?.length > 0) {
    lines.push('');
    lines.push(chalk.yellow(`${result.errors.length} parse error(s):`));
    for (const err of result.errors) {
      lines.push(`  ${chalk.yellow(figures.warning)} ${err}`);
    }
  }

  return lines.join('\n');
}

/**
 * Format a single review item for terminal display.
 * @param {import('../review.js').PendingSuggestion} item
 * @param {number} index
 * @param {number} total
 * @returns {string}
 */
export function formatReviewItem(item, index, total) {
  const lines = [];
  lines.push(chalk.bold(`Review [${index + 1}/${total}]`));
  lines.push(chalk.dim('─'.repeat(32)));
  const confStr = chalk.dim(`(${(item.confidence * 100).toFixed(0)}%)`);
  lines.push(`  ${chalk.cyan(item.source)} ${chalk.dim('--[')}${chalk.yellow(item.type)}${chalk.dim(']-->')} ${chalk.cyan(item.target)} ${confStr}`);
  if (item.rationale) {
    lines.push(`  ${chalk.dim('Rationale:')} ${item.rationale}`);
  }
  return lines.join('\n');
}

/**
 * Format a decision summary for terminal display.
 * @param {{ processed: number, decisions: import('../review.js').ReviewDecision[] }} result
 * @returns {string}
 */
export function formatDecisionSummary(result) {
  const lines = [];

  lines.push(chalk.bold('Review Summary'));
  lines.push(chalk.dim('═'.repeat(32)));
  lines.push('');

  if (result.processed === 0) {
    lines.push(chalk.dim('  No pending suggestions to review'));
  } else {
    const counts = {};
    for (const d of result.decisions ?? []) {
      counts[d.action] = (counts[d.action] ?? 0) + 1;
    }
    lines.push(`  ${chalk.green(figures.tick)} Processed ${result.processed} suggestion(s)`);
    for (const [action, count] of Object.entries(counts)) {
      const icon = action === 'accept' ? chalk.green(figures.tick)
        : action === 'reject' ? chalk.red(figures.cross)
          : chalk.blue(figures.info);
      lines.push(`    ${icon} ${action}: ${count}`);
    }
  }

  return lines.join('\n');
}

/**
 * Format an export result for terminal display.
 * @param {{stats: {nodes: number, edges: number}, path?: string}} result
 * @param {boolean} [toStdout=false]
 * @returns {string}
 */
export function formatExportResult(result, toStdout = false) {
  if (toStdout) {
    return `${chalk.green(figures.tick)} Exported ${result.stats.nodes} node(s), ${result.stats.edges} edge(s)`;
  }
  return `${chalk.green(figures.tick)} Exported ${result.stats.nodes} node(s), ${result.stats.edges} edge(s) to ${chalk.cyan(result.path)}`;
}

/**
 * Format an `at` (time-travel) status for terminal display.
 * @param {string} ref - The git ref that was resolved
 * @param {string} sha - Resolved commit SHA
 * @param {import('../epoch.js').EpochInfo} epoch - Epoch marker info
 * @param {import('../status.js').GraphStatus} status - Computed status at that tick
 * @returns {string}
 */
export function formatAtStatus(ref, sha, epoch, status) {
  const lines = [];

  lines.push(chalk.bold(`Graph at ${ref}`));
  const shaStr = `commit ${chalk.cyan(sha.slice(0, 8))}`;
  const tickStr = `tick ${chalk.yellow(String(epoch.tick))}`;
  const nearestStr = epoch.nearest ? chalk.dim('  (nearest epoch)') : '';
  lines.push(`${shaStr}  ${tickStr}${nearestStr}`);
  lines.push(chalk.dim('═'.repeat(32)));
  lines.push('');

  // Nodes section
  lines.push(`${chalk.bold('Nodes:')} ${status.nodes.total}`);
  renderCountTable(status.nodes.byPrefix, lines);
  lines.push('');

  // Edges section
  lines.push(`${chalk.bold('Edges:')} ${status.edges.total}`);
  renderCountTable(status.edges.byType, lines);

  return lines.join('\n');
}

/**
 * Render a summary diff table with before/after/delta columns.
 * @param {Record<string, {before: number, after: number}>} entries
 * @returns {string[]} Lines of formatted output
 */
function renderDiffTable(entries) {
  const lines = [];
  const sorted = Object.entries(entries).sort(([, a], [, b]) => {
    const deltaB = Math.abs(b.after - b.before);
    const deltaA = Math.abs(a.after - a.before);
    return deltaB - deltaA;
  });
  for (const [key, { before, after }] of sorted) {
    const delta = after - before;
    const sign = delta > 0 ? '+' : delta < 0 ? '' : ' ';
    const deltaStr = delta !== 0
      ? chalk.dim(` (${sign}${delta})`)
      : '';
    lines.push(`  ${chalk.yellow(key.padEnd(14))} ${String(before).padStart(3)} ${figures.arrowRight} ${String(after).padStart(3)}${deltaStr}`);
  }
  return lines;
}

/**
 * Format a diff result for terminal display.
 * @param {import('../diff.js').DiffResult} diff
 * @returns {string}
 */
export function formatDiff(diff) {
  const lines = [];

  // Header
  lines.push(chalk.bold(`Graph Diff: ${diff.from.sha}..${diff.to.sha}`));
  lines.push(chalk.dim('═'.repeat(40)));

  // Endpoints
  const fmtEndpoint = (label, ep) => {
    const shaStr = `commit ${chalk.cyan(ep.sha)}`;
    const tickStr = `tick ${chalk.yellow(String(ep.tick))}`;
    const nearestStr = ep.nearest
      ? `  ${chalk.yellow(figures.warning)} nearest from ${chalk.dim(ep.ref)}`
      : '';
    return `${label}  ${shaStr}  ${tickStr}${nearestStr}`;
  };
  lines.push(fmtEndpoint('from', diff.from));
  lines.push(fmtEndpoint('  to', diff.to));
  lines.push('');

  // Skipped shortcut: same tick, no materialization performed
  if (diff.stats?.skipped) {
    lines.push(chalk.dim('  Same Lamport tick — graph unchanged, diff skipped.'));
    return lines.join('\n');
  }

  // Nodes
  const na = diff.nodes.total.before;
  const nb = diff.nodes.total.after;
  const nAdded = diff.nodes.added.length;
  const nRemoved = diff.nodes.removed.length;
  lines.push(`${chalk.bold('Nodes:')} ${na} ${figures.arrowRight} ${nb} (+${nAdded}, -${nRemoved})`);

  for (const id of diff.nodes.added) {
    lines.push(`  ${chalk.green('+')} ${chalk.cyan(id)}`);
  }
  for (const id of diff.nodes.removed) {
    lines.push(`  ${chalk.red('-')} ${chalk.cyan(id)}`);
  }
  if (nAdded === 0 && nRemoved === 0) {
    lines.push(chalk.dim('  (no changes)'));
  }
  lines.push('');

  // Edges
  const ea = diff.edges.total.before;
  const eb = diff.edges.total.after;
  const eAdded = diff.edges.added.length;
  const eRemoved = diff.edges.removed.length;
  lines.push(`${chalk.bold('Edges:')} ${ea} ${figures.arrowRight} ${eb} (+${eAdded}, -${eRemoved})`);

  for (const e of diff.edges.added) {
    lines.push(`  ${chalk.green('+')} ${chalk.cyan(e.source)} ${chalk.dim('--[')}${chalk.yellow(e.type)}${chalk.dim(']-->')} ${chalk.cyan(e.target)}`);
  }
  for (const e of diff.edges.removed) {
    lines.push(`  ${chalk.red('-')} ${chalk.cyan(e.source)} ${chalk.dim('--[')}${chalk.yellow(e.type)}${chalk.dim(']-->')} ${chalk.cyan(e.target)}`);
  }
  if (eAdded === 0 && eRemoved === 0) {
    lines.push(chalk.dim('  (no changes)'));
  }

  // Summary tables
  if (Object.keys(diff.summary.nodesByPrefix).length > 0) {
    lines.push('');
    lines.push(chalk.bold('By Prefix'));
    lines.push(...renderDiffTable(diff.summary.nodesByPrefix));
  }

  if (Object.keys(diff.summary.edgesByType).length > 0) {
    lines.push('');
    lines.push(chalk.bold('By Type'));
    lines.push(...renderDiffTable(diff.summary.edgesByType));
  }

  // Timing stats (debug only)
  if (process.env.GITMIND_DEBUG) {
    lines.push('');
    lines.push(chalk.dim(`materialize: ${diff.stats.materializeMs.a}ms + ${diff.stats.materializeMs.b}ms  diff: ${diff.stats.diffMs}ms`));
  }

  return lines.join('\n');
}

/**
 * Format progress view meta for terminal display.
 * @param {object} meta - Progress view meta with summary + byStatus
 * @returns {string}
 */
export function formatProgressMeta(meta) {
  const { summary } = meta;
  const lines = [];

  lines.push(chalk.bold(`Progress: ${summary.pct}% (${summary.done}/${summary.total} done)`));
  lines.push('');

  const statuses = ['done', 'in-progress', 'todo', 'blocked', 'unknown'];
  for (const s of statuses) {
    const count = summary[s] ?? 0;
    const label = s.padEnd(14);
    lines.push(`  ${chalk.yellow(label)} ${String(count).padStart(3)}`);
  }

  return lines.join('\n');
}

/**
 * Format an import result for terminal display.
 * @param {import('../import.js').ImportResult} result
 * @returns {string}
 */
export function formatImportResult(result) {
  const lines = [];

  if (result.dryRun) {
    lines.push(chalk.bold('Import dry run'));
  } else {
    lines.push(chalk.bold('Import'));
  }

  if (!result.valid) {
    lines.push(`${chalk.red(figures.cross)} Validation failed`);
    for (const err of result.errors) {
      lines.push(`  ${chalk.red(figures.cross)} ${err}`);
    }
  } else {
    if (result.dryRun) {
      lines.push(`${chalk.green(figures.tick)} Validation passed`);
      lines.push(`  Would import: ${result.stats.nodes} node(s), ${result.stats.edges} edge(s)`);
    } else {
      lines.push(`${chalk.green(figures.tick)} Imported ${result.stats.nodes} node(s), ${result.stats.edges} edge(s)`);
    }
  }

  for (const w of result.warnings) {
    lines.push(`  ${chalk.yellow(figures.warning)} ${w}`);
  }

  return lines.join('\n');
}
