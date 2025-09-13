/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_H
#define GITMIND_H

/* Umbrella header: aggregate public API from core */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Public output API */
#include "gitmind/output.h"

/* Core public headers */
#include "gitmind/constants.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/attribution.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/journal.h"
#include "gitmind/cache.h"

/* Optional utility/public headers */
#if defined(GITMIND_ENABLE_IO)
#include "gitmind/io/io.h"
#endif
#if defined(GITMIND_ENABLE_TIME)
#include "gitmind/time/time.h"
#endif
#if defined(GITMIND_ENABLE_UTIL)
#include "gitmind/util/memory.h"
#endif
#if defined(GITMIND_ENABLE_UTF8)
#include "gitmind/utf8/validate.h"
#endif

#endif /* GITMIND_H */

