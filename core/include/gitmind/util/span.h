/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SPAN_H
#define GITMIND_SPAN_H

#include <stddef.h>

typedef struct {
    void   *ptr;   /* start of the buffer                        */
    size_t  cap;   /* number of _addressable_ bytes from ptr     */
} gm_span_t;

/* Const view when you're only inspecting data */
typedef struct {
    const void *ptr;
    size_t      cap;
} gm_cspan_t;

/* Tiny helpers so call-sites stay readable */
#define GM_SPAN(buf, len)      ((gm_span_t){ (buf), (len) })
#define GM_CSPAN(buf, len)     ((gm_cspan_t){ (buf), (len) })

#endif /* GITMIND_SPAN_H */