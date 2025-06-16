# gitmind-init(1) - Initialize GitMind in a repository

## SYNOPSIS

**gitmind init**

## DESCRIPTION

The **gitmind init** command initializes GitMind in the current Git repository by creating the necessary directory structure for storing semantic links.

This command creates a `.gitmind/links/` directory in the repository root. This directory is tracked by Git, allowing links to be version-controlled and shared with collaborators.

## BEHAVIOR

1. Checks if the current directory is inside a Git repository
2. Creates `.gitmind/` directory if it doesn't exist
3. Creates `.gitmind/links/` subdirectory for storing links
4. Ensures `.gitmind/` is NOT in .gitignore (links should be tracked)
5. Creates an initial `.gitmind/links/.gitkeep` file if the directory is empty

## EXAMPLES

Initialize GitMind in the current repository:
```
$ cd my-project
$ gitmind init
Initialized gitmind in current repository
```

## ERRORS

The command will fail if:
- The current directory is not inside a Git repository
- File system permissions prevent creating directories
- The `.gitmind/` directory already exists (not an error, just skips creation)

## FILES

**.gitmind/links/**
Directory created to store semantic link files.

## SEE ALSO

gitmind(1), git-init(1)

## COPYRIGHT

Copyright © 2025 J. Kirby Ross / Neuroglyph Collective. Licensed under Apache 2.0.