/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/fs/posix_temp_adapter.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "gitmind/constants_internal.h"
#include "gitmind/error.h"
#include "gitmind/fs/path_utils.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"

extern char *realpath(const char *restrict path, char *restrict resolved_path);

struct gm_posix_fs_state {
    char scratch[GM_PATH_MAX];
    char base_state[GM_PATH_MAX];
    char base_temp[GM_PATH_MAX];
    bool base_state_ready;
    bool base_temp_ready;
};

static gm_result_void_t state_base_dir(gm_posix_fs_state_t *state,
                                       bool ensure);
static gm_result_void_t temp_base_dir(gm_posix_fs_state_t *state,
                                      bool ensure);

static inline uint64_t fnv1a64(uint64_t seed, const char *data) {
    const uint64_t prime = 1099511628211ULL;
    uint64_t hash = seed;
    for (const unsigned char *p = (const unsigned char *)data; *p != '\0'; ++p) {
        hash ^= (uint64_t)(*p);
        hash *= prime;
    }
    return hash;
}

GM_NODISCARD gm_result_void_t gm_repo_id_from_path(const char *abs_repo_path,
                                                   gm_repo_id_t *out_id) {
    if (abs_repo_path == NULL || out_id == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repo path or output is null"));
    }

    const uint64_t offset = 1469598103934665603ULL;
    uint64_t hi = fnv1a64(offset, abs_repo_path);
    /* Mix suffix to differentiate high/low halves deterministically */
    uint64_t lo = fnv1a64(hi ^ 0x9E3779B97F4A7C15ULL, abs_repo_path);

    out_id->hi = hi;
    out_id->lo = lo;
    return gm_ok_void();
}

static gm_result_void_t ensure_dir_exists(const char *path) {
    if (path == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "path is null when ensuring dir"));
    }

    struct stat st = {0};
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return gm_ok_void();
        }
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_PATH, "path exists but is not a directory"));
    }

    if (mkdir(path, DIR_PERMS_NORMAL) == 0) {
        return gm_ok_void();
    }

    if (errno == ENOENT) {
        char parent[GM_PATH_MAX];
        const char *slash = strrchr(path, '/');
        if (slash == NULL || slash == path) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH,
                                        "cannot determine parent for %s", path));
        }
        size_t len = (size_t)(slash - path);
        if (len >= sizeof(parent)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "parent path too long"));
        }
        memcpy(parent, path, len);
        parent[len] = '\0';
        GM_TRY(ensure_dir_exists(parent));
        if (mkdir(path, DIR_PERMS_NORMAL) == 0) {
            return gm_ok_void();
        }
    }

    if (errno == EEXIST) {
        return gm_ok_void();
    }
    return gm_err_void(
        GM_ERROR(GM_ERR_IO_FAILED, "mkdir failed for %s: %s", path, strerror(errno)));
}

static gm_result_void_t resolve_home(char *buffer, size_t buffer_size) {
    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        if (gm_strcpy_safe(buffer, buffer_size, home) != GM_OK) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "HOME path exceeds buffer"));
        }
        return gm_ok_void();
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL || pw->pw_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND, "unable to resolve HOME"));
    }
    if (gm_strcpy_safe(buffer, buffer_size, pw->pw_dir) != GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "pw_dir exceeds buffer"));
    }
    return gm_ok_void();
}

static gm_result_void_t errno_to_result(const char *operation,
                                        const char *path) {
    int err = errno;
    int32_t code = GM_ERR_IO_FAILED;
    switch (err) {
    case ENOENT:
        code = GM_ERR_NOT_FOUND;
        break;
    case EACCES:
        code = GM_ERR_PERMISSION_DENIED;
        break;
    case ENAMETOOLONG:
        code = GM_ERR_PATH_TOO_LONG;
        break;
    case EROFS:
        code = GM_ERR_READ_ONLY;
        break;
    default:
        code = GM_ERR_IO_FAILED;
        break;
    }

    return gm_err_void(
        GM_ERROR(code, "%s failed for %s: %s", operation, path, strerror(err)));
}

