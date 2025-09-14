/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stdio.h>
#include <string.h>

#include "gitmind/safety.h"

static int expect_true(const char *url) {
    if (!gm_url_is_official_repo(url)) {
        fprintf(stderr, "Expected TRUE for: %s\n", url ? url : "(null)");
        return 1;
    }
    return 0;
}

static int expect_false(const char *url) {
    if (gm_url_is_official_repo(url)) {
        fprintf(stderr, "Expected FALSE for: %s\n", url ? url : "(null)");
        return 1;
    }
    return 0;
}

int main(void) {
    int fails = 0;

    /* True cases */
    fails += expect_true("https://github.com/neuroglyph/git-mind");
    fails += expect_true("https://github.com/neuroglyph/git-mind.git");
    fails += expect_true("git@github.com:neuroglyph/git-mind.git");
    fails += expect_true("ssh://git@github.com/neuroglyph/git-mind/");

    /* False cases */
    fails += expect_false("https://github.com/neuroglyph/git-mind-extra");
    fails += expect_false("https://github.com/other/git-mind");
    fails += expect_false("https://github.com/neuroglyph/git-mind/foo");
    fails += expect_false(NULL);

    if (fails) {
        fprintf(stderr, "test_safety: %d failures\n", fails);
        return 1;
    }
    return 0;
}

