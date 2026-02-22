#!/usr/bin/env -S node --disable-warning=DEP0169

/**
 * git-mind CLI entry point.
 * Usage: git mind <command> [options]
 */

import { init, link, view, list, remove, nodes, status, at, importCmd, importMarkdownCmd, exportCmd, mergeCmd, installHooks, processCommitCmd, doctor, suggest, review, diff, set, unsetCmd, contentSet, contentShow, contentMeta, contentDelete, extensionList, extensionValidate, extensionAdd, extensionRemove } from '../src/cli/commands.js';
import { parseDiffRefs, collectDiffPositionals } from '../src/diff.js';
import { createContext } from '../src/context-envelope.js';
import { registerBuiltinExtensions } from '../src/extension.js';

const args = process.argv.slice(2);
const command = args[0];
const cwd = process.cwd();

function printUsage() {
  console.log(`Usage: git mind <command> [options]

Context flags (read commands: view, nodes, status, export, doctor):
  --at <ref>                    Show graph as-of a git ref (HEAD~N, branch, SHA)
  --observer <name>             Filter through a named observer profile
  --trust <policy>              Trust policy (open, approved-only)

Commands:
  init                          Initialize git-mind in this repo
  link <source> <target>        Create a semantic edge
    --type <type>               Edge type (default: relates-to)
    --confidence <n>            Confidence 0.0-1.0 (default: 1.0)
    --remote <owner/name>       Qualify IDs as cross-repo
  remove <source> <target>      Remove a semantic edge
    --type <type>               Edge type (default: relates-to)
  view [name[:lens1:lens2]]      Show a view, optionally with lens chaining
    --scope <prefixes>          Comma-separated prefix filter (progress view)
    --at <ref>                  Time-travel: show view as-of a git ref
    --observer <name>           Filter through a named observer profile
    --json                      Output as JSON
    Lenses: incomplete, frontier, critical-path, blocked, parallel
  list                          List all edges
    --type <type>               Filter by edge type
    --source <node>             Filter by source node
    --target <node>             Filter by target node
  nodes                         List and inspect nodes
    --prefix <prefix>           Filter by prefix (e.g. task, spec)
    --id <nodeId>               Show details for a single node
    --at <ref>                  Time-travel: show nodes as-of a git ref
    --observer <name>           Filter through a named observer profile
    --json                      Output as JSON
  set <nodeId> <key> <value>    Set a node property
    --json                      Output as JSON
  unset <nodeId> <key>          Remove a node property
    --json                      Output as JSON
  status                        Show graph health dashboard
    --at <ref>                  Time-travel: show status as-of a git ref
    --observer <name>           Filter through a named observer profile
    --json                      Output as JSON
  at <ref>                      Show graph at a historical point in time
    --json                      Output as JSON
  diff <ref-a>..<ref-b>         Compare graph between two commits
    --json                      Output as JSON
    --prefix <prefix>           Scope to a single prefix
  import <file>                 Import a YAML graph file
    --dry-run, --validate       Validate without writing
    --json                      Output as JSON
    --from-markdown <glob>      Import from markdown frontmatter
  export [file]                 Export graph to YAML/JSON
    --format yaml|json          Output format (default: yaml)
    --prefix <prefix>           Filter by node prefix
    --at <ref>                  Time-travel: export as-of a git ref
    --observer <name>           Filter through a named observer profile
    --json                      Output as JSON metadata
  merge                          Merge another repo's graph
    --from <repo-path>          Path to remote repo
    --repo-name <owner/name>    Override detected repo identifier
    --dry-run                   Preview without writing
    --json                      Output as JSON
  install-hooks                  Install post-commit Git hook
  doctor                        Run graph integrity checks
    --fix                       Auto-fix dangling edges
    --at <ref>                  Time-travel: check graph as-of a git ref
    --observer <name>           Filter through a named observer profile
    --json                      Output as JSON
  suggest                       AI-powered edge suggestions
    --agent <command>           Override GITMIND_AGENT
    --context <sha-range>       Git range for context (default: HEAD~10..HEAD)
    --json                      Output as JSON
  review                        Review pending suggestions
    --batch accept|reject       Non-interactive batch mode
    --json                      Output as JSON
  content <subcommand>           Manage node content
    set <node> --from <file>    Attach content from a file
      --mime <type>             Override MIME type detection
      --json                    Output as JSON
    show <node>                 Display attached content
      --raw                     Output body only (no metadata header)
      --json                    Output as JSON
    meta <node>                 Show content metadata
      --json                    Output as JSON
    delete <node>               Remove attached content
      --json                    Output as JSON
  extension <subcommand>        Manage extensions
    list                        List registered extensions
      --json                    Output as JSON
    validate <manifest>         Validate a manifest file without registering
      --json                    Output as JSON
    add <manifest>              Load and register an extension
      --json                    Output as JSON
    remove <name>               Unregister an extension by name
      --json                    Output as JSON

Edge types: implements, augments, relates-to, blocks, belongs-to,
            consumed-by, depends-on, documents`);
}