static gm_result_void_t state_base_dir(gm_posix_fs_state_t *state,
                                       bool ensure) {
    if (state->base_state_ready) {
        return gm_ok_void();
    }

    char home[GM_PATH_MAX];
    gm_result_void_t home_result = resolve_home(home, sizeof(home));
    bool use_temp_fallback = false;

    if (!home_result.ok) {
        if (home_result.u.err != NULL) {
            gm_error_free(home_result.u.err);
        }
        use_temp_fallback = true;
    } else if (home[0] == '\0' ||
               (home[0] == '/' && home[1] == '\0')) {
        /* Empty or root-only home is unsafe; fall back to temp. */
        use_temp_fallback = true;
    } else {
        if (gm_snprintf(state->base_state, sizeof(state->base_state),
                        "%s/.gitmind", home) < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "failed to compose state base"));
        }

        if (ensure) {
            gm_result_void_t ensure_result = ensure_dir_exists(state->base_state);
            if (!ensure_result.ok) {
                if (ensure_result.u.err != NULL) {
                    gm_error_free(ensure_result.u.err);
                }
                use_temp_fallback = true;
            }
        }

        if (!use_temp_fallback) {
            state->base_state_ready = true;
            return gm_ok_void();
        }
    }

    GM_TRY(temp_base_dir(state, ensure));

    if (gm_snprintf(state->base_state, sizeof(state->base_state),
                    "%s/gitmind-state", state->base_temp) < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "failed to compose fallback state base"));
    }

    if (ensure) {
        gm_result_void_t ensure_result = ensure_dir_exists(state->base_state);
        if (!ensure_result.ok) {
            return ensure_result;
        }
    }

    state->base_state_ready = true;
    return gm_ok_void();
}

static gm_result_void_t temp_base_dir(gm_posix_fs_state_t *state, bool ensure) {
    if (state->base_temp_ready) {
        return gm_ok_void();
    }

    const char *tmp = getenv("TMPDIR");
    if (tmp == NULL || tmp[0] == '\0') {
        tmp = "/tmp";
    }

    if (gm_strcpy_safe(state->base_temp, sizeof(state->base_temp), tmp) !=
        GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "TMPDIR exceeds buffer"));
    }

    if (ensure) {
        GM_TRY(ensure_dir_exists(state->base_temp));
    }

    state->base_temp_ready = true;
    return gm_ok_void();
}

static gm_result_void_t base_dir_dispatch(gm_posix_fs_state_t *state,
                                          gm_fs_base_t base, bool ensure,
                                          const char **out_path) {
    if (out_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "output path pointer is null"));
    }

    if (base == GM_FS_BASE_STATE) {
        GM_TRY(state_base_dir(state, ensure));
        *out_path = state->base_state;
        return gm_ok_void();
    }

    GM_TRY(temp_base_dir(state, ensure));
    *out_path = state->base_temp;
    return gm_ok_void();
}

static bool has_path_separator(const char *component) {
    if (component == NULL) {
        return false;
    }
    for (const char *p = component; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            return true;
        }
    }
    return false;
}

static gm_result_void_t format_repo_component(const gm_posix_fs_state_t *state,
                                              gm_repo_id_t repo, char *out,
                                              size_t out_size) {
    (void)state;
    if (gm_snprintf(out, out_size, "%016" PRIx64 "%016" PRIx64, repo.hi,
                    repo.lo) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed to format repo id"));
    }
    return gm_ok_void();
}

static gm_result_void_t join_segments(char *buffer, size_t buffer_size,
                                      size_t start_index, const char *segments[],
                                      size_t segment_count) {
    size_t idx = start_index;
    for (size_t i = 0; i < segment_count; ++i) {
        const char *seg = segments[i];
        if (seg == NULL || seg[0] == '\0') {
            continue;
        }
        int wrote = gm_snprintf(buffer + idx, buffer_size - idx, "%s%s",
                                (idx > 0) ? "/" : "", seg);
        if (wrote < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "unable to join path segment"));
        }
        idx += (size_t)wrote;
        if (idx >= buffer_size) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "path exceeded buffer"));
        }
    }
    return gm_ok_void();
}

