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
  });
});
