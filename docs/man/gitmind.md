# gitmind

Git as a substrate for distributed semantic memory.

## Usage

```
gitmind <COMMAND> [OPTIONS]
```

## Commands

- `init` - Initialize GitMind in the current repository
- `link` - Create a semantic link between two files
- `list` - List all links in the repository
- `unlink` - Remove links between files
- `check` - Check for and optionally fix broken links
- `help` - Display help for a command

## Global Options

- `-h, --help` - Print help information
- `-V, --version` - Print version information

## Description

GitMind transforms any Git repository into a semantic knowledge graph by tracking 
relationships between files. Links are stored as regular Git objects, making them 
versionable, distributed, and conflict-free.

## Examples

Initialize GitMind in your repository:
```bash
gitmind init
```

Create a link between two files:
```bash
gitmind link README.md docs/architecture.md --type EXPLAINS
```

List all links from a specific file:
```bash
gitmind list --source README.md
```

Check for broken links:
```bash
gitmind check --fix
```

## Environment Variables

- `NO_COLOR` - Disable colored output when set to any non-empty value

## Files

- `.gitmind/` - GitMind metadata directory
- `.gitmind/links/` - Link storage directory

## See Also

- `gitmind help <command>` - Get help for a specific command
- Project documentation: https://neuroglyph.dev
- Source code: https://github.com/neuroglyph/neuroglyph

## Copyright

© 2025 J. Kirby Ross / Neuroglyph Collective
Licensed under Apache-2.0