static gm_result_void_t make_temp_dir_impl(gm_posix_fs_state_t *state,
                                           gm_repo_id_t repo,
                                           const char *component,
                                           bool suffix_random,
                                           gm_tempdir_t *out_dir) {
    if (out_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "temp dir output pointer missing"));
    }
    if (component == NULL || component[0] == '\0' || has_path_separator(component)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "component must be non-empty without separators"));
    }

    const char *base_state = NULL;
    GM_TRY(base_dir_dispatch(state, GM_FS_BASE_STATE, true, &base_state));

    char repo_component[33];
    GM_TRY(format_repo_component(state, repo, repo_component,
                                 sizeof(repo_component)));

    const char *segments[] = {repo_component};
    if (gm_strcpy_safe(state->scratch, sizeof(state->scratch), base_state) !=
        GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "failed copying base state"));
    }

    GM_TRY(join_segments(state->scratch, sizeof(state->scratch),
                         strlen(state->scratch), segments, 1));
    GM_TRY(ensure_dir_exists(state->scratch));

    char template_path[GM_PATH_MAX];
    if (gm_snprintf(template_path, sizeof(template_path), "%s/%s",
                    state->scratch, component) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed to format temp dir base"));
    }

    if (!suffix_random) {
        GM_TRY(ensure_dir_exists(template_path));
        if (gm_strcpy_safe(state->scratch, sizeof(state->scratch),
                           template_path) != GM_OK) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "copying deterministic temp dir failed"));
        }
        out_dir->path = state->scratch;
        return gm_ok_void();
    }

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    for (unsigned int attempt = 0; attempt < 64; ++attempt) {
        seed = (seed * 1103515245U) + 12345U;
        unsigned int suffix = seed & 0xFFFFFFU;
        if (gm_snprintf(template_path, sizeof(template_path), "%s/%s-%06X",
                        state->scratch, component, suffix) < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "failed to format temp dir suffix"));
        }

        if (mkdir(template_path, DIR_PERMS_NORMAL) == 0) {
            if (gm_strcpy_safe(state->scratch, sizeof(state->scratch),
                               template_path) != GM_OK) {
                /* Attempt to clean up before returning */
                (void)rmdir(template_path);
                return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                            "copying temp dir path failed"));
            }
            out_dir->path = state->scratch;
            return gm_ok_void();
        }

        if (errno == EEXIST) {
            continue;
        }

        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "mkdir failed for %s: %s", template_path,
                                    strerror(errno)));
    }

    return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                "unable to allocate temp dir after retries"));
}

static bool is_dot_entry(const struct dirent *entry) {
    if (entry == NULL) {
        return false;
    }
    return (strcmp(entry->d_name, ".") == 0) ||
           (strcmp(entry->d_name, "..") == 0);
}

static gm_result_void_t remove_tree_at(int dirfd, const char *name);

static gm_result_void_t remove_tree_impl(const char *path) {
    if (path == NULL) {
        return gm_ok_void();
    }
    // Open parent directory
    int dirfd = AT_FDCWD;
    // For absolute path, just use AT_FDCWD
    // For relative, this will also work as AT_FDCWD
    return remove_tree_at(dirfd, path);
}

static gm_result_void_t remove_tree_at(int dirfd, const char *name) {
    struct stat st = {0};
    // Use fstatat to check stat info relative to dirfd
    if (fstatat(dirfd, name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "fstatat failed for %s: %s", name,
                                    strerror(errno)));
    }

    if (!S_ISDIR(st.st_mode)) {
        // Unlink regular file or symlink or device, etc.
        if (unlinkat(dirfd, name, 0) != 0) {
            if (errno == ENOENT) {
                return gm_ok_void();
            }
            return gm_err_void(
                GM_ERROR(GM_ERR_IO_FAILED, "unlinkat failed for %s: %s", name,
                         strerror(errno)));
        }
        return gm_ok_void();
    }

    // Open the directory itself
    int subdirfd = openat(dirfd, name, O_RDONLY | O_DIRECTORY);
    if (subdirfd < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "openat (directory) failed for %s: %s", name,
                     strerror(errno)));
    }

    DIR *dir = fdopendir(subdirfd);
    if (dir == NULL) {
        close(subdirfd); // Make sure to close the fd
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "fdopendir failed for %s: %s", name,
                     strerror(errno)));
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (is_dot_entry(entry)) {
            continue;
        }

        // Call recursively with subdirfd as dirfd and entry->d_name
        gm_result_void_t child_result = remove_tree_at(subdirfd, entry->d_name);
        if (!child_result.ok) {
            closedir(dir); // This closes subdirfd as well
            return child_result;
        }
    }

    closedir(dir); // This closes subdirfd
    // Remove the directory itself. Use unlinkat with AT_REMOVEDIR.
    if (unlinkat(dirfd, name, AT_REMOVEDIR) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "unlinkat (rmdir) failed for %s: %s", name,
                     strerror(errno)));
    }

    return gm_ok_void();
}

static gm_result_void_t join_under_base(gm_posix_fs_state_t *state,
                                        gm_fs_base_t base, gm_repo_id_t repo,
                                        const char *s1, const char *s2,
                                        const char *s3, const char *s4,
                                        const char *s5,
                                        const char **out_abs_path) {
    if (out_abs_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "output path pointer missing"));
    }

    const char *base_path = NULL;
    GM_TRY(base_dir_dispatch(state, base, true, &base_path));

    if (gm_strcpy_safe(state->scratch, sizeof(state->scratch), base_path) !=
        GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "failed copying base path"));
    }

    char repo_component[33];
    GM_TRY(format_repo_component(state, repo, repo_component,
                                 sizeof(repo_component)));

    const char *segments[] = {repo_component, s1, s2, s3, s4, s5};
    GM_TRY(join_segments(state->scratch, sizeof(state->scratch),
                         strlen(state->scratch), segments,
                         sizeof(segments) / sizeof(segments[0])));

    *out_abs_path = state->scratch;
    return gm_ok_void();
}

