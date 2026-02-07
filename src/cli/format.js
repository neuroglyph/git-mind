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
