import { describe, it, expect } from 'vitest';
import {
  extractPrefix,
  validateNodeId,
  classifyPrefix,
  validateEdgeType,
  validateConfidence,
  validateEdge,
  NODE_ID_REGEX,
  NODE_ID_MAX_LENGTH,
  CANONICAL_PREFIXES,
  SYSTEM_PREFIXES,
  EDGE_TYPES,
  ALL_PREFIXES,
} from '../src/validators.js';

describe('extractPrefix', () => {
  it('extracts prefix from a valid node ID', () => {
    expect(extractPrefix('milestone:BEDROCK')).toBe('milestone');
  });

  it('returns null when no colon is present', () => {
    expect(extractPrefix('noprefix')).toBeNull();
  });

  it('extracts prefix from ID with multiple colons', () => {
    // Only first colon matters
    expect(extractPrefix('task:some:thing')).toBe('task');
  });

  it('returns null for non-string input', () => {
    expect(extractPrefix(null)).toBeNull();
    expect(extractPrefix(undefined)).toBeNull();
    expect(extractPrefix(42)).toBeNull();
  });
});

describe('validateNodeId', () => {
  it('accepts a valid node ID', () => {
    expect(validateNodeId('task:BDK-001')).toEqual({ valid: true });
  });

  it('accepts IDs with dots, slashes, and @', () => {
    expect(validateNodeId('file:src/auth.js')).toEqual({ valid: true });
    expect(validateNodeId('pkg:@scope/name')).toEqual({ valid: true });
  });

  it('rejects empty string', () => {
    const r = validateNodeId('');
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/non-empty/);
  });

  it('rejects prefix-only (no identifier)', () => {
    const r = validateNodeId('milestone:');
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/Invalid node ID/);
  });

  it('rejects colon-only', () => {
    const r = validateNodeId(':');
    expect(r.valid).toBe(false);
  });

  it('rejects missing prefix (starts with colon)', () => {
    const r = validateNodeId(':foo');
    expect(r.valid).toBe(false);
  });

  it('rejects uppercase prefix', () => {
    const r = validateNodeId('Milestone:BEDROCK');
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/Invalid node ID/);
  });

  it('rejects whitespace in ID', () => {
    const r = validateNodeId('task:my thing');
    expect(r.valid).toBe(false);
  });

  it('accepts ID at exactly max length', () => {
    const prefix = 'task:';
    const id = prefix + 'a'.repeat(NODE_ID_MAX_LENGTH - prefix.length);
    expect(id.length).toBe(NODE_ID_MAX_LENGTH);
    expect(validateNodeId(id).valid).toBe(true);
  });

  it('rejects ID exceeding max length', () => {
    const prefix = 'task:';
    const id = prefix + 'a'.repeat(NODE_ID_MAX_LENGTH - prefix.length + 1);
    expect(id.length).toBe(NODE_ID_MAX_LENGTH + 1);
    const r = validateNodeId(id);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/max length/);
  });

  it('rejects non-string input', () => {
    expect(validateNodeId(null).valid).toBe(false);
    expect(validateNodeId(undefined).valid).toBe(false);
    expect(validateNodeId(42).valid).toBe(false);
  });

  it('rejects bare identifier without prefix', () => {
    const r = validateNodeId('noprefix');
    expect(r.valid).toBe(false);
  });
});

describe('classifyPrefix', () => {
  it('classifies canonical prefixes', () => {
    expect(classifyPrefix('milestone')).toBe('canonical');
    expect(classifyPrefix('task')).toBe('canonical');
    expect(classifyPrefix('file')).toBe('canonical');
  });

  it('classifies system prefixes', () => {
    expect(classifyPrefix('commit')).toBe('system');
  });

  it('classifies commit as system (not canonical) due to precedence', () => {
    // SYSTEM_PREFIXES is checked before CANONICAL_PREFIXES in classifyPrefix,
    // and commit is only in SYSTEM_PREFIXES — not in CANONICAL_PREFIXES
    expect(classifyPrefix('commit')).toBe('system');
    expect(SYSTEM_PREFIXES).toContain('commit');
    expect(CANONICAL_PREFIXES).not.toContain('commit');
  });

  it('classifies unknown prefixes', () => {
    expect(classifyPrefix('banana')).toBe('unknown');
    expect(classifyPrefix('custom')).toBe('unknown');
  });
});

describe('validateEdgeType', () => {
  it('accepts all valid edge types', () => {
    for (const type of EDGE_TYPES) {
      expect(validateEdgeType(type)).toEqual({ valid: true });
    }
  });

  it('rejects unknown edge types', () => {
    const r = validateEdgeType('explodes');
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/Unknown edge type/);
  });
});

