# CLI JSON Contracts

Every `--json` output from the git-mind CLI includes a standard envelope:

```json
{
  "schemaVersion": 1,
  "command": "status",
  ...
}
```

- **`schemaVersion`** — Increments only on breaking changes to the JSON structure. Non-breaking additions (new optional fields) do not increment it.
- **`command`** — The command that produced this output. Useful for routing in pipelines that handle multiple git-mind outputs.

## Version Policy

| Scenario | schemaVersion change |
|----------|---------------------|
| New optional field added | No change |
| Field renamed or removed | Increment |
| Field type changed | Increment |
| Array wrapped in object | Increment |
| New command added | No change (new schema) |

## Command-to-Schema Table

| Command | Schema File |
|---------|-------------|
| `nodes --id <id> --json` | [`node-detail.schema.json`](cli/node-detail.schema.json) |
| `nodes --json` | [`node-list.schema.json`](cli/node-list.schema.json) |
| `status --json` | [`status.schema.json`](cli/status.schema.json) |
| `at <ref> --json` | [`at.schema.json`](cli/at.schema.json) |
| `import --json` | [`import.schema.json`](cli/import.schema.json) |
| `import --from-markdown --json` | [`import.schema.json`](cli/import.schema.json) |
| `export --json` (stdout) | [`export-data.schema.json`](cli/export-data.schema.json) |
| `export --file <path> --json` | [`export-file.schema.json`](cli/export-file.schema.json) |
| `merge --json` | [`merge.schema.json`](cli/merge.schema.json) |
| `doctor --json` | [`doctor.schema.json`](cli/doctor.schema.json) |
| `suggest --json` | [`suggest.schema.json`](cli/suggest.schema.json) |
| `review --json` | [`review-list.schema.json`](cli/review-list.schema.json) |
| `review --batch --json` | [`review-batch.schema.json`](cli/review-batch.schema.json) |
| `diff <ref-a>..<ref-b> --json` | [`diff.schema.json`](cli/diff.schema.json) |
| `set <nodeId> <key> <value> --json` | [`set.schema.json`](cli/set.schema.json) |
| `unset <nodeId> <key> --json` | [`unset.schema.json`](cli/unset.schema.json) |
| `view progress --json` | [`view-progress.schema.json`](cli/view-progress.schema.json) |

> **Note:** `nodes --id` and `nodes` (list) both emit `"command": "nodes"`. To select the correct schema, check for a top-level `id` field (node-detail) vs. a `nodes` array (node-list).

## Programmatic Validation

```javascript
import Ajv from 'ajv/dist/2020.js';
import schema from './docs/contracts/cli/status.schema.json' with { type: 'json' };

const ajv = new Ajv({ strict: true, allErrors: true });
const validate = ajv.compile(schema);

const output = JSON.parse(execSync('git mind status --json'));
if (!validate(output)) {
  console.error(validate.errors);
}
```

## Migration Guide

### `nodes --json` (Breaking)

The output changed from a bare JSON array to a wrapped object.

```bash
# Before (prior to v3.0.0):
git mind nodes --json | jq '.[]'

# After:
git mind nodes --json | jq '.nodes[]'
```

### `review --json` (Breaking)

The output changed from a bare JSON array to a wrapped object.

```bash
# Before (prior to v3.0.0):
git mind review --json | jq '.[].source'

# After:
git mind review --json | jq '.pending[].source'
```
