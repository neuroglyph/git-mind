# Link Command

## Purpose

Create semantic links between files in the current Git repository.

## Design Rationale

### Command Syntax

```bash
git-mind link <source> <target> [--type <type>]
```

Why this order?

1. __Natural reading__: "A links to B"
2. __Consistent with cp/mv__: source first
3. __Type is optional__: defaults to "references"
4. __Extensible__: Can add --confidence, --reason later

### Relationship Types

```
implements  - Code implements design/spec
references  - Documentation cross-reference
depends_on  - Build/runtime dependency
augments    - Updates/extends previous
```

Limited set because:

- Each type has semantic meaning
- Tools can interpret differently
- Start small, extend carefully

### Type Parsing

```c
strcasecmp(str, "depends_on") == 0 || 
strcasecmp(str, "depends-on") == 0
```

- Accept both underscore and hyphen
- Case-insensitive for usability
- Common variations handled

## User Experience

### Success Case

```bash
$ git-mind link README.md docs/install.md --type references
Created link: REFERENCES: README.md -> docs/install.md
```

Clear feedback:

- Confirms action taken
- Shows interpreted type
- Uses arrow notation

### Error Cases

```bash
$ git-mind link missing.md README.md
Error: Not found

$ git-mind link 
Usage: git-mind link <source> <target> [--type <type>]
Types: implements, references, depends_on, augments
```

Helpful errors:

- Brief error message
- Usage hint when args missing
- List valid types

## Implementation Flow

```mermaid
sequenceDiagram
    participant CLI
    participant Edge
    participant Journal
    participant Git
    
    CLI->>CLI: Parse arguments
    CLI->>Edge: Create edge(src, tgt, type)
    Edge->>Git: Resolve paths to SHAs
    Git-->>Edge: SHA values
    Edge-->>CLI: Complete edge struct
    CLI->>Journal: Append edge
    Journal->>Git: Create commit
    Git-->>Journal: Success
    CLI->>CLI: Print confirmation
```

## Edge Cases

### Circular Links

```bash
git-mind link A.md B.md
git-mind link B.md A.md  # Allowed!
```

- No cycle detection
- Both directions may be valid
- Graph algorithms handle cycles

### Self-Links

```bash
$ git-mind link README.md README.md
Created link: REFERENCES: README.md -> README.md
```

- Allowed (might reference earlier section)
- Unusual but not invalid
- Let user decide

### Duplicate Links

```bash
git-mind link A.md B.md --type implements
git-mind link A.md B.md --type implements  # Duplicate!
```

- Currently: Creates another edge
- Same content, different ULID
- Future: Dedupe option?

### Long Paths

```bash
$ git-mind link very/very/very/.../long/path.md other.md
Created link: REFERENCES: very/very/very/.../lo... -> other.md
```

- Paths truncated in display
- Full paths stored internally
- User-friendly output

## Argument Parsing

### Current Approach

```c
for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
        type_str = argv[++i];
    } else if (!src_path) {
        src_path = argv[i];
    } else if (!tgt_path) {
        tgt_path = argv[i];
    }
}
```

Simple but effective:

- No getopt dependency
- Position-independent flags
- First two non-flags are source/target

### Future: Rich Options

```bash
git-mind link README.md API.md \
    --type implements \
    --confidence 0.8 \
    --reason "Implements v2 API design" \
    --author "alice@example.com"
```

Extensibility built in.

## Performance

### Single Link

```
Parse args: ~1μs
Create edge: ~25μs
Journal append: ~200μs
Print result: ~10μs
Total: ~236μs
```

Dominated by Git commit creation.

### Bulk Operations

Future enhancement:

```bash
git-mind link --batch < links.txt
# Format: source target type
```

Batch multiple edges per commit.

## Error Handling

### File Not Found

```c
result = gm_edge_create(ctx, src_path, tgt_path, rel_type, &edge);
if (result != GM_OK) {
    fprintf(stderr, "Error: %s\n", gm_error_string(result));
    return result;
}
```

- Clear error propagation
- Human-readable messages
- Return code for scripts

### Journal Write Failure

```
Error: Failed to write link
```

- Generic message (details in stderr)
- Could be permissions, disk full, etc.
- Git errors are complex

## Security Considerations

### Command Injection

```bash
git-mind link "file;rm -rf /" other.md
```

- No shell execution
- Paths passed directly to Git
- Safe from injection

### Path Validation

- Git validates paths
- No additional sanitization
- Trust Git's rules

## Testing Strategy

### Unit Tests

1. Valid link creation
2. Missing source file
3. Missing target file
4. Invalid type string
5. No arguments provided

### Integration Tests

```bash
# Setup
git init test-repo
cd test-repo
echo "test" > A.md
echo "test" > B.md
git add .
git commit -m "test"

# Test
git-mind link A.md B.md --type implements

# Verify
git-mind list | grep "IMPLEMENTS: A.md -> B.md"
```

### User Acceptance Tests

```gherkin
Given a repository with files README.md and API.md
When I run "git-mind link README.md API.md --type references"
Then I should see "Created link: REFERENCES: README.md -> API.md"
And the link should be persisted in the journal
```

## Future Enhancements

### Interactive Mode

```bash
$ git-mind link -i
Source file: README.md
Target file: (fuzzy finder)
Relationship: (menu)
Confidence [1.0]: 
Reason (optional): 
Create link? [Y/n]:
```

### Bulk Import

```bash
$ git-mind link --import links.json
Imported 1,337 links
```

### Smart Suggestions

```bash
$ git-mind link README.md
Suggested targets:
  1. docs/install.md (similar content)
  2. CONTRIBUTING.md (often linked)
  3. examples/basic.md (references README)
Select target [1-3]:
```

## Why This Design Rocks

1. __Simple__: Minimal arguments, clear output
2. __Flexible__: Extensible without breaking changes
3. __Fast__: Near-instant for single links
4. __Safe__: No side effects beyond journal
5. __Scriptable__: Exit codes and parseable output

As Linus would say: "Make the common case fast and simple."
