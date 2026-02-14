import { describe, it, expect } from 'vitest';
import { formatSuggestionsAsMarkdown, parseReviewCommand } from '../src/format-pr.js';

describe('format-pr', () => {
  // ── formatSuggestionsAsMarkdown ─────────────────────────────

  describe('formatSuggestionsAsMarkdown', () => {
    it('returns empty message for no suggestions', () => {
      const result = formatSuggestionsAsMarkdown([]);
      expect(result).toContain('No new edge suggestions');
    });

    it('returns empty message for null suggestions', () => {
      const result = formatSuggestionsAsMarkdown(null);
      expect(result).toContain('No new edge suggestions');
    });

    it('formats a single suggestion', () => {
      const suggestions = [{
        source: 'file:auth.js',
        target: 'spec:auth',
        type: 'implements',
        confidence: 0.8,
        rationale: 'Auth module',
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      expect(result).toContain('| 1 |');
      expect(result).toContain('`file:auth.js`');
      expect(result).toContain('`spec:auth`');
      expect(result).toContain('implements');
      expect(result).toContain('80%');
      expect(result).toContain('Auth module');
    });

    it('formats multiple suggestions with commands', () => {
      const suggestions = [
        { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.9 },
        { source: 'task:c', target: 'spec:d', type: 'relates-to', confidence: 0.7 },
      ];

      const result = formatSuggestionsAsMarkdown(suggestions);
      expect(result).toContain('| 1 |');
      expect(result).toContain('| 2 |');
      expect(result).toContain('/gitmind accept 1');
      expect(result).toContain('/gitmind reject 2');
      expect(result).toContain('/gitmind accept-all');
    });

    it('escapes pipe characters in rationale', () => {
      const suggestions = [{
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
        confidence: 0.8,
        rationale: 'reason A | reason B',
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      // Pipe should be escaped so it doesn't break the table
      expect(result).not.toMatch(/\| reason A \| reason B \|/);
      expect(result).toContain('reason A');
      expect(result).toContain('reason B');
    });

    it('escapes backslashes in rationale', () => {
      const suggestions = [{
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
        confidence: 0.8,
        rationale: 'path\\to\\file',
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      // Backslashes should be escaped so markdown doesn't treat them as escape chars
      expect(result).toContain('path\\\\to\\\\file');
    });

    it('escapes backslash-pipe sequence in rationale', () => {
      const suggestions = [{
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
        confidence: 0.8,
        rationale: 'test\\|end', // literal backslash followed by pipe
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      const dataRow = result.split('\n').find(l => l.startsWith('| 1'));
      // A 7-column table row should have exactly 8 pipe delimiters.
      // Without backslash escaping, \| becomes \\| which is a raw pipe = 9 delimiters.
      expect((dataRow.match(/\|/g) || []).length).toBe(8);
    });

    it('handles missing rationale', () => {
      const suggestions = [{
        source: 'task:a',
        target: 'spec:b',
        type: 'implements',
        confidence: 1.0,
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      expect(result).toContain('100%');
      // Should not contain "undefined"
      expect(result).not.toContain('undefined');
    });

    it('handles suggestion with missing type gracefully', () => {
      const suggestions = [{
        source: 'task:a',
        target: 'spec:b',
        confidence: 0.8,
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      expect(result).not.toContain('undefined');
      expect(result).toContain('| 1 |');
    });
  });

  // ── parseReviewCommand ──────────────────────────────────────

  describe('parseReviewCommand', () => {
    it('parses accept command', () => {
      expect(parseReviewCommand('/gitmind accept 1')).toEqual({ command: 'accept', index: 1 });
    });

    it('parses reject command', () => {
      expect(parseReviewCommand('/gitmind reject 3')).toEqual({ command: 'reject', index: 3 });
    });

    it('parses accept-all command', () => {
      expect(parseReviewCommand('/gitmind accept-all')).toEqual({ command: 'accept-all' });
    });

    it('returns null for non-matching text', () => {
      expect(parseReviewCommand('just a normal comment')).toBeNull();
    });

    it('returns null for accept without index', () => {
      expect(parseReviewCommand('/gitmind accept')).toBeNull();
    });

    it('returns null for non-string input', () => {
      expect(parseReviewCommand(null)).toBeNull();
    });

    it('parses command embedded in larger comment', () => {
      const body = 'LGTM!\n\n/gitmind accept 2\n\nGreat work!';
      expect(parseReviewCommand(body)).toEqual({ command: 'accept', index: 2 });
    });

    it('returns null for accept with index 0 (1-indexed)', () => {
      expect(parseReviewCommand('/gitmind accept 0')).toBeNull();
    });

    it('returns null for reject with index 0 (1-indexed)', () => {
      expect(parseReviewCommand('/gitmind reject 0')).toBeNull();
    });
  });

  // ── backtick escaping ──────────────────────────────────────

  describe('backtick escaping in suggestions', () => {
    it('strips backtick characters from source and target', () => {
      const suggestions = [{
        source: 'file:`auth`.js',
        target: 'spec:`auth`',
        type: 'implements',
        confidence: 0.8,
      }];

      const result = formatSuggestionsAsMarkdown(suggestions);
      // Backticks should be stripped to prevent breaking code spans
      expect(result).toContain('`file:auth.js`');
      expect(result).toContain('`spec:auth`');
      // Verify table structure is intact (6 columns = 7 pipe delimiters)
      const dataRow = result.split('\n').find(l => l.startsWith('| 1'));
      expect((dataRow.match(/\|/g) || []).length).toBe(7);
    });
  });
});
