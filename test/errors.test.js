import { describe, it, expect } from 'vitest';
import { GmindError, ExitCode, ERROR_CATALOG } from '../src/errors.js';

describe('GmindError (#207)', () => {
  it('carries code, exitCode, hint, and message', () => {
    const err = new GmindError('GMIND_E_NODE_NOT_FOUND', 'Node task:X not found');
    expect(err.message).toBe('Node task:X not found');
    expect(err.code).toBe('GMIND_E_NODE_NOT_FOUND');
    expect(err.exitCode).toBe(ExitCode.NOT_FOUND);
    expect(err.hint).toBe('Use "git mind nodes" to list existing nodes');
    expect(err.name).toBe('GmindError');
    expect(err).toBeInstanceOf(Error);
  });

  it('allows hint override', () => {
    const err = new GmindError('GMIND_E_USAGE', 'bad flag', { hint: 'Try --help' });
    expect(err.hint).toBe('Try --help');
    expect(err.exitCode).toBe(ExitCode.USAGE);
  });

  it('allows exitCode override', () => {
    const err = new GmindError('GMIND_E_INTERNAL', 'boom', { exitCode: 99 });
    expect(err.exitCode).toBe(99);
  });

  it('chains cause', () => {
    const root = new Error('disk full');
    const err = new GmindError('GMIND_E_EXPORT', 'export failed', { cause: root });
    expect(err.cause).toBe(root);
  });

  it('falls back to INTERNAL for unknown codes', () => {
    const err = new GmindError('GMIND_E_UNKNOWN_BOGUS', 'wat');
    expect(err.code).toBe('GMIND_E_INTERNAL');
    expect(err.exitCode).toBe(ExitCode.GENERAL);
    expect(err.hint).toContain('Unknown error code');
  });

  it('toJSON() returns structured envelope', () => {
    const err = new GmindError('GMIND_E_VALIDATION', 'bad schema');
    const json = err.toJSON();
    expect(json).toEqual({
      error: 'bad schema',
      errorCode: 'GMIND_E_VALIDATION',
      exitCode: ExitCode.VALIDATION,
      hint: 'Check input data against the expected schema',
    });
  });

  it('is serializable via JSON.stringify', () => {
    const err = new GmindError('GMIND_E_NOT_FOUND', 'missing');
    const parsed = JSON.parse(JSON.stringify(err));
    expect(parsed.errorCode).toBe('GMIND_E_NOT_FOUND');
    expect(parsed.error).toBe('missing');
  });
});

describe('ExitCode', () => {
  it('defines expected values', () => {
    expect(ExitCode.SUCCESS).toBe(0);
    expect(ExitCode.GENERAL).toBe(1);
    expect(ExitCode.USAGE).toBe(2);
    expect(ExitCode.VALIDATION).toBe(3);
    expect(ExitCode.NOT_FOUND).toBe(4);
  });

  it('is frozen', () => {
    expect(Object.isFrozen(ExitCode)).toBe(true);
  });
});

describe('ERROR_CATALOG', () => {
  it('every entry has exit and hint', () => {
    for (const [code, entry] of Object.entries(ERROR_CATALOG)) {
      expect(typeof entry.exit).toBe('number');
      expect(typeof entry.hint).toBe('string');
      expect(entry.hint.length).toBeGreaterThan(0);
      expect(code.startsWith('GMIND_E_')).toBe(true);
    }
  });

  it('is frozen', () => {
    expect(Object.isFrozen(ERROR_CATALOG)).toBe(true);
  });

  it('has entries for all documented error codes', () => {
    const expected = [
      'GMIND_E_USAGE',
      'GMIND_E_UNKNOWN_CMD',
      'GMIND_E_NOT_INITIALIZED',
      'GMIND_E_INVALID_ID',
      'GMIND_E_INVALID_EDGE',
      'GMIND_E_VALIDATION',
      'GMIND_E_NODE_NOT_FOUND',
      'GMIND_E_NOT_FOUND',
      'GMIND_E_IMPORT',
      'GMIND_E_EXPORT',
      'GMIND_E_EXTENSION',
      'GMIND_E_CONTENT',
      'GMIND_E_INTERNAL',
    ];
    for (const code of expected) {
      expect(ERROR_CATALOG).toHaveProperty(code);
    }
  });
});
