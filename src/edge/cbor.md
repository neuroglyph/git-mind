# CBOR Edge Encoder/Decoder

## Purpose
Serialize and deserialize edge structures to/from CBOR (Concise Binary Object Representation) format for storage in Git commit messages.

## Design Rationale

### Why CBOR?
1. **Binary efficiency**: ~100-300 bytes per edge vs 500+ for JSON
2. **Self-describing**: Can decode without schema
3. **Streaming**: Can decode partial data
4. **Standard**: RFC 8949, widely supported
5. **Git-friendly**: Binary commit messages supported via `encoding=binary`

### Why Not Protocol Buffers/MessagePack/etc?
- **Protobuf**: Requires schema files, versioning complexity
- **MessagePack**: Less standard, fewer tools
- **Raw binary**: No forward compatibility, brittle
- **JSON**: 3-5x larger, escaping issues in commit messages

## Implementation Details

### CBOR Structure
```
Array[7] {
    [0] bytes(20)  - Source SHA-1
    [1] bytes(20)  - Target SHA-1  
    [2] uint       - Relationship type (1-65535)
    [3] uint       - Confidence (IEEE-754 half as uint16)
    [4] uint       - Timestamp (Unix millis)
    [5] text       - Source path (for humans)
    [6] text       - Target path (for humans)
}
```

### Encoding Strategy
- Use definite-length encoding (not indefinite)
- Optimize for small numbers (< 24 = 1 byte)
- Text strings use UTF-8
- No CBOR tags (keep it simple)

## Edge Cases

### Size Limits
- **Max edge size**: ~512 bytes (paths < 256 chars each)
- **Max commit message**: 8KB (can hold ~16 edges)
- **Batch overflow**: Writer creates new commit when approaching limit

### Invalid Data
- **Truncated CBOR**: Decoder returns GM_INVALID_ARG
- **Wrong types**: Strict type checking, no coercion
- **Buffer overflow**: All bounds checked before memcpy

### Unicode Paths
- Paths stored as UTF-8 text
- Git already handles Unicode paths
- No normalization (preserve exact bytes)

## Security Considerations

1. **Buffer overflows**: Every read checks bounds
2. **Integer overflow**: 64-bit arithmetic throughout
3. **Malformed input**: Fails safely with GM_INVALID_ARG
4. **DoS**: Max recursion depth = 1 (flat array only)

## Performance

- **Encode**: ~1-2 μs per edge
- **Decode**: ~2-3 μs per edge  
- **Memory**: Zero allocations (stack buffers)
- **Cache-friendly**: Sequential access pattern

## Future Considerations

### SHA-256 Support
When Git transitions to SHA-256:
- Detect hash size from repository
- Encode as bytes(32) instead of bytes(20)
- Version flag in first commit?

### Compression
If edge volume becomes massive:
- ZSTD compress commit messages
- Git already compresses objects
- Probably unnecessary complexity

## Testing Notes

Key test cases:
1. Round-trip encode/decode
2. Boundary values (0, MAX_UINT64, etc)
3. Truncated input at every byte position
4. Invalid type bytes
5. Path encoding edge cases (empty, max length, Unicode)

## Why This Makes Linus Happy

1. **No allocations**: Stack only, predictable memory
2. **No dependencies**: Pure C99, no external libs
3. **Fail fast**: Invalid input detected immediately
4. **Simple**: ~200 lines, obvious correctness
5. **Efficient**: Minimal CPU cycles, no waste

As Linus says: "Show me the code." And this code shows exactly what it does, no more, no less.