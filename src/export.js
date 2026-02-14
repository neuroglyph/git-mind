/**
 * @module export
 * Graph export to v1 import-compatible format (YAML/JSON).
 * Enables round-trip: export → edit → import.
 */

import { writeFile } from 'node:fs/promises';
import yaml from 'js-yaml';
import { extractPrefix } from './validators.js';

/** Edge property keys excluded from export (timestamps are system-managed). */
const EXCLUDED_EDGE_PROPS = new Set(['createdAt', 'importedAt', 'reviewedAt']);

/** Node prefixes excluded from export by default (system-generated). */
const EXCLUDED_PREFIXES = new Set(['decision', 'commit', 'epoch']);

/**
 * @typedef {object} ExportOptions
 * @property {string} [prefix] - Only include nodes matching this prefix
 * @property {string} [format='yaml'] - Output format: 'yaml' or 'json'
 */

/**
 * @typedef {object} ExportData
 * @property {number} version - Schema version (always 1)
 * @property {Array<{id: string, properties?: Record<string, unknown>}>} nodes
 * @property {Array<{source: string, target: string, type: string, confidence?: number, rationale?: string}>} edges
 */

/**
 * Export the graph as a v1 import-compatible data structure.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {ExportOptions} [opts]
 * @returns {Promise<ExportData>}
 */
export async function exportGraph(graph, opts = {}) {
  const prefixFilter = opts.prefix ?? null;

  // Collect all nodes
  const allNodeIds = await graph.getNodes();

  // Filter nodes
  const filteredNodeIds = allNodeIds.filter(id => {
    const prefix = extractPrefix(id);
    // Exclude system prefixes
    if (prefix && EXCLUDED_PREFIXES.has(prefix)) return false;
    // Apply prefix filter if specified
    if (prefixFilter && prefix !== prefixFilter) return false;
    return true;
  });

  const nodeIdSet = new Set(filteredNodeIds);

  // Build node entries with properties
  const nodes = [];
  for (const id of filteredNodeIds) {
    const propsMap = await graph.getNodeProps(id);
    const entry = { id };

    if (propsMap && propsMap.size > 0) {
      const properties = {};
      for (const [key, value] of propsMap) {
        properties[key] = value;
      }
      entry.properties = properties;
    }

    nodes.push(entry);
  }

  // Collect and filter edges
  const allEdges = await graph.getEdges();
  const edges = [];

  for (const edge of allEdges) {
    // Only include edges where both endpoints are in the filtered set
    if (!nodeIdSet.has(edge.from) || !nodeIdSet.has(edge.to)) continue;

    const entry = {
      source: edge.from,
      target: edge.to,
      type: edge.label,
    };

    // Include non-excluded edge properties
    if (edge.props) {
      for (const [key, value] of Object.entries(edge.props)) {
        if (EXCLUDED_EDGE_PROPS.has(key)) continue;
        if (value !== undefined && value !== null) {
          entry[key] = value;
        }
      }
    }

    edges.push(entry);
  }

  return { version: 1, nodes, edges };
}

/**
 * Serialize export data to a string.
 *
 * @param {ExportData} data
 * @param {'yaml'|'json'} [format='yaml']
 * @returns {string}
 */
export function serializeExport(data, format = 'yaml') {
  if (format === 'json') {
    return JSON.stringify(data, null, 2);
  }
  if (format !== 'yaml') {
    throw new Error(`Unsupported export format: "${format}". Expected "yaml" or "json".`);
  }
  return yaml.dump(data, { lineWidth: -1, noRefs: true, sortKeys: false });
}

/**
 * Export the graph to a file.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} filePath - Output file path
 * @param {ExportOptions} [opts]
 * @returns {Promise<{stats: {nodes: number, edges: number}, path: string}>}
 */
export async function exportToFile(graph, filePath, opts = {}) {
  const data = await exportGraph(graph, opts);
  const format = opts.format ?? (filePath.endsWith('.json') ? 'json' : 'yaml');
  const content = serializeExport(data, format);
  await writeFile(filePath, content, 'utf-8');
  return {
    stats: { nodes: data.nodes.length, edges: data.edges.length },
    path: filePath,
  };
}
