/**
 * @module cli/format
 * Terminal output formatting for git-mind CLI.
 */

import chalk from 'chalk';
import figures from 'figures';

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
  const prefixes = Object.entries(status.nodes.byPrefix)
    .sort(([, a], [, b]) => b - a);
  for (const [prefix, count] of prefixes) {
    const pct = status.nodes.total > 0
      ? Math.round((count / status.nodes.total) * 100)
      : 0;
    lines.push(`  ${chalk.yellow(prefix.padEnd(14))} ${String(count).padStart(3)}  ${chalk.dim(`(${pct}%)`)}`);
  }
  lines.push('');

  // Edges section
  lines.push(`${chalk.bold('Edges:')} ${status.edges.total}`);
  const types = Object.entries(status.edges.byType)
    .sort(([, a], [, b]) => b - a);
  for (const [type, count] of types) {
    lines.push(`  ${chalk.yellow(type.padEnd(14))} ${String(count).padStart(3)}`);
  }
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
