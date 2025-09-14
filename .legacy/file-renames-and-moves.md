# File Renames or Moves

Git does not have a dedicated hook for `git mv` or file renames specifically.

However, here’s what you need to know:

## Why There’s No post-mv Hook

Git treats git mv as a convenience wrapper around:

```bash
mv oldname newname
git rm oldname
git add newname
```

Git itself doesn’t track renames directly in the index—it infers them later using similarity detection during diff/merge operations.

## Workarounds

To react to file renames, you can:

1. Use pre-commit or post-commit hooks, then detect renames using:

```bash
git diff --cached --name-status -M
```

The `-M` flag tells Git to detect renames (based on content similarity). Look for lines starting with R100, R90, etc.

2. In CI or custom scripts, do:

```bash
git log --diff-filter=R --summary
```

3. Track file paths via custom mapping

If you’re building a tool like `git-mind`, consider maintaining your own `.gitmind/filemap` that maps canonical logical names to paths, so renames become path updates rather than semantic breaks.

## For git-mind, I’d suggest

- Add a `git mind rename <old> <new>` command that updates internal references
- Use post-commit hook with rename detection if needed
- Or just add a `git mind check --fix` that spots and helps patch broken links due to renames
