/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTABILITY_THREADS_H
#define GITMIND_PORTABILITY_THREADS_H

/* Portable threads.h implementation for macOS/systems without C11 threads */

#ifdef __APPLE__
/* macOS doesn't have threads.h, use pthread */
#include <pthread.h>

typedef pthread_once_t once_flag;
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#define call_once(flag, func) pthread_once(flag, func)

#else
/* Use standard C11 threads.h if available */
#include <threads.h>
#endif

#endif /* GITMIND_PORTABILITY_THREADS_H */
