#!/usr/bin/env node

/**
 * Post git-mind suggestions as a GitHub PR comment.
 * Called by the composite action after `git mind suggest --json`.
 *
 * Usage: node post-comment.js <owner/repo> <pr-number>
 * Reads SUGGEST_RESULT from environment variable.
 */

import { execSync } from 'node:child_process';

const [repo, prNumber] = process.argv.slice(2);

if (!repo || !prNumber) {
  console.error('Usage: post-comment.js <owner/repo> <pr-number>');
  process.exit(1);
}

const raw = process.env.SUGGEST_RESULT ?? '';

/**
 * Format suggestions as a markdown table.
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
    const conf = `${(s.confidence * 100).toFixed(0)}%`;
    const rationale = s.rationale ?? '';
    lines.push(`| ${i + 1} | \`${s.source}\` | \`${s.target}\` | ${s.type} | ${conf} | ${rationale} |`);
  }

  lines.push('');
  lines.push('---');
  lines.push('*Posted by [git-mind](https://github.com/neuroglyph/git-mind)*');

  return lines.join('\n');
}

// Parse the suggest result
let result;
try {
  result = JSON.parse(raw);
} catch {
  console.log('No valid JSON in suggest result — skipping comment.');
  process.exit(0);
}

const body = `## git-mind Suggestions\n\n${formatSuggestionsAsMarkdown(result.suggestions)}`;

// Post comment via gh CLI
try {
  execSync(
    `gh api repos/${repo}/issues/${prNumber}/comments -f body=${JSON.stringify(body)}`,
    { stdio: 'inherit' },
  );
  console.log('Comment posted successfully.');
} catch (err) {
  console.error(`Failed to post comment: ${err.message}`);
  process.exit(1);
}
