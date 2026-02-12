/**
 * @module import
 * YAML import pipeline for git-mind.
 * Schema-validated, idempotent, atomic graph ingestion.
 */

import { readFile } from 'node:fs/promises';
import yaml from 'js-yaml';
import { validateNodeId, validateEdgeType, validateConfidence, extractPrefix, classifyPrefix } from './validators.js';

const SUPPORTED_VERSIONS = [1];

/**
 * @typedef {object} ImportResult
 * @property {boolean} valid - Whether the import data passed all validation
 * @property {string[]} errors - Validation errors (import aborted if non-empty)
 * @property {string[]} warnings - Non-fatal warnings
 * @property {{ nodes: number, edges: number }} stats - Count of items that were (or would be) written
 * @property {boolean} dryRun - Whether this was a dry-run (no writes)
 */

/**
 * Parse and validate a YAML import file.
 *
 * @param {string} filePath - Path to the YAML file
 * @returns {Promise<{ data: object|null, parseError: string|null }>}
 */
export async function parseImportFile(filePath) {
  try {
    const raw = await readFile(filePath, 'utf-8');
    const data = yaml.load(raw);
    if (data === null || data === undefined || typeof data !== 'object' || Array.isArray(data)) {
      return { data: null, parseError: 'YAML file is empty or not an object' };
    }
    return { data, parseError: null };
  } catch (err) {
    if (err.code === 'ENOENT') {
      return { data: null, parseError: `File not found: ${filePath}` };
    }
    return { data: null, parseError: `YAML parse error: ${err.message}` };
  }
}

/**
 * Validate import data against the schema.
 *
 * @param {object} data - Parsed YAML data
 * @param {import('@git-stunts/git-warp').default} graph - Graph for reference checks
 * @returns {Promise<{ valid: boolean, errors: string[], warnings: string[], nodeIds: Set<string> }>}
 */
export async function validateImportData(data, graph) {
  const errors = [];
  const warnings = [];

  // Version check
  if (data.version === undefined || data.version === null) {
    errors.push('Missing required field: "version"');
    return { valid: false, errors, warnings, nodeIds: new Set() };
  }
  if (!SUPPORTED_VERSIONS.includes(data.version)) {
    errors.push(`Unsupported version: ${data.version}. Supported: ${SUPPORTED_VERSIONS.join(', ')}`);
    return { valid: false, errors, warnings, nodeIds: new Set() };
  }

  // Collect all declared node IDs (from nodes array + edge endpoints)
  const declaredNodeIds = new Set();

  // Validate nodes
  const nodeEntries = data.nodes ?? [];
  if (!Array.isArray(nodeEntries)) {
    errors.push('"nodes" must be an array');
  } else {
    for (let i = 0; i < nodeEntries.length; i++) {
      const node = nodeEntries[i];
      if (!node || typeof node !== 'object') {
        errors.push(`nodes[${i}]: must be an object`);
        continue;
      }
      if (!node.id) {
        errors.push(`nodes[${i}]: missing required field "id"`);
        continue;
      }
      const v = validateNodeId(node.id);
      if (!v.valid) {
        errors.push(`nodes[${i}]: ${v.error}`);
        continue;
      }
      // Validate properties if provided
      if (node.properties !== undefined && node.properties !== null) {
        if (typeof node.properties !== 'object' || Array.isArray(node.properties)) {
          errors.push(`nodes[${i}].properties: must be a plain object, not ${Array.isArray(node.properties) ? 'an array' : typeof node.properties}`);
        }
      }

      // Prefix warning
      const prefix = extractPrefix(node.id);
      if (prefix && classifyPrefix(prefix) === 'unknown') {
        warnings.push(`nodes[${i}]: prefix "${prefix}" is not a canonical prefix`);
      }
      declaredNodeIds.add(node.id);
    }
  }

  // Validate edges
  const edgeEntries = data.edges ?? [];
  if (!Array.isArray(edgeEntries)) {
    errors.push('"edges" must be an array');
  } else {
    // Get existing nodes for reference validation
    const existingNodes = new Set(await graph.getNodes());

    for (let i = 0; i < edgeEntries.length; i++) {
      const edge = edgeEntries[i];
      if (!edge || typeof edge !== 'object') {
        errors.push(`edges[${i}]: must be an object`);
        continue;
      }

      // Required fields — collect missing, skip further checks if any absent
      const missing = [];
      if (!edge.source) missing.push('source');
      if (!edge.target) missing.push('target');
      if (!edge.type) missing.push('type');
      if (missing.length > 0) {
        for (const field of missing) {
          errors.push(`edges[${i}]: missing required field "${field}"`);
        }
        continue;
      }

      // Validate source/target node IDs
      const sv = validateNodeId(edge.source);
      if (!sv.valid) errors.push(`edges[${i}].source: ${sv.error}`);

      const tv = validateNodeId(edge.target);
      if (!tv.valid) errors.push(`edges[${i}].target: ${tv.error}`);

      // Validate edge type
      const et = validateEdgeType(edge.type);
      if (!et.valid) errors.push(`edges[${i}].type: ${et.error}`);

      // Validate confidence if provided
      if (edge.confidence !== undefined) {
        const cv = validateConfidence(edge.confidence);
        if (!cv.valid) errors.push(`edges[${i}].confidence: ${cv.error}`);
      }

      // Self-edge check for blocks/depends-on
      if (edge.source === edge.target && ['blocks', 'depends-on'].includes(edge.type)) {
        errors.push(`edges[${i}]: self-edge forbidden for "${edge.type}"`);
      }

      // Reference validation: source and target must be declared or pre-existing
      if (sv.valid && !declaredNodeIds.has(edge.source) && !existingNodes.has(edge.source)) {
        errors.push(`edges[${i}].source: "${edge.source}" is not declared in nodes[] and does not exist in the graph`);
      }
      if (tv.valid && !declaredNodeIds.has(edge.target) && !existingNodes.has(edge.target)) {
        errors.push(`edges[${i}].target: "${edge.target}" is not declared in nodes[] and does not exist in the graph`);
      }
    }
  }

  return { valid: errors.length === 0, errors, warnings, nodeIds: declaredNodeIds };
}

