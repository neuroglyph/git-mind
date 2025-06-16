# gitmind-link(1) - Create semantic links between files

## SYNOPSIS

**gitmind link** *source* *target* [**--type** *TYPE*]

## DESCRIPTION

The **gitmind link** command creates a semantic relationship between two files in your repository. Links are stored as plain text files in `.gitmind/links/` with filenames based on the SHA-1 hash of their content.

## OPTIONS

**--type** *TYPE*
: Specify the relationship type. Default is REFERENCES.

## LINK TYPES

**IMPLEMENTS**
: Source file implements concepts or specifications from target

**REFERENCES**  
: Source file references or cites target (default)

**INSPIRED_BY**
: Source file was inspired by ideas in target

**DEPENDS_ON**
: Source file depends on target to function

**CONTRADICTS**
: Source file contradicts or disputes target

**DISCUSSES**
: Source file discusses topics from target

**REVIEWS**
: Source file reviews or critiques target

## LINK FORMAT

Links are stored in the format:
```
TYPE: source -> target  # ts:timestamp
```

The filename is the SHA-1 hash of this content, ensuring deduplication.

## EXAMPLES

Create a simple reference link:
```
$ gitmind link README.md docs/guide.md
Created link: README.md -> docs/guide.md (REFERENCES)
```

Specify that a file implements a specification:
```
$ gitmind link src/auth.c specs/oauth2.md --type IMPLEMENTS
Created link: src/auth.c -> specs/oauth2.md (IMPLEMENTS)
```

Document inspiration:
```
$ gitmind link ideas/new-feature.md research/paper.pdf --type INSPIRED_BY
Created link: ideas/new-feature.md -> research/paper.pdf (INSPIRED_BY)
```

## BEHAVIOR

1. Validates that both source and target files exist
2. Creates canonical link content with timestamp
3. Computes SHA-1 hash of content
4. Writes link to `.gitmind/links/<hash>.link`
5. Stages the new link file with `git add`

Duplicate links are automatically prevented by the SHA-1 naming scheme.

## ERRORS

The command will fail if:
- Source file does not exist
- Target file does not exist  
- GitMind is not initialized (run `gitmind init` first)
- File system permissions prevent writing

## SEE ALSO

gitmind(1), gitmind-unlink(1), gitmind-list(1)

## COPYRIGHT

Copyright © 2025 J. Kirby Ross / Neuroglyph Collective. Licensed under Apache 2.0.