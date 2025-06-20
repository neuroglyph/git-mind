# git-mind Documentation

This directory contains user-facing documentation for git-mind.

## Quick Start

- **[QUICK_START.md](QUICK_START.md)** - Get started with git-mind in 5 minutes

## CLI Documentation

Complete command reference with examples:
- [git-mind](cli/gitmind.md) - Main command overview
- [git-mind link](cli/gitmind-link.md) - Create semantic relationships
- [git-mind list](cli/gitmind-list.md) - Query your knowledge graph
- [git-mind cache-rebuild](cli/gitmind-cache-rebuild.md) - Optimize performance
- [git-mind install-hooks](cli/gitmind-install-hooks.md) - Track file evolution

## Architecture Documentation

- [Architecture Overview](architecture/) - System design and internals
  - [Attribution System](architecture/attribution-system.md) - Human-AI collaboration 🆕
  - [Journal System](architecture/journal-architecture-pivot.md) - Git-native storage
  - [Bitmap Cache](architecture/bitmap-cache-design.md) - Roaring Bitmap performance
  - [AUGMENTS System](architecture/augments-system.md) - Automatic evolution tracking

## API Reference

- [Core API](api/gitmind-api-reference.md) - C API documentation

## Additional Resources

- [Project Roadmap](../TASKLIST.md) - Current implementation status
- [Source Modules](../src/) - Implementation details

## Getting Help

1. Check the [Quick Start Guide](QUICK_START.md)
2. Read the CLI documentation above
3. Review the [architecture docs](architecture/) for deep understanding
4. See [CLAUDE.md](../CLAUDE.md) for AI collaboration guidelines

## For Developers

- [CLAUDE.md](../CLAUDE.md) - Development instructions and standards
- [TASKLIST.md](../TASKLIST.md) - Current implementation status
- [Attribution Integration Guide](architecture/attribution-integration-guide.md) - Add attribution to components