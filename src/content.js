/**
 * @module content
 * Content-on-node: attach rich content (markdown, text, etc.) to graph nodes
 * using git-warp's native content-addressed storage.
 *
 * Content is stored as git blobs via WARP's patch.attachContent(). The blob OID
 * and metadata are recorded as WARP node properties.
 *
 * Property convention:
 *   _content        — git blob OID (managed by WARP via CONTENT_PROPERTY_KEY)
 *   _content.mime   — MIME type (e.g. "text/markdown")
 *   _content.size   — byte count
 */

import { CONTENT_PROPERTY_KEY } from '@git-stunts/git-warp';

const MIME_KEY = '_content.mime';
const SIZE_KEY = '_content.size';

/**
 * @typedef {object} ContentMeta
 * @property {string} sha - Git blob OID
 * @property {string} mime - MIME type
 * @property {number} size - Content size in bytes
 */

/**
 * @typedef {object} WriteContentResult
 * @property {string} nodeId - Target node
 * @property {string} sha - Written blob OID
 * @property {string} mime - MIME type
 * @property {number} size - Byte count
 */

/**
 * Write content to a graph node. Stores the content as a git blob via WARP's
 * native content API and records metadata as node properties.
 *
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @param {Buffer|string} content - Content to store
 * @param {{ mime?: string }} [opts]
 * @returns {Promise<WriteContentResult>}
 */
export async function writeContent(graph, nodeId, content, opts = {}) {
  const exists = await graph.hasNode(nodeId);
  if (!exists) {
    throw new Error(`Node not found: ${nodeId}`);
  }

  const buf = Buffer.isBuffer(content) ? content : Buffer.from(content, 'utf-8');
  const mime = opts.mime ?? 'text/plain';
  const size = buf.length;

  const patch = await graph.createPatch();
  await patch.attachContent(nodeId, buf);
  patch.setProperty(nodeId, MIME_KEY, mime);
  patch.setProperty(nodeId, SIZE_KEY, size);
  await patch.commit();

  const sha = await graph.getContentOid(nodeId);

  if (sha == null) {
    throw new Error(`Failed to retrieve OID after writing content to node: ${nodeId}`);
  }

  return { nodeId, sha, mime, size };
}

/**
 * Read content attached to a graph node. Retrieves the blob from WARP's
 * native content store.
 *
 * @param {import('@git-stunts/git-warp').default} graph - WARP graph instance
 * @param {string} nodeId - Target node ID
 * @returns {Promise<{ content: string, meta: ContentMeta }>}
 */
export async function readContent(graph, nodeId) {
  const meta = await getContentMeta(graph, nodeId);
  if (!meta) {
    throw new Error(`No content attached to node: ${nodeId}`);
  }

  let contentBuf;
  try {
    contentBuf = await graph.getContent(nodeId);
  } catch (err) {
    throw new Error(
      `Failed to retrieve content blob ${meta.sha} for node: ${nodeId}`,
      { cause: err },
    );
  }

  if (contentBuf == null || (contentBuf.length === 0 && meta.size > 0)) {
    throw new Error(
      `Failed to retrieve content blob ${meta.sha} for node: ${nodeId}`,
    );
  }

  return { content: contentBuf.toString('utf-8'), meta };
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

  const sha = await graph.getContentOid(nodeId);
  if (!sha) return null;

  const propsMap = await graph.getNodeProps(nodeId);

  return {
    sha,
    mime: propsMap?.get(MIME_KEY) ?? 'text/plain',
    size: propsMap?.get(SIZE_KEY) ?? 0,
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

  const sha = await graph.getContentOid(nodeId);
  return sha != null;
}

/**
 * Delete content from a node by clearing the content properties.
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

  const previousSha = await graph.getContentOid(nodeId);

  if (!previousSha) {
    return { nodeId, removed: false, previousSha: null };
  }

  const patch = await graph.createPatch();
  patch.setProperty(nodeId, CONTENT_PROPERTY_KEY, null);
  patch.setProperty(nodeId, MIME_KEY, null);
  patch.setProperty(nodeId, SIZE_KEY, null);
  await patch.commit();

  return { nodeId, removed: true, previousSha };
}