const BOOLEAN_FLAGS = new Set(['json', 'fix', 'dry-run', 'validate', 'raw']);

/**
 * Extract a ContextEnvelope from parsed flags.
 * Builds one only when context flags are present; otherwise returns null.
 *
 * @param {Record<string, string|true>} flags
 * @returns {import('../src/context-envelope.js').ContextEnvelope|null}
 */
function contextFromFlags(flags) {
  if (!flags.at && !flags.observer && !flags.trust) return null;
  const overrides = {};
  if (flags.at) overrides.asOf = flags.at;
  if (flags.observer) overrides.observer = flags.observer;
  if (flags.trust) overrides.trustPolicy = flags.trust;
  return createContext(overrides);
}

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

  case 'view': {
    const viewArgs = args.slice(1);
    const viewFlags = parseFlags(viewArgs);
    // Collect positionals: skip flags and their consumed values
    const viewPositionals = [];
    for (let i = 0; i < viewArgs.length; i++) {
      if (viewArgs[i].startsWith('--')) {
        const flag = viewArgs[i].slice(2);
        if (!BOOLEAN_FLAGS.has(flag) && i + 1 < viewArgs.length) i++; // skip value
      } else {
        viewPositionals.push(viewArgs[i]);
      }
    }
    const viewCtx = contextFromFlags(viewFlags);
    await view(cwd, viewPositionals[0], {
      scope: viewFlags.scope,
      json: viewFlags.json ?? false,
      ...(viewCtx && { context: viewCtx }),
    });
    break;
  }

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
    const nodesArgs = args.slice(1);
    const nodesFlags = parseFlags(nodesArgs);
    const nodesCtx = contextFromFlags(nodesFlags);
    await nodes(cwd, {
      prefix: nodesFlags.prefix,
      id: nodesFlags.id,
      json: nodesFlags.json ?? false,
      ...(nodesCtx && { context: nodesCtx }),
    });
    break;
  }

  case 'set': {
    const setNodeId = args[1];
    const setKey = args[2];
    const setValue = args[3];
    if (!setNodeId || !setKey || setValue === undefined || setValue.startsWith('--')) {
      console.error('Usage: git mind set <nodeId> <key> <value> [--json]');
      console.error('  <value> is positional and required (flags are not valid values)');
      process.exitCode = 1;
      break;
    }
    await set(cwd, setNodeId, setKey, setValue, { json: args.includes('--json') });
    break;
  }

  case 'unset': {
    const unsetNodeId = args[1];
    const unsetKey = args[2];
    if (!unsetNodeId || !unsetKey) {
      console.error('Usage: git mind unset <nodeId> <key>');
      process.exitCode = 1;
      break;
    }
    await unsetCmd(cwd, unsetNodeId, unsetKey, { json: args.includes('--json') });
    break;
  }

  case 'status': {
    const statusFlags = parseFlags(args.slice(1));
    const statusCtx = contextFromFlags(statusFlags);
    await status(cwd, {
      json: statusFlags.json ?? false,
      ...(statusCtx && { context: statusCtx }),
    });
    break;
  }

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

  case 'diff': {
    const diffFlags = parseFlags(args.slice(1));
    const diffPositionals = collectDiffPositionals(args.slice(1));
    try {
      const { refA, refB } = parseDiffRefs(diffPositionals);
      await diff(cwd, refA, refB, {
        json: diffFlags.json ?? false,
        prefix: diffFlags.prefix,
      });
    } catch (err) {
      console.error(err.message);
      process.exitCode = 1;
    }
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
    const exportFile = args.slice(1).find(a => !a.startsWith('--'));
    const exportCtx = contextFromFlags(exportFlags);
    await exportCmd(cwd, {
      file: exportFile,
      format: exportFlags.format,
      prefix: exportFlags.prefix,
      json: exportFlags.json ?? false,
      ...(exportCtx && { context: exportCtx }),
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
    const doctorCtx = contextFromFlags(doctorFlags);
    await doctor(cwd, {
      json: doctorFlags.json ?? false,
      fix: doctorFlags.fix ?? false,
      ...(doctorCtx && { context: doctorCtx }),
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

  case 'content': {
    const contentSubCmd = args[1];
    const contentFlags = parseFlags(args.slice(2));
    switch (contentSubCmd) {
      case 'set': {
        const setNode = args.slice(2).find(a => !a.startsWith('--'));
        const fromFile = contentFlags.from;
        if (!setNode || !fromFile) {
          console.error('Usage: git mind content set <node> --from <file> [--mime <type>] [--json]');
          process.exitCode = 1;
          break;
        }
        await contentSet(cwd, setNode, fromFile, {
          mime: contentFlags.mime,
          json: contentFlags.json ?? false,
        });
        break;
      }
      case 'show': {
        const showNode = args.slice(2).find(a => !a.startsWith('--'));
        if (!showNode) {
          console.error('Usage: git mind content show <node> [--raw] [--json]');
          process.exitCode = 1;
          break;
        }
        await contentShow(cwd, showNode, {
          raw: contentFlags.raw ?? false,
          json: contentFlags.json ?? false,
        });
        break;
      }
      case 'meta': {
        const metaNode = args.slice(2).find(a => !a.startsWith('--'));
        if (!metaNode) {
          console.error('Usage: git mind content meta <node> [--json]');
          process.exitCode = 1;
          break;
        }
        await contentMeta(cwd, metaNode, { json: contentFlags.json ?? false });
        break;
      }
      case 'delete': {
        const deleteNode = args.slice(2).find(a => !a.startsWith('--'));
        if (!deleteNode) {
          console.error('Usage: git mind content delete <node> [--json]');
          process.exitCode = 1;
          break;
        }
        await contentDelete(cwd, deleteNode, { json: contentFlags.json ?? false });
        break;
      }
      default:
        console.error(`Unknown content subcommand: ${contentSubCmd ?? '(none)'}`);
        console.error('Usage: git mind content <set|show|meta|delete>');
        process.exitCode = 1;
    }
    break;
  }

  case 'extension': {
    await registerBuiltinExtensions();
    const subCmd = args[1];
    const extFlags = parseFlags(args.slice(2));
    switch (subCmd) {
      case 'list':
        extensionList(cwd, { json: extFlags.json ?? false });
        break;
      case 'validate': {
        const validatePath = args.slice(2).find(a => !a.startsWith('--'));
        await extensionValidate(cwd, validatePath, { json: extFlags.json ?? false });
        break;
      }
      case 'add': {
        const addPath = args.slice(2).find(a => !a.startsWith('--'));
        await extensionAdd(cwd, addPath, { json: extFlags.json ?? false });
        break;
      }
      case 'remove': {
        const removeName = args.slice(2).find(a => !a.startsWith('--'));
        extensionRemove(cwd, removeName, { json: extFlags.json ?? false });
        break;
      }
      default:
        console.error(`Unknown extension subcommand: ${subCmd ?? '(none)'}`);
        console.error('Usage: git mind extension <list|validate|add|remove>');
        process.exitCode = 1;
    }
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
