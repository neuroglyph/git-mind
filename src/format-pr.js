/**
 * @module format-pr
 * Markdown formatting for PR suggestion display and slash command parsing.
 */

/** Escape backslashes and pipe characters for markdown table cells. */
function escapeCell(str) {
  return str
    .replace(/\\/g, '\\\\')
    .replace(/\|/g, '\\|');
}

/**
 * Format suggestions as a markdown table for PR comments.
 *
 * @param {Array<{source: string, target: string, type: string, confidence: number, rationale?: string}>} suggestions
 * @returns {string}
 */
export function formatSuggestionsAsMarkdown(suggestions) {
  if (!suggestions || suggestions.length === 0) {
    return '> No new edge suggestions for this PR.';
  }

  const lines = [
    '| # | Source | Target | Type | Confidence | Rationale |',
    '|---|--------|--------|------|------------|-----------|',
  ];

  for (let i = 0; i < suggestions.length; i++) {
    const s = suggestions[i];
    const conf = `${(((s.confidence ?? 0)) * 100).toFixed(0)}%`;
    const rationale = escapeCell(s.rationale ?? '');
    const source = escapeCell((s.source ?? '').replace(/`/g, ''));
    const target = escapeCell((s.target ?? '').replace(/`/g, ''));
    lines.push(`| ${i + 1} | \`${source}\` | \`${target}\` | ${escapeCell(s.type)} | ${conf} | ${rationale} |`);
  }

  lines.push('');
  lines.push('<details><summary>Commands</summary>');
  lines.push('');
  for (let i = 0; i < suggestions.length; i++) {
    lines.push(`- \`/gitmind accept ${i + 1}\` — Accept suggestion ${i + 1}`);
    lines.push(`- \`/gitmind reject ${i + 1}\` — Reject suggestion ${i + 1}`);
  }
  lines.push(`- \`/gitmind accept-all\` — Accept all suggestions`);
  lines.push('');
  lines.push('</details>');
  lines.push('');
  lines.push('---');
  lines.push('*Posted by [git-mind](https://github.com/neuroglyph/git-mind)*');

  return lines.join('\n');
}

/**
 * Parse a /gitmind slash command from a comment body.
 *
 * @param {string} body - Comment body text
 * @returns {{ command: string, index?: number } | null}
 */
export function parseReviewCommand(body) {
  if (typeof body !== 'string') return null;

  const match = body.match(/\/gitmind\s+(accept-all|accept|reject)(?:\s+(\d+))?/);
  if (!match) return null;

  const command = match[1];

  if (command === 'accept-all') {
    return { command: 'accept-all' };
  }

  const index = match[2] ? parseInt(match[2], 10) : undefined;
  if (index === undefined || index < 1) return null; // accept/reject require a positive 1-indexed value

  return { command, index };
}
