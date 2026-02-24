/**
 * @module @flyingrobots/git-mind
 * Public API for git-mind — a project knowledge graph tool built on git-warp.
 */

export { initGraph, loadGraph, saveGraph } from './graph.js';
export { createEdge, queryEdges, removeEdge, EDGE_TYPES } from './edges.js';
export { getNodes, hasNode, getNode, getNodesByPrefix, setNodeProperty, unsetNodeProperty } from './nodes.js';
export { computeStatus } from './status.js';
export { importFile, importData, parseImportFile, validateImportData } from './import.js';
export { importFromMarkdown, parseFrontmatter } from './frontmatter.js';
export { exportGraph, serializeExport, exportToFile } from './export.js';
export {
  validateNodeId, validateEdgeType, validateConfidence, validateEdge,
  extractPrefix, classifyPrefix, isLowConfidence,
  NODE_ID_REGEX, NODE_ID_MAX_LENGTH, CANONICAL_PREFIXES, SYSTEM_PREFIXES, ALL_PREFIXES,
  LOW_CONFIDENCE_THRESHOLD,
} from './validators.js';
export { defineView, declareView, renderView, listViews, resetViews, classifyStatus } from './views.js';
export { defineLens, listLenses, resetLenses, composeLenses } from './lens.js';
export { parseDirectives, processCommit } from './hooks.js';
export {
  getCurrentTick, recordEpoch, lookupEpoch,
  lookupNearestEpoch, getEpochForRef,
} from './epoch.js';
export {
  detectDanglingEdges, detectOrphanMilestones, detectOrphanNodes,
  detectLowConfidenceEdges, runDoctor, fixIssues,
} from './doctor.js';
export {
  parseCrossRepoId, buildCrossRepoId, isCrossRepoId,
  extractRepo, qualifyNodeId, CROSS_REPO_ID_REGEX,
} from './remote.js';
export { mergeFromRepo, detectRepoIdentifier } from './merge.js';
export { formatSuggestionsAsMarkdown, parseReviewCommand } from './format-pr.js';
export {
  extractFileContext, extractCommitContext, extractGraphContext,
  buildPrompt, extractContext,
} from './context.js';
export { callAgent, parseSuggestions, filterRejected, generateSuggestions } from './suggest.js';
export {
  getPendingSuggestions, acceptSuggestion, rejectSuggestion,
  adjustSuggestion, skipSuggestion, getReviewHistory, batchDecision,
} from './review.js';
export { computeDiff, diffSnapshots } from './diff.js';
export { createContext, DEFAULT_CONTEXT } from './context-envelope.js';
export {
  loadExtension, registerExtension, removeExtension, listExtensions, getExtension,
  validateExtension, resetExtensions, registerBuiltinExtensions,
} from './extension.js';
export {
  writeContent, readContent, getContentMeta, hasContent, deleteContent,
} from './content.js';
export { VERSION, NAME } from './version.js';
