import { describe, it, expect } from 'vitest';
import { createContext, DEFAULT_CONTEXT } from '../src/context-envelope.js';

describe('ContextEnvelope', () => {
  describe('DEFAULT_CONTEXT', () => {
    it('has expected default values', () => {
      expect(DEFAULT_CONTEXT.asOf).toBe('HEAD');
      expect(DEFAULT_CONTEXT.observer).toBeNull();
      expect(DEFAULT_CONTEXT.trustPolicy).toBe('open');
      expect(DEFAULT_CONTEXT.extensionLock).toBeNull();
    });

    it('is frozen (immutable)', () => {
      expect(Object.isFrozen(DEFAULT_CONTEXT)).toBe(true);
    });
  });

  describe('createContext()', () => {
    it('returns defaults when called with no arguments', () => {
      const ctx = createContext();
      expect(ctx).toEqual(DEFAULT_CONTEXT);
    });

    it('returns defaults when called with empty object', () => {
      const ctx = createContext({});
      expect(ctx).toEqual(DEFAULT_CONTEXT);
    });

    it('overrides only specified fields', () => {
      const ctx = createContext({ asOf: 'main~5' });
      expect(ctx.asOf).toBe('main~5');
      expect(ctx.observer).toBeNull();
      expect(ctx.trustPolicy).toBe('open');
      expect(ctx.extensionLock).toBeNull();
    });

    it('overrides observer', () => {
      const ctx = createContext({ observer: 'alice' });
      expect(ctx.observer).toBe('alice');
      expect(ctx.asOf).toBe('HEAD');
    });

    it('overrides trustPolicy', () => {
      const ctx = createContext({ trustPolicy: 'strict' });
      expect(ctx.trustPolicy).toBe('strict');
    });

    it('overrides extensionLock', () => {
      const ctx = createContext({ extensionLock: 'abc123' });
      expect(ctx.extensionLock).toBe('abc123');
    });

    it('overrides multiple fields at once', () => {
      const ctx = createContext({ asOf: 'v1.0.0', observer: 'bob', trustPolicy: 'strict', extensionLock: 'deadbeef' });
      expect(ctx.asOf).toBe('v1.0.0');
      expect(ctx.observer).toBe('bob');
      expect(ctx.trustPolicy).toBe('strict');
      expect(ctx.extensionLock).toBe('deadbeef');
    });

    it('returns a frozen object', () => {
      expect(Object.isFrozen(createContext())).toBe(true);
      expect(Object.isFrozen(createContext({ asOf: 'main~3' }))).toBe(true);
    });

    it('does not mutate DEFAULT_CONTEXT', () => {
      createContext({ asOf: 'feature-branch' });
      expect(DEFAULT_CONTEXT.asOf).toBe('HEAD');
    });

    it('each call returns a distinct object', () => {
      const a = createContext();
      const b = createContext();
      expect(a).not.toBe(b);
    });
  });
});
