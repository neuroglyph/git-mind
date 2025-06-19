# git-mind Quick Start Guide

Get up and running with git-mind in under 5 minutes!

## Installation

### Option 1: Download Pre-built Binary (Fastest)

```bash
# Download the latest release
curl -LO https://github.com/neuroglyph/git-mind/releases/latest/download/git-mind-$(uname -s)-$(uname -m)

# Make it executable
chmod +x git-mind-*

# Move to your PATH
sudo mv git-mind-* /usr/local/bin/git-mind

# Verify installation
git mind version
```

### Option 2: Build from Source

Prerequisites: Docker, Make

```bash
# Clone the repository
git clone https://github.com/neuroglyph/git-mind.git
cd git-mind

# Build with Docker (ensures consistency)
make build

# Binary will be at build/git-mind
sudo cp build/git-mind /usr/local/bin/

# Verify installation
git mind version
```

## Basic Usage

### 1. Initialize git-mind in Your Repository

```bash
cd your-git-repo
git mind init
```

This creates the journal system to store your semantic links as Git commits.

### 2. Create Your First Links

```bash
# Link your README to documentation
git mind link README.md docs/API.md --type documents

# Link implementation to its spec
git mind link src/parser.c docs/parser-spec.md --type implements

# Link related files
git mind link tests/auth.test.js src/auth.js --type tests
```

### 3. Explore Your Knowledge Graph

```bash
# List all links
git mind list

# See what README.md links to
git mind list --from README.md

# Show only human-created edges
git mind list --source human

# Rebuild cache for performance
git mind cache-rebuild
```

### 4. Maintain Link Health

```bash
# Check for broken links (after deleting files)
git mind check

# Automatically remove broken links
git mind check --fix

# Install hooks for automatic evolution tracking
git mind install-hooks
```

### 5. Remove Links

```bash
# Remove a specific link
git mind unlink README.md docs/old-api.md
```

## Real-World Example

Here's a practical example for a typical project:

```bash
# Initialize (not needed - auto-creates on first link)
cd my-project

# Document your architecture
git mind link README.md docs/architecture.md --type references
git mind link docs/architecture.md src/core/engine.js --type documents
git mind link src/core/engine.js tests/engine.test.js --type tested_by

# Track dependencies
git mind link package.json README.md --type referenced_by
git mind link src/api/server.js package.json --type depends_on

# View the connections
git mind list
# Output:
# REFERENCES: README.md -> docs/architecture.md
# DOCUMENTS: docs/architecture.md -> src/core/engine.js
# TESTED_BY: src/core/engine.js -> tests/engine.test.js
# REFERENCED_BY: package.json -> README.md
# DEPENDS_ON: src/api/server.js -> package.json

# See all connections from architecture doc
git mind list --from docs/architecture.md
# Output:
# DOCUMENTS: docs/architecture.md -> src/core/engine.js
```

## Common Link Types

While you can use any type, here are some common patterns:

- `implements` - Code implements a specification
- `documents` - Documentation describes code
- `references` - General reference between files
- `depends_on` - File depends on another
- `tests` - Test file tests implementation
- `includes` - File includes/imports another
- `related_to` - General relationship
- `augments` - File evolved from another (automatic)

## Git Integration

Links are stored as Git commits, so they work seamlessly with Git:

```bash
# Links are automatically committed to a special ref
# Just push the edge refs along with your code
git push origin refs/gitmind/edges/*

# Your teammates pull and immediately see the relationships!
git pull origin refs/gitmind/edges/*
```

## Performance

git-mind is blazing fast:
- **Startup**: <1ms
- **Create link**: ~5ms
- **List 100K links**: <10ms (with Roaring Bitmap cache)
- **Binary size**: ~2MB (includes libgit2)

## Next Steps

1. **Human-AI Collaboration**
   ```bash
   # AI can create edges with attribution
   export GIT_MIND_SOURCE=claude
   git mind link src/auth.c config/oauth.json --type depends_on --confidence 0.85
   
   # Review AI suggestions
   git mind review --pending
   ```

2. **Coming Soon**: Web visualization
   ```bash
   # This will show an interactive 3D graph
   git mind explore
   # Opens browser with time-travel visualization
   ```

3. **Coming Soon**: MCP Integration
   ```bash
   # Claude will have persistent memory of your codebase
   # Real-time collaboration between human and AI understanding
   ```

## Tips

1. **Start Small**: Begin with a few key relationships
2. **Be Consistent**: Use consistent link types across your team
3. **Auto Evolution**: Use `git mind install-hooks` to track file changes
4. **Performance**: Run `git mind cache-rebuild` for large repos
5. **AI Collaboration**: Set attribution when AI creates edges
6. **Document Important Connections**: Focus on non-obvious relationships

## Troubleshooting

**"git-mind: command not found"**
- Make sure git-mind is in your PATH
- Try using the full path: `./git-mind`
- Or use the git alias: `git mind`

**"Not a git repository"**
- git-mind requires a Git repository
- Run `git init` first

**"Permission denied"**
- Make sure the binary is executable: `chmod +x git-mind`
- Use `sudo` when copying to `/usr/local/bin`

## Get Help

- GitHub Issues: https://github.com/neuroglyph/git-mind/issues
- Documentation: https://github.com/neuroglyph/git-mind/tree/main/docs
- Architecture Docs: https://github.com/neuroglyph/git-mind/tree/main/docs/architecture

---

That's it! You're now tracking semantic relationships in your codebase with git-mind. 🚀

**Next: Enable human-AI collaboration to build understanding together!**