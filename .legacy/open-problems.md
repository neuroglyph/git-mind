# I. Merge Conflicts

Yes. This is the git-mind equivalent of “merge hell.” We need a clear strategy: automatic merging when links don’t conflict, manual resolution UI/UX when they do...

# II. File Renames/Moves

- Currently a soft spot. Git tracks file moves heuristically, so you either:
  - Use path hashes and rebind on git mv
  - Or log file identities by content hash + path pairs

See [file-renames-and-moves.md](docs/file-renames-and-moves.md) for more ideas.

# 3. Multi-Dimensional Evolution

Hell yes. Confidence decay over time? AI-inferred pattern recognition? This is advanced territory, but even a simple ASCII timeline could open the door.

# 4. Git Notes Scalability

Notes are neat for now, but could turn into a bottleneck if each link gets dozens of revisions/comments.

Long-term: a better storage backend, still Git-native.
