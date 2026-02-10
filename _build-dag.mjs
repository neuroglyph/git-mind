#!/usr/bin/env node
/**
 * _build-dag.mjs — Materialize the ROADMAP DAG into git-mind's own graph.
 *
 * Adds 81 nodes (7 milestones, 27 features, 46 tasks, 1 doc) and 167 edges
 * using the git-warp graph API directly.
 *
 * Run: node _build-dag.mjs
 * Ref: #180
 */

import { loadGraph } from './src/graph.js';

const CWD = process.cwd();

// ──────────────────────────────────────────────────────────────────
// NODE DEFINITIONS
// ──────────────────────────────────────────────────────────────────

const milestones = [
  { id: 'milestone:BEDROCK',         name: 'BEDROCK',         theme: 'Schema & Node Foundations',    features: 4, tasks: 8,  estHours: 28, order: 1 },
  { id: 'milestone:INTAKE',          name: 'INTAKE',          theme: 'Data Ingestion Pipeline',      features: 4, tasks: 8,  estHours: 34, order: 2 },
  { id: 'milestone:PRISM',           name: 'PRISM',           theme: 'Views & Value Surfaces',       features: 5, tasks: 7,  estHours: 30, order: 3 },
  { id: 'milestone:WATCHTOWER',      name: 'WATCHTOWER',      theme: 'Dashboard & Observability',    features: 3, tasks: 5,  estHours: 18, order: 4 },
  { id: 'milestone:PROVING-GROUND',  name: 'PROVING GROUND',  theme: 'Dogfood Validation',           features: 3, tasks: 5,  estHours: 16, order: 5 },
  { id: 'milestone:ORACLE',          name: 'ORACLE',          theme: 'AI Intelligence & Curation',   features: 4, tasks: 7,  estHours: 40, order: 6 },
  { id: 'milestone:NEXUS',           name: 'NEXUS',           theme: 'Integration & Federation',     features: 4, tasks: 6,  estHours: 36, order: 7 },
];

const features = [
  // BEDROCK
  { id: 'feature:BDK-SCHEMA',    name: 'Schema Contract',           milestone: 'milestone:BEDROCK' },
  { id: 'feature:BDK-NODES',     name: 'Node Query Layer',          milestone: 'milestone:BEDROCK' },
  { id: 'feature:BDK-CLI',       name: 'Node CLI Command',          milestone: 'milestone:BEDROCK' },
  { id: 'feature:BDK-TEST',      name: 'Foundation Test Suite',     milestone: 'milestone:BEDROCK' },
  // INTAKE
  { id: 'feature:INT-ENGINE',    name: 'YAML Import Engine',        milestone: 'milestone:INTAKE' },
  { id: 'feature:INT-CLI',       name: 'Import CLI Command',        milestone: 'milestone:INTAKE' },
  { id: 'feature:INT-GUARD',     name: 'Schema Enforcement',        milestone: 'milestone:INTAKE' },
  { id: 'feature:INT-ATOMIC',    name: 'Atomic Writes',             milestone: 'milestone:INTAKE' },
  // PRISM
  { id: 'feature:PRI-ENGINE',    name: 'Declarative View Engine',   milestone: 'milestone:PRISM' },
  { id: 'feature:PRI-MILESTONE', name: 'Milestone View',            milestone: 'milestone:PRISM' },
  { id: 'feature:PRI-TRACE',     name: 'Traceability View',         milestone: 'milestone:PRISM' },
  { id: 'feature:PRI-BLOCK',     name: 'Blockers View',             milestone: 'milestone:PRISM' },
  { id: 'feature:PRI-ONBOARD',   name: 'Onboarding View',           milestone: 'milestone:PRISM' },
  // WATCHTOWER
  { id: 'feature:WTC-STATUS',    name: 'Status Command',            milestone: 'milestone:WATCHTOWER' },
  { id: 'feature:WTC-METRICS',   name: 'Graph Metrics',             milestone: 'milestone:WATCHTOWER' },
  { id: 'feature:WTC-TEST',      name: 'Dashboard Test Suite',      milestone: 'milestone:WATCHTOWER' },
  // PROVING GROUND
  { id: 'feature:PRV-SEED',      name: 'Echo Seed',                 milestone: 'milestone:PROVING-GROUND' },
  { id: 'feature:PRV-VALIDATE',  name: 'Validation Suite',          milestone: 'milestone:PROVING-GROUND' },
  { id: 'feature:PRV-DOCS',      name: 'Demo Documentation',        milestone: 'milestone:PROVING-GROUND' },
  // ORACLE
  { id: 'feature:ORC-SUGGEST',   name: 'AI Suggestions',            milestone: 'milestone:ORACLE' },
  { id: 'feature:ORC-REVIEW',    name: 'Interactive Review',        milestone: 'milestone:ORACLE' },
  { id: 'feature:ORC-LEARN',     name: 'Curation Loop',             milestone: 'milestone:ORACLE' },
  { id: 'feature:ORC-DOCTOR',    name: 'Integrity Checks',          milestone: 'milestone:ORACLE' },
  // NEXUS
  { id: 'feature:NEX-ACTION',    name: 'GitHub Action',             milestone: 'milestone:NEXUS' },
  { id: 'feature:NEX-MARKDOWN',  name: 'Markdown Import',           milestone: 'milestone:NEXUS' },
  { id: 'feature:NEX-FEDERATION',name: 'Multi-Repo Federation',     milestone: 'milestone:NEXUS' },
  { id: 'feature:NEX-EXPORT',    name: 'Round-Trip Export',          milestone: 'milestone:NEXUS' },
];

