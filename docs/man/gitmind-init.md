# gitmind init

Initialize GitMind in the current Git repository.

## Usage

```
gitmind init
```

## Description

The `init` command sets up GitMind in your current Git repository by creating 
the `.gitmind/links/` directory structure. This directory will store all semantic 
links between files in your repository.

GitMind requires an existing Git repository. If you haven't initialized Git yet,
run `git init` first.

## Options

This command has no additional options.

## Examples

Initialize GitMind in your repository:
```bash
cd my-project
gitmind init
```

Full workflow for a new project:
```bash
mkdir my-knowledge-base
cd my-knowledge-base
git init
gitmind init
echo "# My Knowledge Base" > README.md
git add .
git commit -m "Initial commit"
```

## Errors

The command will fail if:
- You're not in a Git repository (run `git init` first)
- GitMind is already initialized (`.gitmind/` already exists)
- You don't have write permissions in the current directory

## Implementation Details

The init command:
1. Verifies you're in a Git repository
2. Creates `.gitmind/links/` directory
3. Adds a `.gitkeep` file to ensure the directory is tracked
4. Stages the `.gitkeep` file for commit

Note: The `.gitmind/` directory is tracked by Git, not ignored. This ensures
your semantic links are versioned and distributed with your repository.

## See Also

- `gitmind link` - Create your first semantic link
- `gitmind list` - View existing links
- `git init` - Initialize a Git repository