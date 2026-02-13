#!/usr/bin/env node

/**
 * git-mind CLI entry point.
 * Usage: git mind <command> [options]
 */

import { init, link, view, list, remove, nodes, status, at, importCmd, importMarkdownCmd, exportCmd, mergeCmd, installHooks, processCommitCmd, doctor, suggest, review } from '../src/cli/commands.js';

const args = process.argv.slice(2);
const command = args[0];
const cwd = process.cwd();

function printUsage() {
  console.log(`Usage: git mind <command> [options]

Commands:
  init                          Initialize git-mind in this repo
  link <source> <target>        Create a semantic edge
    --type <type>               Edge type (default: relates-to)
    --confidence <n>            Confidence 0.0-1.0 (default: 1.0)
    --remote <owner/name>       Qualify IDs as cross-repo
  remove <source> <target>      Remove a semantic edge
    --type <type>               Edge type (default: relates-to)
  view [name]                   Show a named view (or list views)
  list                          List all edges
    --type <type>               Filter by edge type
    --source <node>             Filter by source node
    --target <node>             Filter by target node
  nodes                         List and inspect nodes
    --prefix <prefix>           Filter by prefix (e.g. task, spec)
    --id <nodeId>               Show details for a single node
    --json                      Output as JSON
  status                        Show graph health dashboard
    --json                      Output as JSON
  at <ref>                      Show graph at a historical point in time
    --json                      Output as JSON
  import <file>                 Import a YAML graph file
    --dry-run, --validate       Validate without writing
    --json                      Output as JSON
    --from-markdown <glob>      Import from markdown frontmatter
  export [file]                 Export graph to YAML/JSON
    --format yaml|json          Output format (default: yaml)
    --prefix <prefix>           Filter by node prefix
    --json                      Output as JSON metadata
  merge                          Merge another repo's graph
    --from <repo-path>          Path to remote repo
    --repo-name <owner/name>    Override detected repo identifier
    --dry-run                   Preview without writing
    --json                      Output as JSON
  install-hooks                  Install post-commit Git hook
  doctor                        Run graph integrity checks
    --fix                       Auto-fix dangling edges
    --json                      Output as JSON
  suggest                       AI-powered edge suggestions
    --agent <command>           Override GITMIND_AGENT
    --context <sha-range>       Git range for context (default: HEAD~10..HEAD)
    --json                      Output as JSON
  review                        Review pending suggestions
    --batch accept|reject       Non-interactive batch mode
    --json                      Output as JSON

Edge types: implements, augments, relates-to, blocks, belongs-to,
            consumed-by, depends-on, documents`);
}

const BOOLEAN_FLAGS = new Set(['json', 'fix', 'dry-run', 'validate']);

/**
 * Parse --flag value pairs from args.
 * Boolean flags (--json, --fix) are set to true; others consume the next arg.
 * @param {string[]} args
 * @returns {Record<string, string|true>}
 */
function parseFlags(args) {
  const flags = {};
  for (let i = 0; i < args.length; i++) {
    if (args[i].startsWith('--')) {
      const name = args[i].slice(2);
      if (BOOLEAN_FLAGS.has(name)) {
        flags[name] = true;
      } else if (i + 1 < args.length) {
        flags[name] = args[i + 1];
        i++;
      }
    }
  }
  return flags;
}