const tasks = [
  // BEDROCK
  { id: 'task:BDK-001', name: 'Write GRAPH_SCHEMA.md Specification',   feature: 'feature:BDK-SCHEMA', estHours: 3 },
  { id: 'task:BDK-002', name: 'Implement Schema Runtime Validators',   feature: 'feature:BDK-SCHEMA', estHours: 3 },
  { id: 'task:BDK-003', name: 'Implement Node Query & Inspection API', feature: 'feature:BDK-NODES',  estHours: 4 },
  { id: 'task:BDK-004', name: 'Node Property Getters & Metadata',      feature: 'feature:BDK-NODES',  estHours: 2 },
  { id: 'task:BDK-005', name: 'Implement git mind nodes Command',      feature: 'feature:BDK-CLI',    estHours: 3 },
  { id: 'task:BDK-006', name: 'Node Command Flag Integration',         feature: 'feature:BDK-CLI',    estHours: 1 },
  { id: 'task:BDK-007', name: 'Schema Validation Test Suite',          feature: 'feature:BDK-TEST',   estHours: 3 },
  { id: 'task:BDK-008', name: 'Node Query Test Suite',                 feature: 'feature:BDK-TEST',   estHours: 3 },
  // INTAKE
  { id: 'task:INT-001', name: 'Implement Core Import Logic',           feature: 'feature:INT-ENGINE',  estHours: 5 },
  { id: 'task:INT-002', name: 'Implement Idempotent Merge Semantics',  feature: 'feature:INT-ENGINE',  estHours: 3 },
  { id: 'task:INT-003', name: 'Implement git mind import Command',     feature: 'feature:INT-CLI',     estHours: 2 },
  { id: 'task:INT-004', name: 'Import Command Flags (dry-run, etc)',   feature: 'feature:INT-CLI',     estHours: 2 },
  { id: 'task:INT-005', name: 'Version Field Enforcement',             feature: 'feature:INT-GUARD',   estHours: 1 },
  { id: 'task:INT-006', name: 'Reference Validation',                  feature: 'feature:INT-GUARD',   estHours: 2 },
  { id: 'task:INT-007', name: 'Build-Validate-Write Pipeline',         feature: 'feature:INT-ATOMIC',  estHours: 3 },
  { id: 'task:INT-008', name: 'Import Test Suite',                     feature: 'feature:INT-ATOMIC',  estHours: 4 },
  // PRISM
  { id: 'task:PRI-001', name: 'Refactor View System to Declarative',   feature: 'feature:PRI-ENGINE',    estHours: 5 },
  { id: 'task:PRI-002', name: 'View Config Schema & Validation',       feature: 'feature:PRI-ENGINE',    estHours: 2 },
  { id: 'task:PRI-003', name: 'Implement Milestone Progress View',     feature: 'feature:PRI-MILESTONE', estHours: 3 },
  { id: 'task:PRI-004', name: 'Implement Spec-to-Impl Traceability',   feature: 'feature:PRI-TRACE',     estHours: 3 },
  { id: 'task:PRI-005', name: 'Implement Blocker Analysis View',       feature: 'feature:PRI-BLOCK',     estHours: 4 },
  { id: 'task:PRI-006', name: 'Implement Onboarding Reading Order',    feature: 'feature:PRI-ONBOARD',   estHours: 3 },
  { id: 'task:PRI-007', name: 'View Fixture Test Suite',               feature: 'feature:PRI-ONBOARD',   estHours: 4 },
  // WATCHTOWER
  { id: 'task:WTC-001', name: 'Implement git mind status Command',     feature: 'feature:WTC-STATUS',  estHours: 4 },
  { id: 'task:WTC-002', name: 'Status Command --json Flag',            feature: 'feature:WTC-STATUS',  estHours: 1 },
  { id: 'task:WTC-003', name: 'Node Metrics by Prefix',                feature: 'feature:WTC-METRICS', estHours: 1 },
  { id: 'task:WTC-004', name: 'Edge Metrics & Quality Signals',        feature: 'feature:WTC-METRICS', estHours: 2 },
  { id: 'task:WTC-005', name: 'Status Output Test Suite',              feature: 'feature:WTC-TEST',    estHours: 2 },
  // PROVING GROUND
  { id: 'task:PRV-001', name: 'Create Echo Project YAML Seed File',    feature: 'feature:PRV-SEED',     estHours: 4 },
  { id: 'task:PRV-002', name: 'Import Seed into Echo Repo',            feature: 'feature:PRV-SEED',     estHours: 1 },
  { id: 'task:PRV-003', name: 'Answer 5 Echo Questions via CLI',       feature: 'feature:PRV-VALIDATE', estHours: 3 },
  { id: 'task:PRV-004', name: 'Validate Answers Against Known Truth',  feature: 'feature:PRV-VALIDATE', estHours: 2 },
  { id: 'task:PRV-005', name: 'Record and Commit Demo Transcript',     feature: 'feature:PRV-DOCS',     estHours: 2 },
  // ORACLE
  { id: 'task:ORC-001', name: 'Implement suggest --ai with LLM',       feature: 'feature:ORC-SUGGEST', estHours: 8 },
  { id: 'task:ORC-002', name: 'Context Extraction from Code',          feature: 'feature:ORC-SUGGEST', estHours: 5 },
  { id: 'task:ORC-003', name: 'Implement git mind review Flow',        feature: 'feature:ORC-REVIEW',  estHours: 6 },
  { id: 'task:ORC-004', name: 'Accept/Reject/Adjust Edge Workflow',    feature: 'feature:ORC-REVIEW',  estHours: 3 },
  { id: 'task:ORC-005', name: 'Review History Provenance',             feature: 'feature:ORC-LEARN',   estHours: 5 },
  { id: 'task:ORC-006', name: 'Implement git mind doctor Command',     feature: 'feature:ORC-DOCTOR',  estHours: 5 },
  { id: 'task:ORC-007', name: 'Dangling/Orphan/Duplicate Detection',   feature: 'feature:ORC-DOCTOR',  estHours: 4 },
  // NEXUS
  { id: 'task:NEX-001', name: 'Create GitHub Action for PR Suggests',  feature: 'feature:NEX-ACTION',     estHours: 6 },
  { id: 'task:NEX-002', name: 'PR Reviewer Edge Display',              feature: 'feature:NEX-ACTION',     estHours: 4 },
  { id: 'task:NEX-003', name: 'Import from Markdown Frontmatter',      feature: 'feature:NEX-MARKDOWN',   estHours: 5 },
  { id: 'task:NEX-004', name: 'Cross-Repo Edge Protocol',              feature: 'feature:NEX-FEDERATION', estHours: 6 },
  { id: 'task:NEX-005', name: 'Multi-Repo Graph Merge',                feature: 'feature:NEX-FEDERATION', estHours: 6 },
  { id: 'task:NEX-006', name: 'Implement git mind export',             feature: 'feature:NEX-EXPORT',     estHours: 4 },
];

