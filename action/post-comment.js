#!/usr/bin/env node

/**
 * Post git-mind suggestions as a GitHub PR comment.
 * Called by the composite action after `git mind suggest --json`.
 *
 * Usage: node post-comment.js <owner/repo> <pr-number>
 * Reads SUGGEST_RESULT from environment variable.
 */

import { execSync } from 'node:child_process';
import { formatSuggestionsAsMarkdown } from '../src/format-pr.js';

const [repo, prNumber] = process.argv.slice(2);

if (!repo || !prNumber) {
  console.error('Usage: post-comment.js <owner/repo> <pr-number>');
  process.exit(1);
}

const raw = process.env.SUGGEST_RESULT ?? '';

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
