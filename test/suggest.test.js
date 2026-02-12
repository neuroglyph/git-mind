import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { mkdtemp, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { execSync } from 'node:child_process';
import { initGraph } from '../src/graph.js';
import { createEdge } from '../src/edges.js';
import { callAgent, parseSuggestions, filterRejected } from '../src/suggest.js';

describe('suggest', () => {
  let tempDir;
  let graph;

  beforeEach(async () => {
    tempDir = await mkdtemp(join(tmpdir(), 'gitmind-test-'));
    execSync('git init', { cwd: tempDir, stdio: 'ignore' });
    graph = await initGraph(tempDir);
  });

  afterEach(async () => {
    await rm(tempDir, { recursive: true, force: true });
  });

  // ── callAgent ───────────────────────────────────────────────

  it('throws when GITMIND_AGENT is not set', () => {
    const original = process.env.GITMIND_AGENT;
    delete process.env.GITMIND_AGENT;
    try {
      expect(() => callAgent('test prompt')).toThrow(/GITMIND_AGENT not set/);
    } finally {
      if (original) process.env.GITMIND_AGENT = original;
    }
  });

  // ── parseSuggestions ────────────────────────────────────────

  it('parses valid JSON array', () => {
    const input = JSON.stringify([
      { source: 'file:app.js', target: 'spec:auth', type: 'implements', confidence: 0.8, rationale: 'direct impl' },
    ]);
    const { suggestions, errors } = parseSuggestions(input);

    expect(errors).toHaveLength(0);
    expect(suggestions).toHaveLength(1);
    expect(suggestions[0].source).toBe('file:app.js');
    expect(suggestions[0].confidence).toBe(0.8);
  });

  it('extracts JSON from markdown code fences', () => {
    const input = `Here are my suggestions:

\`\`\`json
[
  { "source": "task:a", "target": "spec:b", "type": "implements", "confidence": 0.7 }
]
\`\`\`

That's all!`;

    const { suggestions, errors } = parseSuggestions(input);

    expect(errors).toHaveLength(0);
    expect(suggestions).toHaveLength(1);
    expect(suggestions[0].type).toBe('implements');
  });

  it('rejects invalid edge types', () => {
    const input = JSON.stringify([
      { source: 'task:a', target: 'spec:b', type: 'foobar', confidence: 0.5 },
    ]);
    const { suggestions, errors } = parseSuggestions(input);

    expect(suggestions).toHaveLength(0);
    expect(errors).toHaveLength(1);
    expect(errors[0]).toMatch(/foobar/);
  });

  it('rejects invalid node IDs', () => {
    const input = JSON.stringify([
      { source: 'bad id!', target: 'spec:b', type: 'implements', confidence: 0.5 },
    ]);
    const { suggestions, errors } = parseSuggestions(input);

    expect(suggestions).toHaveLength(0);
    expect(errors).toHaveLength(1);
    expect(errors[0]).toMatch(/source/);
  });

  it('handles empty response', () => {
    const { suggestions, errors } = parseSuggestions('');
    expect(suggestions).toHaveLength(0);
    expect(errors).toHaveLength(1);
    expect(errors[0]).toMatch(/Empty response/);
  });

  it('extracts JSON array using indexOf fallback', () => {
    const input = 'Some preamble text [ { "source": "task:a", "target": "spec:b", "type": "implements", "confidence": 0.6 } ] trailing';
    const { suggestions, errors } = parseSuggestions(input);
    expect(errors).toHaveLength(0);
    expect(suggestions).toHaveLength(1);
    expect(suggestions[0].source).toBe('task:a');
  });

  it('handles malformed JSON', () => {
    const { suggestions, errors } = parseSuggestions('not json at all');
    expect(suggestions).toHaveLength(0);
    expect(errors).toHaveLength(1);
    expect(errors[0]).toMatch(/Failed to parse/);
  });

  it('defaults confidence to 0.5 when missing', () => {
    const input = JSON.stringify([
      { source: 'task:a', target: 'spec:b', type: 'implements' },
    ]);
    const { suggestions } = parseSuggestions(input);
    expect(suggestions).toHaveLength(1);
    expect(suggestions[0].confidence).toBe(0.5);
  });

  // ── filterRejected ─────────────────────────────────────────

  it('filters out previously rejected suggestions', async () => {
    // Record a rejection decision
    const patch = await graph.createPatch();
    patch.addNode('decision:123-abc');
    patch.setProperty('decision:123-abc', 'action', 'reject');
    patch.setProperty('decision:123-abc', 'source', 'task:a');
    patch.setProperty('decision:123-abc', 'target', 'spec:b');
    patch.setProperty('decision:123-abc', 'edgeType', 'implements');
    await patch.commit();

    const suggestions = [
      { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.5 },
      { source: 'task:c', target: 'spec:d', type: 'relates-to', confidence: 0.6 },
    ];

    const filtered = await filterRejected(suggestions, graph);

    expect(filtered).toHaveLength(1);
    expect(filtered[0].source).toBe('task:c');
  });

  it('returns all suggestions when no decision nodes exist', async () => {
    const suggestions = [
      { source: 'task:a', target: 'spec:b', type: 'implements', confidence: 0.7 },
    ];
    const filtered = await filterRejected(suggestions, graph);
    expect(filtered).toHaveLength(1);
  });
});