const doc = { id: 'doc:ROADMAP', name: 'ROADMAP.md', path: 'ROADMAP.md' };

// ──────────────────────────────────────────────────────────────────
// EDGE DEFINITIONS
// ──────────────────────────────────────────────────────────────────

// Milestone ordering (depends-on)
const milestoneDeps = [
  ['milestone:INTAKE',         'milestone:BEDROCK'],
  ['milestone:PRISM',          'milestone:BEDROCK'],
  ['milestone:WATCHTOWER',     'milestone:BEDROCK'],
  ['milestone:WATCHTOWER',     'milestone:PRISM'],
  ['milestone:PROVING-GROUND', 'milestone:BEDROCK'],
  ['milestone:PROVING-GROUND', 'milestone:INTAKE'],
  ['milestone:PROVING-GROUND', 'milestone:PRISM'],
  ['milestone:PROVING-GROUND', 'milestone:WATCHTOWER'],
  ['milestone:ORACLE',         'milestone:PROVING-GROUND'],
  ['milestone:NEXUS',          'milestone:ORACLE'],
];

// Task dependency DAG (blocks) — source blocks target
const blocks = [
  // BEDROCK internal
  ['task:BDK-001', 'task:BDK-002'],
  ['task:BDK-001', 'task:BDK-003'],
  ['task:BDK-003', 'task:BDK-004'],
  ['task:BDK-003', 'task:BDK-005'],
  ['task:BDK-004', 'task:BDK-005'],
  ['task:BDK-005', 'task:BDK-006'],
  ['task:BDK-002', 'task:BDK-007'],
  ['task:BDK-003', 'task:BDK-008'],
  ['task:BDK-004', 'task:BDK-008'],
  // BEDROCK → INTAKE
  ['task:BDK-002', 'task:INT-001'],
  ['task:BDK-003', 'task:INT-001'],
  ['task:BDK-004', 'task:INT-002'],
  ['task:BDK-002', 'task:INT-005'],
  ['task:BDK-007', 'task:INT-005'],
  ['task:BDK-002', 'task:INT-006'],
  // INTAKE internal
  ['task:INT-001', 'task:INT-002'],
  ['task:INT-001', 'task:INT-003'],
  ['task:INT-003', 'task:INT-004'],
  ['task:INT-001', 'task:INT-006'],
  ['task:INT-001', 'task:INT-007'],
  ['task:INT-006', 'task:INT-007'],
  ['task:INT-001', 'task:INT-008'],
  ['task:INT-002', 'task:INT-008'],
  ['task:INT-005', 'task:INT-008'],
  ['task:INT-006', 'task:INT-008'],
  ['task:INT-007', 'task:INT-008'],
  // BEDROCK → PRISM
  ['task:BDK-003', 'task:PRI-001'],
  ['task:BDK-002', 'task:PRI-002'],
  // PRISM internal
  ['task:PRI-001', 'task:PRI-002'],
  ['task:PRI-001', 'task:PRI-003'],
  ['task:PRI-001', 'task:PRI-004'],
  ['task:PRI-001', 'task:PRI-005'],
  ['task:PRI-001', 'task:PRI-006'],
  ['task:PRI-001', 'task:PRI-007'],
  ['task:PRI-003', 'task:PRI-007'],
  ['task:PRI-004', 'task:PRI-007'],
  ['task:PRI-005', 'task:PRI-007'],
  ['task:PRI-006', 'task:PRI-007'],
  // BEDROCK → WATCHTOWER
  ['task:BDK-003', 'task:WTC-001'],
  ['task:BDK-005', 'task:WTC-001'],
  ['task:BDK-003', 'task:WTC-003'],
  ['task:BDK-003', 'task:WTC-004'],
  // WATCHTOWER internal
  ['task:WTC-001', 'task:WTC-002'],
  ['task:WTC-001', 'task:WTC-005'],
  ['task:WTC-003', 'task:WTC-005'],
  ['task:WTC-004', 'task:WTC-005'],
  // INTAKE → PROVING GROUND
  ['task:INT-003', 'task:PRV-001'],
  ['task:INT-004', 'task:PRV-001'],
  ['task:INT-003', 'task:PRV-002'],
  // PROVING GROUND internal
  ['task:PRV-001', 'task:PRV-002'],
  ['task:PRV-002', 'task:PRV-003'],
  ['task:PRV-003', 'task:PRV-004'],
  ['task:PRV-004', 'task:PRV-005'],
  // Cross-milestone → PROVING GROUND
  ['task:BDK-005', 'task:PRV-003'],
  ['task:WTC-001', 'task:PRV-003'],
  ['task:PRI-003', 'task:PRV-003'],
  ['task:PRI-004', 'task:PRV-003'],
  ['task:PRI-005', 'task:PRV-003'],
  ['task:PRI-006', 'task:PRV-003'],
  // PROVING GROUND → ORACLE
  ['task:PRV-003', 'task:ORC-001'],
  // ORACLE internal
  ['task:ORC-001', 'task:ORC-002'],
  ['task:ORC-001', 'task:ORC-003'],
  ['task:ORC-002', 'task:ORC-003'],
  ['task:ORC-003', 'task:ORC-004'],
  ['task:ORC-003', 'task:ORC-005'],
  ['task:ORC-004', 'task:ORC-005'],
  ['task:ORC-006', 'task:ORC-007'],
  // BEDROCK → ORACLE (ORC-006 doctor)
  ['task:BDK-003', 'task:ORC-006'],
  ['task:BDK-002', 'task:ORC-006'],
  // ORACLE → NEXUS
  ['task:ORC-001', 'task:NEX-001'],
  // NEXUS internal
  ['task:NEX-001', 'task:NEX-002'],
  ['task:NEX-003', 'task:NEX-004'],
  ['task:NEX-004', 'task:NEX-005'],
  // Cross-milestone → NEXUS
  ['task:INT-001', 'task:NEX-003'],
  ['task:INT-002', 'task:NEX-003'],
  ['task:BDK-002', 'task:NEX-004'],
  ['task:INT-001', 'task:NEX-006'],
];

