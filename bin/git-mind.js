#!/usr/bin/env node

/**
 * git-mind CLI entry point.
 * Usage: git mind <command> [options]
 */

import { init, link, view, list, suggest, review } from '../src/cli/commands.js';

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
  view [name]                   Show a named view (or list views)
  list                          List all edges
  suggest --ai                  AI suggestions (stub)
  review                        Review edges (stub)

Edge types: implements, augments, relates-to, blocks, belongs-to,
            consumed-by, depends-on, documents`);
}

/**
 * Parse --flag value pairs from args.
 * @param {string[]} args
 * @returns {Record<string, string>}
 */
function parseFlags(args) {
  const flags = {};
  for (let i = 0; i < args.length; i++) {
    if (args[i].startsWith('--') && i + 1 < args.length) {
      flags[args[i].slice(2)] = args[i + 1];
      i++;
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
    });
    break;
  }

  case 'view':
    await view(cwd, args[1]);
    break;

  case 'list':
    await list(cwd);
    break;

  case 'suggest':
    await suggest();
    break;

  case 'review':
    await review();
    break;

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
