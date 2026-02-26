/**
 * API Surface Stability Test (#206)
 *
 * This test IS the public API snapshot. If an export is removed or its type
 * changes, this test fails — forcing an intentional review before any
 * breaking change ships.
 *
 * Rules:
 *   - Removing an export → test fails → requires semver-major bump
 *   - Changing an export's type → test fails → requires semver-major bump
 *   - Adding a new export → test fails → update snapshot, semver-minor bump
 */

import { describe, it, expect } from 'vitest';
import * as api from '../src/index.js';

/**
 * Canonical snapshot of the public API surface.
 * Sorted alphabetically. Each entry: [name, expectedType].
 *
 * Last updated: v5.1.0
 */
const API_SNAPSHOT = [
  ['ALL_PREFIXES', 'object'],
  ['CANONICAL_PREFIXES', 'object'],
  ['CROSS_REPO_ID_REGEX', 'object'],
  ['DEFAULT_CONTEXT', 'object'],
  ['EDGE_TYPES', 'object'],
  ['ERROR_CATALOG', 'object'],
  ['ExitCode', 'object'],
  ['LOW_CONFIDENCE_THRESHOLD', 'number'],
  ['NODE_ID_MAX_LENGTH', 'number'],
  ['NODE_ID_REGEX', 'object'],
  ['SYSTEM_PREFIXES', 'object'],
  ['acceptSuggestion', 'function'],
  ['adjustSuggestion', 'function'],
  ['batchDecision', 'function'],
  ['buildCrossRepoId', 'function'],
  ['buildPrompt', 'function'],
  ['callAgent', 'function'],
  ['classifyPrefix', 'function'],
  ['classifyStatus', 'function'],
  ['composeLenses', 'function'],
  ['computeDiff', 'function'],
  ['computeStatus', 'function'],
  ['createContext', 'function'],
  ['createEdge', 'function'],
  ['declareView', 'function'],
  ['defineView', 'function'],
  ['defineLens', 'function'],
  ['deleteContent', 'function'],
  ['detectDanglingEdges', 'function'],
  ['detectLowConfidenceEdges', 'function'],
  ['detectOrphanMilestones', 'function'],
  ['detectOrphanNodes', 'function'],
  ['detectRepoIdentifier', 'function'],
  ['diffSnapshots', 'function'],
  ['exportGraph', 'function'],
  ['exportToFile', 'function'],
  ['extractCommitContext', 'function'],
  ['extractContext', 'function'],
  ['extractFileContext', 'function'],
  ['extractGraphContext', 'function'],
  ['extractPrefix', 'function'],
  ['extractRepo', 'function'],
  ['filterRejected', 'function'],
  ['fixIssues', 'function'],
  ['formatSuggestionsAsMarkdown', 'function'],
  ['GmindError', 'function'],
  ['generateSuggestions', 'function'],
  ['getContentMeta', 'function'],
  ['getCurrentTick', 'function'],
  ['getEpochForRef', 'function'],
  ['getExtension', 'function'],
  ['getNode', 'function'],
  ['getPendingSuggestions', 'function'],
  ['getReviewHistory', 'function'],
  ['hasContent', 'function'],
  ['importData', 'function'],
  ['importFile', 'function'],
  ['importFromMarkdown', 'function'],
  ['initGraph', 'function'],
  ['isCrossRepoId', 'function'],
  ['isLowConfidence', 'function'],
  ['listExtensions', 'function'],
  ['listLenses', 'function'],
  ['listViews', 'function'],
  ['loadExtension', 'function'],
  ['loadGraph', 'function'],
  ['lookupEpoch', 'function'],
  ['lookupNearestEpoch', 'function'],
  ['mergeFromRepo', 'function'],
  ['parseCrossRepoId', 'function'],
  ['parseDirectives', 'function'],
  ['parseFrontmatter', 'function'],
  ['parseImportFile', 'function'],
  ['parseReviewCommand', 'function'],
  ['parseSuggestions', 'function'],
  ['processCommit', 'function'],
  ['qualifyNodeId', 'function'],
  ['readContent', 'function'],
  ['recordEpoch', 'function'],
  ['registerBuiltinExtensions', 'function'],
  ['registerExtension', 'function'],
  ['rejectSuggestion', 'function'],
  ['removeEdge', 'function'],
  ['removeExtension', 'function'],
  ['renderView', 'function'],
  ['resetExtensions', 'function'],
  ['resetLenses', 'function'],
  ['resetViews', 'function'],
  ['runDoctor', 'function'],
  ['serializeExport', 'function'],
  ['setNodeProperty', 'function'],
  ['skipSuggestion', 'function'],
  ['unsetNodeProperty', 'function'],
  ['validateConfidence', 'function'],
  ['validateEdge', 'function'],
  ['validateEdgeType', 'function'],
  ['validateExtension', 'function'],
  ['validateImportData', 'function'],
  ['validateNodeId', 'function'],
  ['writeContent', 'function'],
];

describe('API Surface Stability (#206)', () => {
  const actualExports = Object.keys(api).sort();

  it('exports exactly the expected names', () => {
    const expectedNames = API_SNAPSHOT.map(([name]) => name).sort();
    const added = actualExports.filter(n => !expectedNames.includes(n));
    const removed = expectedNames.filter(n => !actualExports.includes(n));

    if (added.length > 0 || removed.length > 0) {
      const parts = [];
      if (added.length) parts.push(`Added exports: ${added.join(', ')}`);
      if (removed.length) parts.push(`Removed exports: ${removed.join(', ')}`);
      expect.fail(
        `API surface changed — update test/api-surface.test.js\n${parts.join('\n')}\n` +
        'See CONTRIBUTING.md § Deprecation Protocol for the process.'
      );
    }
  });

  it('each export has the expected type', () => {
    const mismatches = [];
    for (const [name, expectedType] of API_SNAPSHOT) {
      const actual = typeof api[name];
      if (actual !== expectedType) {
        mismatches.push(`${name}: expected ${expectedType}, got ${actual}`);
      }
    }
    if (mismatches.length > 0) {
      expect.fail(
        `API type changes detected — this is a breaking change:\n${mismatches.join('\n')}`
      );
    }
  });

  it('snapshot count matches actual export count', () => {
    expect(actualExports.length).toBe(API_SNAPSHOT.length);
  });
});
