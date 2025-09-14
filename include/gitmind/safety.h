/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SAFETY_H
#define GITMIND_SAFETY_H

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check whether a remote URL appears to point to the official
 * git-mind repository.
 *
 * The match is strict on the path suffix "neuroglyph/git-mind",
 * allowing an optional ".git" or a single trailing slash.
 *
 * Examples (true):
 *  - https://github.com/neuroglyph/git-mind
 *  - https://github.com/neuroglyph/git-mind.git
 *  - git@github.com:neuroglyph/git-mind.git
 *  - ssh://git@github.com/neuroglyph/git-mind/
 *
 * Examples (false):
 *  - https://github.com/neuroglyph/git-mind-extra
 *  - https://github.com/other/git-mind
 *  - https://github.com/neuroglyph/git-mind/foo
 */
static inline int gm_url_is_official_repo(const char *url) {
    if (!url) return 0;
    const char *needle = "neuroglyph/git-mind";
    const char *p = strstr(url, needle);
    if (!p) return 0;
    const char *rest = p + strlen(needle);
    if (rest[0] == '\0') return 1;
    if (strcmp(rest, ".git") == 0) return 1;
    if (rest[0] == '/' && rest[1] == '\0') return 1;
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_SAFETY_H */

