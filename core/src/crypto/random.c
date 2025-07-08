/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/random.h"

#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <sodium/randombytes.h>
#include <stddef.h>
#include <stdint.h>

/* gm_err_u32 and gm_err_u64 are now defined in result.h */

gm_result_void_t gm_random_bytes(void *buf, size_t size) {
    if (!buf) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr buffer"));
    }
    if (size == 0) {
        return gm_ok_void();
    }

    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->random_bytes) {
        backend->random_bytes(buf, size);
        return gm_ok_void();
    }

    /* Fallback to direct libsodium */
    randombytes_buf(buf, size);
    return gm_ok_void();
}

gm_result_u32_t gm_random_u32(void) {
    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->random_u32) {
        return (gm_result_u32_t){.ok = true, .u.val = backend->random_u32()};
    }

    /* Fallback to direct libsodium */
    return (gm_result_u32_t){.ok = true, .u.val = randombytes_random()};
}

gm_result_u64_t gm_random_u64(void) {
    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->random_u64) {
        return (gm_result_u64_t){.ok = true, .u.val = backend->random_u64()};
    }

    /* Fallback to direct libsodium */
    uint64_t val;
    randombytes_buf(&val, sizeof(val));
    return (gm_result_u64_t){.ok = true, .u.val = val};
}