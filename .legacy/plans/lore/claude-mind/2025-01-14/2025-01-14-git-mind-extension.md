# Git Extension Architecture: From gitmind to git-mind

## The Power of a Hyphen

Today's refactoring from `gitmind` to `git-mind` isn't just a naming change - it's an architectural shift that leverages Git's brilliant extension mechanism. By simply naming our binary `git-mind`, we transform from a standalone tool to a first-class Git citizen.

## How Git Extensions Work

Git has a beautifully simple extension mechanism:
1. Name your executable `git-{command}`
2. Put it in your PATH
3. Run `git {command}`

That's it. Git will find and execute `git-{command}`, passing along all arguments. No registration, no configuration, no API to implement. Pure Unix philosophy.

## Benefits of Being a Git Extension

### 1. **Namespace Integration**
```bash
# Before: Standalone tool
gitmind init
gitmind link file1.txt file2.txt

# After: Git extension
git mind init
git mind link file1.txt file2.txt
```

### 2. **Muscle Memory**
Developers already type `git` thousands of times. Adding commands to that namespace feels natural:
- `git commit` - track code changes
- `git mind link` - track knowledge connections

### 3. **Discovery**
```bash
git help mind           # Works automatically
git mind --help         # Also works
man git-mind            # If we add a manpage
```

### 4. **Shell Completion**
Git's tab completion can be extended to include our commands. Users get autocomplete for free.

### 5. **Ecosystem Integration**
Tools that wrap Git (GUIs, IDEs, scripts) can discover and integrate with our extension naturally.

## Implementation Details

The change required updating:
- Makefile target name
- All test scripts
- Documentation and READMEs
- Usage strings and help text
- Error messages

But critically, we kept `.gitmind/` as the data directory - only the executable changed names. This maintains backward compatibility with existing repositories.

## Future Possibilities

Being a Git extension opens doors:

1. **Git Hooks Integration**
   ```bash
   # .git/hooks/post-commit
   git mind auto-link --from-diff
   ```

2. **Alias Potential**
   ```bash
   git config --global alias.m mind
   git m link README.md DESIGN.md
   ```

3. **Porcelain Commands**
   ```bash
   git mind log --graph    # Visualize knowledge graph
   git mind blame FILE     # Who linked what when
   ```

## The Unix Way

This change embodies Unix philosophy:
- Do one thing well (knowledge linking)
- Compose with other tools (Git)
- Text streams as universal interface
- Simple interfaces that compose

By becoming a Git extension, we're not just following conventions - we're embracing an ecosystem. We're saying "git-mind belongs here, alongside commit, branch, and merge, as a fundamental operation on your repository's knowledge."

## Lessons Learned

1. **Names Matter**: The difference between `gitmind` and `git-mind` is architectural, not cosmetic.

2. **Leverage Ecosystems**: Why build our own CLI framework when Git provides one?

3. **Convention Over Configuration**: Git's extension mechanism requires zero configuration.

4. **Backwards Compatibility**: Keep data formats stable even when interfaces change.

The hyphen isn't just punctuation - it's a bridge between our tool and the entire Git ecosystem. Sometimes the smallest changes have the biggest impact.