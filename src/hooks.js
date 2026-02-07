/**
 * @module hooks
 * Post-commit hook: parse directives from commit messages and auto-create edges.
 */

import { createEdge } from './edges.js';

/**
 * @typedef {object} Directive
 * @property {string} type - Edge type (lowercased from directive keyword)
 * @property {string} target - Target node referenced by the directive
 */

/**
 * Supported directive patterns in commit messages.
 * Format: `DIRECTIVE: <target>`
 */
const DIRECTIVE_PATTERN = /^(IMPLEMENTS|AUGMENTS|RELATES-TO|BLOCKS|DEPENDS-ON|DOCUMENTS):\s*(.+)$/gmi;

/**
 * Parse directives from a commit message.
 *
 * @param {string} message - The commit message to parse
 * @returns {Directive[]}
 */
export function parseDirectives(message) {
  const directives = [];
  let match;

  // Reset lastIndex for global regex
  DIRECTIVE_PATTERN.lastIndex = 0;

  while ((match = DIRECTIVE_PATTERN.exec(message)) !== null) {
    directives.push({
      type: match[1].toLowerCase(),
      target: match[2].trim(),
    });
  }

  return directives;
}

/**
 * Process a commit: parse directives and create edges in the graph.
 *
 * @param {import('@git-stunts/git-warp').default} graph
 * @param {{ sha: string, message: string, files?: string[] }} commit
 * @returns {Promise<Directive[]>} The directives that were processed
 */
export async function processCommit(graph, commit) {
  const directives = parseDirectives(commit.message);

  for (const directive of directives) {
    // Source is the commit SHA — represents the code change
    const source = `commit:${commit.sha}`;

    await createEdge(graph, {
      source,
      target: directive.target,
      type: directive.type,
      confidence: 0.8, // High but not human-reviewed
      rationale: `Auto-created from commit ${commit.sha.slice(0, 8)}`,
    });
  }

  return directives;
}
