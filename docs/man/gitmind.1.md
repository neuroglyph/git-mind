# gitmind(1) - Git-powered semantic knowledge graph

## SYNOPSIS

**gitmind** *command* [*options*] [*arguments*]

## DESCRIPTION

GitMind turns Git into a semantic knowledge graph by tracking relationships between files. It stores links as Git objects, making your knowledge version-controlled, distributed, and merge-friendly.

## COMMANDS

### gitmind init
Initialize GitMind in the current Git repository. Creates `.gitmind/links/` directory for storing semantic links.

### gitmind link *source* *target* [--type *TYPE*]
Create a semantic link between two files.

**Options:**
- `--type TYPE` - Link type (default: REFERENCES)

**Link Types:**
- IMPLEMENTS - Source implements concepts from target
- REFERENCES - Source references target
- INSPIRED_BY - Source was inspired by target  
- DEPENDS_ON - Source depends on target
- CONTRADICTS - Source contradicts target
- DISCUSSES - Source discusses target
- REVIEWS - Source reviews target

### gitmind list [--source *FILE*] [--target *FILE*]
List all semantic links, optionally filtered by source or target.

**Options:**
- `--source FILE` - Show only links from FILE
- `--target FILE` - Show only links to FILE

### gitmind unlink *source* *target*
Remove the link between source and target files.

### gitmind check [--fix]
Check integrity of all links, finding broken references.

**Options:**
- `--fix` - Remove broken links automatically

### gitmind status
Show repository statistics including link count and graph density.

### gitmind version
Display version information.

## EXAMPLES

Initialize GitMind in a repository:
```
$ gitmind init
Initialized gitmind in current repository
```

Create a link indicating design.md implements the API specification:
```
$ gitmind link design.md specs/api.md --type IMPLEMENTS
Created link: design.md -> specs/api.md (IMPLEMENTS)
```

Find all files that reference the README:
```
$ gitmind list --target README.md
REFERENCES: docs/guide.md -> README.md
INSPIRED_BY: examples/demo.md -> README.md
```

Check and fix broken links:
```
$ gitmind check --fix
Found 2 broken links
Removed: old.md -> deleted.md
Removed: temp.md -> missing.md
```

## FILES

**.gitmind/links/**
Directory containing link files. Each link is stored as a single line in a file named by the SHA-1 of its content.

## ENVIRONMENT

**GITMIND_DEBUG**
Enable debug output when set to 1.

## EXIT STATUS

- 0 - Success
- 1 - General error
- 2 - Not in a Git repository
- 3 - File not found
- 4 - I/O error
- 5 - Git operation failed

## SEE ALSO

git(1), ln(1)

## BUGS

Report bugs at: https://github.com/neuroglyph/neuroglyph/issues

## AUTHOR

J. Kirby Ross and the Neuroglyph Collective

## COPYRIGHT

Copyright © 2025 J. Kirby Ross / Neuroglyph Collective. Licensed under Apache 2.0.