static gm_result_void_t canonicalize_impl(gm_posix_fs_state_t *state,
                                          const char *abs_path_in,
                                          gm_fs_canon_opts_t opts,
                                          const char **out_abs_path) {
    if (abs_path_in == NULL || out_abs_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "path or output missing for canonicalize"));
    }

    switch (opts.mode) {
    case GM_FS_CANON_PHYSICAL_EXISTING: {
        if (realpath(abs_path_in, state->scratch) == NULL) {
            return errno_to_result("realpath", abs_path_in);
        }
        *out_abs_path = state->scratch;
        return gm_ok_void();
    }
    case GM_FS_CANON_PHYSICAL_CREATE_OK: {
        char normalized[GM_PATH_MAX];
        GM_TRY(gm_fs_path_normalize_logical(abs_path_in, normalized,
                                            sizeof(normalized)));

        if (normalized[0] != '/') {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "create-ok canonicalize requires absolute path"));
        }

        char parent[GM_PATH_MAX];
        GM_TRY(gm_fs_path_dirname(normalized, parent, sizeof(parent)));

        if (realpath(parent, state->scratch) == NULL) {
            return errno_to_result("realpath", parent);
        }

        size_t base_len = strlen(state->scratch);
        GM_TRY(gm_fs_path_basename_append(state->scratch, sizeof(state->scratch),
                                          &base_len, normalized));
        *out_abs_path = state->scratch;
        return gm_ok_void();
    }
    case GM_FS_CANON_LOGICAL:
    default: {
        GM_TRY(gm_fs_path_normalize_logical(abs_path_in, state->scratch,
                                            sizeof(state->scratch)));
        *out_abs_path = state->scratch;
        return gm_ok_void();
    }
    }
}

static gm_result_void_t base_dir_bridge(void *self, gm_fs_base_t base,
                                        bool ensure, const char **out_abs_path) {
    return base_dir_dispatch((gm_posix_fs_state_t *)self, base, ensure,
                             out_abs_path);
}

static gm_result_void_t make_temp_dir_bridge(void *self, gm_repo_id_t repo,
                                             const char *component,
                                             bool suffix_random,
                                             gm_tempdir_t *out_dir) {
    return make_temp_dir_impl((gm_posix_fs_state_t *)self, repo, component,
                              suffix_random, out_dir);
}

static gm_result_void_t remove_tree_bridge(void *self, const char *abs_path) {
    (void)self;
    return remove_tree_impl(abs_path);
}

static gm_result_void_t join_under_base_bridge(void *self, gm_fs_base_t base,
                                               gm_repo_id_t repo,
                                               const char *s1, const char *s2,
                                               const char *s3, const char *s4,
                                               const char *s5,
                                               const char **out_abs_path) {
    return join_under_base((gm_posix_fs_state_t *)self, base, repo, s1, s2, s3,
                           s4, s5, out_abs_path);
}

static gm_result_void_t canonicalize_bridge(void *self, const char *abs_path_in,
                                            gm_fs_canon_opts_t opts,
                                            const char **out_abs_path) {
    return canonicalize_impl((gm_posix_fs_state_t *)self, abs_path_in, opts,
                             out_abs_path);
}

static const gm_fs_temp_port_vtbl_t GM_POSIX_FS_TEMP_VTBL = {
    .base_dir = base_dir_bridge,
    .make_temp_dir = make_temp_dir_bridge,
    .remove_tree = remove_tree_bridge,
    .path_join_under_base = join_under_base_bridge,
    .canonicalize_ex = canonicalize_bridge,
};

static void dispose_port(gm_fs_temp_port_t *port) {
    if (port == NULL || port->self == NULL) {
        return;
    }

    gm_posix_fs_state_t *state = (gm_posix_fs_state_t *)port->self;
    free(state);
    port->self = NULL;
    port->vtbl = NULL;
}

gm_result_void_t gm_posix_fs_temp_port_create(gm_fs_temp_port_t *out_port,
                                              gm_posix_fs_state_t **out_state,
                                              void (**out_dispose)(gm_fs_temp_port_t *)) {
    if (out_port == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "out_port pointer is null"));
    }

    gm_posix_fs_state_t *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating fs port state failed"));
    }

    out_port->vtbl = &GM_POSIX_FS_TEMP_VTBL;
    out_port->self = state;

    if (out_state != NULL) {
        *out_state = state;
    }
    if (out_dispose != NULL) {
        *out_dispose = dispose_port;
    }

    return gm_ok_void();
}
