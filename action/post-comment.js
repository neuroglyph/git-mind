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

// Validate inputs before shell interpolation
if (!/^[A-Za-z0-9._-]+\/[A-Za-z0-9._-]+$/.test(repo)) {
  console.error('Invalid repo format. Expected owner/name.');
  process.exit(1);
}
if (!/^\d+$/.test(prNumber)) {
  console.error('Invalid PR number. Expected an integer.');
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

// Post comment via gh CLI — pass body as JSON via stdin to avoid shell injection
try {
  const payload = JSON.stringify({ body });
  execSync(
    `gh api repos/${repo}/issues/${prNumber}/comments --input -`,
    { input: payload, stdio: ['pipe', 'inherit', 'inherit'] },
  );
  console.log('Comment posted successfully.');
} catch (err) {
  console.error(`Failed to post comment: ${err.message}`);
  process.exit(1);
}
