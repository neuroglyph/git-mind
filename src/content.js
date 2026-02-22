/**
 * @module content
 * Content-on-node: attach rich content (markdown, text, etc.) to graph nodes
 * using git's native content-addressed storage.
 *
 * Content is stored as git blobs via `git hash-object -w`. The blob SHA and
 * metadata are recorded as WARP node properties under the `_content.` prefix.
 *
 * Property convention:
 *   _content.sha      — git blob SHA
 *   _content.mime     — MIME type (e.g. "text/markdown")
 *   _content.size     — byte count
 *   _content.encoding — "utf-8" | "base64"
 */

import { execSync } from 'node:child_process';

/** Property key prefix for content metadata. */
const PREFIX = '_content.';

/** Known content property keys. */
const KEYS = {
  sha: `${PREFIX}sha`,
  mime: `${PREFIX}mime`,
  size: `${PREFIX}size`,
  encoding: `${PREFIX}encoding`,
};

/**
 * @typedef {object} ContentMeta
 * @property {string} sha - Git blob SHA
 * @property {string} mime - MIME type
 * @property {number} size - Content size in bytes
 * @property {string} encoding - Content encoding
 */

/**
 * @typedef {object} WriteContentResult
 * @property {string} nodeId - Target node
 * @property {string} sha - Written blob SHA
 * @property {string} mime - MIME type
 * @property {number} size - Byte count
 * @property {string} encoding - Content encoding
 */

/**
 * Write content to a graph node. Stores the content as a git blob and records
 * metadata as node properties.
 *
 * @param {string} cwd - Repository working directory
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @param {Buffer|string} content - Content to store
 * @param {{ mime?: string, encoding?: string }} [opts]
 * @returns {Promise<WriteContentResult>}
 */
export async function writeContent(cwd, graph, nodeId, content, opts = {}) {
  const exists = await graph.hasNode(nodeId);
  if (!exists) {
    throw new Error(`Node not found: ${nodeId}`);
  }

  const buf = Buffer.isBuffer(content) ? content : Buffer.from(content, 'utf-8');
  const mime = opts.mime ?? 'text/plain';
  const encoding = opts.encoding ?? 'utf-8';
  const size = buf.length;

  // Write blob to git object store
  const sha = execSync('git hash-object -w --stdin', {
    cwd,
    input: buf,
    encoding: 'utf-8',
  }).trim();

  // Record metadata as node properties
  const patch = await graph.createPatch();
  patch.setProperty(nodeId, KEYS.sha, sha);
  patch.setProperty(nodeId, KEYS.mime, mime);
  patch.setProperty(nodeId, KEYS.size, size);
  patch.setProperty(nodeId, KEYS.encoding, encoding);
  await patch.commit();

  return { nodeId, sha, mime, size, encoding };
}

/**
 * Read content attached to a graph node. Retrieves the blob from git's object
 * store and verifies SHA integrity.
 *
 * @param {string} cwd - Repository working directory
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @returns {Promise<{ content: string, meta: ContentMeta }>}
 */
export async function readContent(cwd, graph, nodeId) {
  const meta = await getContentMeta(graph, nodeId);
  if (!meta) {
    throw new Error(`No content attached to node: ${nodeId}`);
  }

  // Retrieve blob from git object store
  let content;
  try {
    content = execSync(`git cat-file blob ${meta.sha}`, {
      cwd,
      encoding: meta.encoding === 'base64' ? 'buffer' : 'utf-8',
    });
  } catch {
    throw new Error(
      `Content blob ${meta.sha} not found in git object store for node: ${nodeId}`,
    );
  }

  // Verify integrity: re-hash and compare
  const verifyBuf = Buffer.isBuffer(content) ? content : Buffer.from(content, 'utf-8');
  const verifySha = execSync('git hash-object --stdin', {
    cwd,
    input: verifyBuf,
    encoding: 'utf-8',
  }).trim();

  if (verifySha !== meta.sha) {
    throw new Error(
      `Content integrity check failed for node ${nodeId}: ` +
      `expected ${meta.sha}, got ${verifySha}`,
    );
  }

  return {
    content: Buffer.isBuffer(content) ? content.toString('base64') : content,
    meta,
  };
}

/**
 * Get content metadata for a node without retrieving the blob.
 * Returns null if no content is attached.
 *
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @returns {Promise<ContentMeta|null>}
 */
export async function getContentMeta(graph, nodeId) {
  const exists = await graph.hasNode(nodeId);
  if (!exists) {
    throw new Error(`Node not found: ${nodeId}`);
  }

  const propsMap = await graph.getNodeProps(nodeId);
  const sha = propsMap?.get(KEYS.sha) ?? null;
  if (!sha) return null;

  return {
    sha,
    mime: propsMap.get(KEYS.mime) ?? 'text/plain',
    size: propsMap.get(KEYS.size) ?? 0,
    encoding: propsMap.get(KEYS.encoding) ?? 'utf-8',
  };
}

/**
 * Check whether a node has content attached.
 *
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @returns {Promise<boolean>}
 */
export async function hasContent(graph, nodeId) {
  const exists = await graph.hasNode(nodeId);
  if (!exists) return false;

  const propsMap = await graph.getNodeProps(nodeId);
  const sha = propsMap?.get(KEYS.sha) ?? null;
  return sha !== null && sha !== undefined;
}

/**
 * Delete content from a node by clearing the `_content.*` properties.
 * The git blob remains in the object store (cleaned up by git gc).
 *
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @returns {Promise<{ nodeId: string, removed: boolean, previousSha: string|null }>}
 */
export async function deleteContent(graph, nodeId) {
  const exists = await graph.hasNode(nodeId);
  if (!exists) {
    throw new Error(`Node not found: ${nodeId}`);
  }

  const propsMap = await graph.getNodeProps(nodeId);
  const previousSha = propsMap?.get(KEYS.sha) ?? null;

  if (!previousSha) {
    return { nodeId, removed: false, previousSha: null };
  }

  const patch = await graph.createPatch();
  patch.setProperty(nodeId, KEYS.sha, null);
  patch.setProperty(nodeId, KEYS.mime, null);
  patch.setProperty(nodeId, KEYS.size, null);
  patch.setProperty(nodeId, KEYS.encoding, null);
  await patch.commit();

  return { nodeId, removed: true, previousSha };
}
