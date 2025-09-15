/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_H
#define GITMIND_H

/* Umbrella header: aggregate public API from core */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/*
 * Feature toggles
 *
 * Define the following macros to include optional subsystems in the
 * umbrella header aggregation:
 *   - GITMIND_ENABLE_IO     : include gitmind/io/io.h
 *   - GITMIND_ENABLE_TIME   : include gitmind/time/time.h
 *   - GITMIND_ENABLE_UTIL   : include gitmind/util/memory.h
 *   - GITMIND_ENABLE_UTF8   : include gitmind/utf8/validate.h
 *
 * The Meson build exposes these as boolean options (enable_io, enable_time,
 * enable_util, enable_utf8) that define the corresponding macros when ON.
 */
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

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_H */