describe('validateConfidence', () => {
  it('accepts 0.0', () => {
    expect(validateConfidence(0.0)).toEqual({ valid: true });
  });

  it('accepts 0.5', () => {
    expect(validateConfidence(0.5)).toEqual({ valid: true });
  });

  it('accepts 1.0', () => {
    expect(validateConfidence(1.0)).toEqual({ valid: true });
  });

  it('rejects 1.5 (out of range)', () => {
    const r = validateConfidence(1.5);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/between 0\.0 and 1\.0/);
  });

  it('rejects -0.1 (out of range)', () => {
    const r = validateConfidence(-0.1);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/between 0\.0 and 1\.0/);
  });

  it('rejects NaN', () => {
    const r = validateConfidence(NaN);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/finite number/);
  });

  it('rejects Infinity', () => {
    const r = validateConfidence(Infinity);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/finite number/);
  });

  it('rejects string "0.9"', () => {
    const r = validateConfidence('0.9');
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/must be a number/);
  });

  it('rejects null', () => {
    const r = validateConfidence(null);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/must be a number/);
  });

  it('rejects undefined', () => {
    const r = validateConfidence(undefined);
    expect(r.valid).toBe(false);
    expect(r.error).toMatch(/must be a number/);
  });
});

describe('validateEdge', () => {
  it('passes for a valid edge', () => {
    const r = validateEdge('task:BDK-001', 'feature:BDK-SCHEMA', 'implements', 1.0);
    expect(r.valid).toBe(true);
    expect(r.errors).toEqual([]);
    expect(r.warnings).toEqual([]);
  });

  it('passes without confidence (optional)', () => {
    const r = validateEdge('task:BDK-001', 'feature:BDK-SCHEMA', 'implements');
    expect(r.valid).toBe(true);
  });

  it('rejects self-edge for blocks', () => {
    const r = validateEdge('task:X', 'task:X', 'blocks');
    expect(r.valid).toBe(false);
    expect(r.errors.some(e => /self-edge/i.test(e))).toBe(true);
  });

  it('rejects self-edge for depends-on', () => {
    const r = validateEdge('task:X', 'task:X', 'depends-on');
    expect(r.valid).toBe(false);
    expect(r.errors.some(e => /self-edge/i.test(e))).toBe(true);
  });

  it('allows self-edge for relates-to', () => {
    const r = validateEdge('task:X', 'task:X', 'relates-to');
    expect(r.valid).toBe(true);
  });

  it('warns on unknown prefix', () => {
    const r = validateEdge('banana:X', 'task:Y', 'relates-to');
    expect(r.valid).toBe(true);
    expect(r.warnings.length).toBe(1);
    expect(r.warnings[0]).toMatch(/banana/);
  });

  it('collects multiple errors', () => {
    const r = validateEdge('', '', 'explodes', 'bad');
    expect(r.valid).toBe(false);
    // 2 invalid IDs + unknown edge type + bad confidence = 4 errors
    // Self-edge check is skipped because IDs are invalid
    expect(r.errors.length).toBe(4);
  });

  it('skips self-edge check when IDs are already invalid', () => {
    const r = validateEdge('bad', 'bad', 'blocks');
    expect(r.valid).toBe(false);
    // Should report invalid IDs but NOT a redundant self-edge error
    expect(r.errors.some(e => /self-edge/i.test(e))).toBe(false);
  });
});

describe('constants', () => {
  it('NODE_ID_REGEX is a RegExp', () => {
    expect(NODE_ID_REGEX).toBeInstanceOf(RegExp);
  });

  it('NODE_ID_MAX_LENGTH is 256', () => {
    expect(NODE_ID_MAX_LENGTH).toBe(256);
  });

  it('CANONICAL_PREFIXES has 18 user-facing entries', () => {
    expect(CANONICAL_PREFIXES.length).toBe(18);
    expect(CANONICAL_PREFIXES).toContain('milestone');
    expect(CANONICAL_PREFIXES).not.toContain('commit');
  });

  it('SYSTEM_PREFIXES contains commit', () => {
    expect(SYSTEM_PREFIXES).toEqual(['commit']);
  });

  it('ALL_PREFIXES is the union of canonical and system', () => {
    expect(ALL_PREFIXES.length).toBe(CANONICAL_PREFIXES.length + SYSTEM_PREFIXES.length);
    expect(ALL_PREFIXES).toContain('milestone');
    expect(ALL_PREFIXES).toContain('commit');
  });

  it('EDGE_TYPES has 8 entries', () => {
    expect(EDGE_TYPES.length).toBe(8);
  });
});
