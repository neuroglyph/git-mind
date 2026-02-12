/**
 * @module suggest
 * Agent-powered edge suggestions for git-mind.
 * Shells out to a user-configured command via GITMIND_AGENT env var.
 */

import { spawn } from 'node:child_process';
import { validateNodeId, validateEdgeType, validateConfidence } from './validators.js';
import { queryEdges } from './edges.js';
import { extractContext } from './context.js';

/**
 * @typedef {object} Suggestion
 * @property {string} source - Source node ID
 * @property {string} target - Target node ID
 * @property {string} type - Edge type
 * @property {number} confidence - 0.0–1.0
 * @property {string} [rationale] - Why this edge is suggested
 */

/**
 * @typedef {object} SuggestResult
 * @property {Suggestion[]} suggestions
 * @property {string[]} errors
 * @property {string} prompt - The prompt sent to the agent
 */

/**
 * Call an external agent command with a prompt via stdin.
 * Returns the raw stdout text.
 *
 * @param {string} prompt - Prompt to send via stdin
 * @param {{ agent?: string }} [opts={}]
 * @returns {Promise<string>}
 */
export function callAgent(prompt, opts = {}) {
  const cmd = opts.agent ?? process.env.GITMIND_AGENT;
  if (!cmd) {
    throw new Error(
      'GITMIND_AGENT not set. Configure an AI agent command.\n' +
      'Example: export GITMIND_AGENT="claude -p --output-format json"'
    );
  }

  const timeout = opts.timeout ?? 120_000;

  return new Promise((resolve, reject) => {
    const child = spawn(cmd, { shell: true, stdio: ['pipe', 'pipe', 'pipe'] });
    const chunks = [];
    const errChunks = [];
    let settled = false;

    const timer = setTimeout(() => {
      if (!settled) {
        settled = true;
        child.kill('SIGTERM');
        reject(new Error(`Agent command timed out after ${timeout}ms`));
      }
    }, timeout);

    child.stdout.on('data', (data) => chunks.push(data));
    child.stderr.on('data', (data) => errChunks.push(data));

    child.on('error', (err) => {
      if (!settled) {
        settled = true;
        clearTimeout(timer);
        reject(new Error(`Agent command failed to start: ${err.message}`));
      }
    });

    child.on('close', (code) => {
      if (!settled) {
        settled = true;
        clearTimeout(timer);
        const stdout = Buffer.concat(chunks).toString('utf-8');
        if (code !== 0) {
          const stderr = Buffer.concat(errChunks).toString('utf-8');
          reject(new Error(`Agent exited with code ${code}: ${stderr.slice(0, 500)}`));
        } else {
          resolve(stdout);
        }
      }
    });

    child.stdin.write(prompt);
    child.stdin.end();
  });
}

/**
 * Parse and validate suggestions from raw agent response text.
 * Handles raw JSON arrays and markdown code fences.
 *
 * @param {string} responseText
 * @returns {{ suggestions: Suggestion[], errors: string[] }}
 */
export function parseSuggestions(responseText) {
  const errors = [];

  if (!responseText || !responseText.trim()) {
    return { suggestions: [], errors: ['Empty response from agent'] };
  }

  let text = responseText.trim();

  // Extract JSON from markdown code fences if present (indexOf-based, no regex)
  const fenceStart = text.indexOf('```');
  if (fenceStart !== -1) {
    const contentStart = text.indexOf('\n', fenceStart);
    const fenceEnd = text.indexOf('\n```', contentStart + 1);
    if (contentStart !== -1 && fenceEnd !== -1) {
      text = text.slice(contentStart + 1, fenceEnd).trim();
    }
  }

  // Try to parse as JSON
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch (err) {
    // Try to find a JSON array in the text using indexOf (avoids ReDoS)
    const start = text.indexOf('[');
    const end = text.lastIndexOf(']');
    if (start !== -1 && end > start) {
      try {
        parsed = JSON.parse(text.slice(start, end + 1));
      } catch {
        return { suggestions: [], errors: [`Failed to parse agent response as JSON: ${err.message}`] };
      }
    } else {
      return { suggestions: [], errors: [`Failed to parse agent response as JSON: ${err.message}`] };
    }
  }

  if (!Array.isArray(parsed)) {
    return { suggestions: [], errors: ['Agent response is not a JSON array'] };
  }

  const suggestions = [];
  for (let i = 0; i < parsed.length; i++) {
    const item = parsed[i];
    const itemErrors = [];

    if (!item || typeof item !== 'object') {
      errors.push(`Item ${i}: not an object`);
      continue;
    }

    // Validate source
    const srcResult = validateNodeId(item.source);
    if (!srcResult.valid) itemErrors.push(`source: ${srcResult.error}`);

    // Validate target
    const tgtResult = validateNodeId(item.target);
    if (!tgtResult.valid) itemErrors.push(`target: ${tgtResult.error}`);

    // Validate type
    const typeResult = validateEdgeType(item.type);
    if (!typeResult.valid) itemErrors.push(typeResult.error);

    // Validate confidence
    const conf = typeof item.confidence === 'number' ? item.confidence : 0.5;
    const confResult = validateConfidence(conf);
    if (!confResult.valid) itemErrors.push(confResult.error);

    if (itemErrors.length > 0) {
      errors.push(`Item ${i}: ${itemErrors.join('; ')}`);
      continue;
    }

    suggestions.push({
      source: item.source,
      target: item.target,
      type: item.type,
      confidence: conf,
      rationale: item.rationale ?? '',
    });
  }

  return { suggestions, errors };
}

/**
 * Filter out suggestions that have been previously rejected (decision nodes exist).
 *
 * @param {Suggestion[]} suggestions
 * @param {import('@git-stunts/git-warp').default} graph
 * @returns {Promise<Suggestion[]>}
 */
export async function filterRejected(suggestions, graph) {
  const nodes = await graph.getNodes();
  const decisionNodes = nodes.filter(n => n.startsWith('decision:'));

  if (decisionNodes.length === 0) return suggestions;

  // Build a set of rejected source|target|type combinations
  const rejectedKeys = new Set();
  const propsResults = await Promise.all(decisionNodes.map(id => graph.getNodeProps(id)));
  for (const propsMap of propsResults) {
    if (!propsMap) continue;
    const action = propsMap.get('action');
    if (action !== 'reject') continue;
    const source = propsMap.get('source');
    const target = propsMap.get('target');
    const edgeType = propsMap.get('edgeType');
    if (source && target && edgeType) {
      rejectedKeys.add(`${source}|${target}|${edgeType}`);
    }
  }

  return suggestions.filter(s => !rejectedKeys.has(`${s.source}|${s.target}|${s.type}`));
}

/**
 * Generate suggestions by extracting context, calling agent, and parsing results.
 *
 * @param {string} cwd - Repository root
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {{ agent?: string, range?: string }} [opts={}]
 * @returns {Promise<SuggestResult>}
 */
export async function generateSuggestions(cwd, graph, opts = {}) {
  const context = await extractContext(cwd, graph, opts);
  const responseText = await callAgent(context.prompt, opts);
  const { suggestions, errors } = parseSuggestions(responseText);
  const filtered = await filterRejected(suggestions, graph);

  return { suggestions: filtered, errors, prompt: context.prompt };
}