// ──────────────────────────────────────────────────────────────────
// BUILD
// ──────────────────────────────────────────────────────────────────

async function build() {
  console.log('Loading graph...');
  const graph = await loadGraph(CWD);

  console.log('Creating patch...');
  const patch = await graph.createPatch();

  // --- Add all nodes ---
  let nodeCount = 0;

  // Milestones
  for (const m of milestones) {
    patch.addNode(m.id);
    patch.setProperty(m.id, 'name', m.name);
    patch.setProperty(m.id, 'theme', m.theme);
    patch.setProperty(m.id, 'featureCount', m.features);
    patch.setProperty(m.id, 'taskCount', m.tasks);
    patch.setProperty(m.id, 'estHours', m.estHours);
    patch.setProperty(m.id, 'order', m.order);
    patch.setProperty(m.id, 'type', 'milestone');
    nodeCount++;
  }

  // Features
  for (const f of features) {
    patch.addNode(f.id);
    patch.setProperty(f.id, 'name', f.name);
    patch.setProperty(f.id, 'type', 'feature');
    nodeCount++;
  }

  // Tasks
  for (const t of tasks) {
    patch.addNode(t.id);
    patch.setProperty(t.id, 'name', t.name);
    patch.setProperty(t.id, 'estHours', t.estHours);
    patch.setProperty(t.id, 'type', 'task');
    patch.setProperty(t.id, 'status', 'pending');
    nodeCount++;
  }

  // Doc node
  patch.addNode(doc.id);
  patch.setProperty(doc.id, 'name', doc.name);
  patch.setProperty(doc.id, 'path', doc.path);
  patch.setProperty(doc.id, 'type', 'doc');
  nodeCount++;

  console.log(`  ${nodeCount} nodes added`);

  // --- Add all edges ---
  let edgeCount = 0;

  // Feature belongs-to milestone (27)
  for (const f of features) {
    patch.addEdge(f.id, f.milestone, 'belongs-to');
    edgeCount++;
  }

  // Task implements feature (46)
  for (const t of tasks) {
    patch.addEdge(t.id, t.feature, 'implements');
    edgeCount++;
  }

  // Milestone depends-on milestone (10)
  for (const [from, to] of milestoneDeps) {
    patch.addEdge(from, to, 'depends-on');
    edgeCount++;
  }

  // Task blocks task (77)
  for (const [from, to] of blocks) {
    patch.addEdge(from, to, 'blocks');
    edgeCount++;
  }

  // Doc documents each milestone (7)
  for (const m of milestones) {
    patch.addEdge(doc.id, m.id, 'documents');
    edgeCount++;
  }

  console.log(`  ${edgeCount} edges added`);

  // --- Commit ---
  console.log('Committing patch...');
  await patch.commit();

  // --- Verify ---
  const allNodes = await graph.getNodes();
  const allEdges = await graph.getEdges();
  console.log(`\nVerification:`);
  console.log(`  Nodes in graph: ${allNodes.length}`);
  console.log(`  Edges in graph: ${allEdges.length}`);
  console.log(`\nDAG build complete.`);
}

build().catch(err => {
  console.error('BUILD FAILED:', err);
  process.exitCode = 1;
});