/**
 * Import a validated data object into the graph atomically.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {object} data - Validated import data
 * @returns {Promise<{ nodes: number, edges: number }>}
 */
async function writeImport(graph, data) {
  const patch = await graph.createPatch();
  let nodeCount = 0;
  let edgeCount = 0;

  // Add nodes
  for (const node of (data.nodes ?? [])) {
    patch.addNode(node.id);
    nodeCount++;

    // Set node properties if provided
    if (node.properties && typeof node.properties === 'object') {
      for (const [key, value] of Object.entries(node.properties)) {
        patch.setProperty(node.id, key, value);
      }
    }
  }

  // Add edges
  for (const edge of (data.edges ?? [])) {
    // Ensure edge endpoint nodes exist (idempotent)
    patch.addNode(edge.source);
    patch.addNode(edge.target);

    patch.addEdge(edge.source, edge.target, edge.type);

    const confidence = edge.confidence ?? 1.0;
    patch.setEdgeProperty(edge.source, edge.target, edge.type, 'confidence', confidence);
    patch.setEdgeProperty(edge.source, edge.target, edge.type, 'importedAt', new Date().toISOString());

    if (edge.rationale) {
      patch.setEdgeProperty(edge.source, edge.target, edge.type, 'rationale', edge.rationale);
    }

    edgeCount++;
  }

  // Only commit if there are operations to write
  if (nodeCount > 0 || edgeCount > 0) {
    await patch.commit();
  }
  return { nodes: nodeCount, edges: edgeCount };
}

/**
 * Import a validated v1 data object into the graph.
 * This is the shared pipeline used by both YAML import and frontmatter import.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {object} data - v1 import data ({ version, nodes, edges })
 * @param {{ dryRun?: boolean }} [opts]
 * @returns {Promise<ImportResult>}
 */
export async function importData(graph, data, opts = {}) {
  const dryRun = opts.dryRun ?? false;

  // Validate
  const { valid, errors, warnings } = await validateImportData(data, graph);
  if (!valid) {
    return { valid: false, errors, warnings, stats: { nodes: 0, edges: 0 }, dryRun };
  }

  // Dry-run: report stats without writing
  if (dryRun) {
    const stats = {
      nodes: (data.nodes ?? []).length,
      edges: (data.edges ?? []).length,
    };
    return { valid: true, errors: [], warnings, stats, dryRun: true };
  }

  // Write atomically
  const stats = await writeImport(graph, data);
  return { valid: true, errors: [], warnings, stats, dryRun: false };
}

/**
 * Import a YAML file into the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {string} filePath - Path to the YAML file
 * @param {{ dryRun?: boolean }} [opts]
 * @returns {Promise<ImportResult>}
 */
export async function importFile(graph, filePath, opts = {}) {
  const dryRun = opts.dryRun ?? false;

  // Parse
  const { data, parseError } = await parseImportFile(filePath);
  if (parseError) {
    return { valid: false, errors: [parseError], warnings: [], stats: { nodes: 0, edges: 0 }, dryRun };
  }

  return importData(graph, data, opts);
}