switch (command) {
  case 'init':
    await init(cwd);
    break;

  case 'link': {
    const source = args[1];
    const target = args[2];
    if (!source || !target) {
      console.error('Usage: git mind link <source> <target> [--type <type>]');
      process.exitCode = 1;
      break;
    }
    const flags = parseFlags(args.slice(3));
    await link(cwd, source, target, {
      type: flags.type,
      confidence: flags.confidence ? parseFloat(flags.confidence) : undefined,
      remote: flags.remote,
    });
    break;
  }

  case 'view':
    await view(cwd, args[1]);
    break;

  case 'remove': {
    const rmSource = args[1];
    const rmTarget = args[2];
    if (!rmSource || !rmTarget) {
      console.error('Usage: git mind remove <source> <target> [--type <type>]');
      process.exitCode = 1;
      break;
    }
    const rmFlags = parseFlags(args.slice(3));
    await remove(cwd, rmSource, rmTarget, { type: rmFlags.type });
    break;
  }

  case 'list': {
    const listFlags = parseFlags(args.slice(1));
    await list(cwd, {
      type: listFlags.type,
      source: listFlags.source,
      target: listFlags.target,
    });
    break;
  }

  case 'nodes': {
    const jsonMode = args.includes('--json');
    const nodesArgs = args.slice(1).filter(a => a !== '--json');
    const nodesFlags = parseFlags(nodesArgs);
    await nodes(cwd, {
      prefix: nodesFlags.prefix,
      id: nodesFlags.id,
      json: jsonMode,
    });
    break;
  }

  case 'status':
    await status(cwd, { json: args.includes('--json') });
    break;

  case 'at': {
    const atRef = args[1];
    if (!atRef || atRef.startsWith('--')) {
      console.error('Usage: git mind at <ref>');
      process.exitCode = 1;
      break;
    }
    await at(cwd, atRef, { json: args.includes('--json') });
    break;
  }

  case 'import': {
    const importFlags = parseFlags(args.slice(1));
    const dryRun = importFlags['dry-run'] === true || importFlags['validate'] === true;
    const jsonMode = importFlags.json === true;

    if (importFlags['from-markdown']) {
      await importMarkdownCmd(cwd, importFlags['from-markdown'], { dryRun, json: jsonMode });
      break;
    }

    const importPath = args.slice(1).find(a => !a.startsWith('--'));
    if (!importPath) {
      console.error('Usage: git mind import <file> [--dry-run] [--json] [--from-markdown <glob>]');
      process.exitCode = 1;
      break;
    }
    await importCmd(cwd, importPath, { dryRun, json: jsonMode });
    break;
  }

  case 'export': {
    const exportFlags = parseFlags(args.slice(1));
    const exportFile = args[1] && !args[1].startsWith('--') ? args[1] : undefined;
    await exportCmd(cwd, {
      file: exportFile,
      format: exportFlags.format,
      prefix: exportFlags.prefix,
      json: exportFlags.json ?? false,
    });
    break;
  }

  case 'merge': {
    const mergeFlags = parseFlags(args.slice(1));
    await mergeCmd(cwd, {
      from: mergeFlags.from,
      repoName: mergeFlags['repo-name'],
      dryRun: mergeFlags['dry-run'] === true,
      json: mergeFlags.json === true,
    });
    break;
  }

  case 'install-hooks':
    await installHooks(cwd);
    break;

  case 'process-commit':
    if (!args[1]) {
      console.error('Usage: git mind process-commit <sha>');
      process.exitCode = 1;
      break;
    }
    await processCommitCmd(cwd, args[1]);
    break;

  case 'doctor': {
    const doctorFlags = parseFlags(args.slice(1));
    await doctor(cwd, {
      json: doctorFlags.json ?? false,
      fix: doctorFlags.fix ?? false,
    });
    break;
  }

  case 'suggest': {
    const suggestFlags = parseFlags(args.slice(1));
    await suggest(cwd, {
      agent: suggestFlags.agent,
      context: suggestFlags.context,
      json: suggestFlags.json ?? false,
    });
    break;
  }

  case 'review': {
    const reviewFlags = parseFlags(args.slice(1));
    await review(cwd, {
      batch: reviewFlags.batch,
      index: reviewFlags.index ? parseInt(reviewFlags.index, 10) : undefined,
      json: reviewFlags.json ?? false,
    });
    break;
  }

  case '--help':
  case '-h':
  case 'help':
    printUsage();
    break;

  default:
    if (command) {
      console.error(`Unknown command: ${command}\n`);
    }
    printUsage();
    process.exitCode = command ? 1 : 0;
    break;
}
