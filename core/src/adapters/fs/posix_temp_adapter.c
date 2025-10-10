/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include "gitmind/adapters/fs/posix_temp_adapter.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "gitmind/constants_internal.h"
#include "gitmind/error.h"
#include "gitmind/fs/path_utils.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"

#ifndef GM_HAVE_REALPATH_DECL
#define GM_HAVE_REALPATH_DECL 1
extern char *realpath(const char *path, char *resolved_path);
#endif

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
static gm_result_void_t errno_to_result(const char *operation,
                                        const char *path);

static const uint64_t kFnvOffsetBasis = 1469598103934665603ULL;
static const uint64_t kFnvPrime = 1099511628211ULL;
static const uint64_t kFnvMixConstant = 0x9E3779B97F4A7C15ULL;
enum {
    kRepoIdHexLength = 32,
    kRepoIdHexBufferSize = kRepoIdHexLength + 1,
    kTempSuffixAttempts = 64
};
static const unsigned int kTempRandomMultiplier = 1103515245U;
static const unsigned int kTempRandomIncrement = 12345U;
static const unsigned int kTempRandomMask = 0x00FFFFFFU;

static gm_result_void_t copy_c_string(char *destination, size_t destination_size,
                                      const char *source,
                                      const char *error_context) {
    if (destination == NULL || source == NULL || destination_size == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "%s", error_context));
    }

    size_t source_length = strnlen(source, destination_size);
    if (source_length >= destination_size) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "%s", error_context));
    }

    if (gm_memcpy_span(destination, destination_size, source, source_length) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "%s", error_context));
    }
    destination[source_length] = '\0';
    return gm_ok_void();
}

static inline uint64_t fnv1a64(uint64_t seed, const char *data) {
    uint64_t hash = seed;
    for (const unsigned char *cursor = (const unsigned char *)data; *cursor != '\0';
         ++cursor) {
        hash ^= (uint64_t)(*cursor);
        hash *= kFnvPrime;
    }
    return hash;
}

GM_NODISCARD gm_result_void_t gm_repo_id_from_path(const char *abs_repo_path,
                                                   gm_repo_id_t *out_id) {
    if (abs_repo_path == NULL || out_id == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repo path or output is null"));
    }

    uint64_t high_hash = fnv1a64(kFnvOffsetBasis, abs_repo_path);
    /* Mix suffix to differentiate high/low halves deterministically */
    uint64_t low_hash = fnv1a64(high_hash ^ kFnvMixConstant, abs_repo_path);

    out_id->hi = high_hash;
    out_id->lo = low_hash;
    return gm_ok_void();
}

static gm_result_void_t ensure_dir_exists(const char *path) {
    if (path == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "path is null when ensuring dir"));
    }

    char normalized[GM_PATH_MAX];
    GM_TRY(copy_c_string(normalized, sizeof(normalized), path,
                         "path exceeds buffer while ensuring dir"));

    size_t length = strlen(normalized);
    if (length == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_PATH, "cannot ensure empty path"));
    }

    while (length > 1U && normalized[length - 1U] == '/') {
        normalized[--length] = '\0';
    }

    struct stat metadata = {0};
    if (stat(normalized, &metadata) == 0) {
        if (!S_ISDIR(metadata.st_mode)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_INVALID_PATH, "path exists but is not a directory"));
        }
        return gm_ok_void();
    }
    if (errno != ENOENT) {
        return errno_to_result("stat", normalized);
    }

    size_t missing_offsets[GM_PATH_MAX] = {0};
    size_t missing_count = 0U;

    char working[GM_PATH_MAX];
    GM_TRY(copy_c_string(working, sizeof(working), normalized,
                         "failed to copy path while ensuring dir"));

    while (true) {
        if (stat(working, &metadata) == 0) {
            if (!S_ISDIR(metadata.st_mode)) {
                return gm_err_void(
                    GM_ERROR(GM_ERR_INVALID_PATH, "path exists but is not a directory"));
            }
            break;
        }
        if (errno != ENOENT) {
            return errno_to_result("stat", working);
        }

        if (missing_count >= (sizeof(missing_offsets) / sizeof(missing_offsets[0]))) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH,
                                        "directory depth exceeds supported limit"));
        }
        missing_offsets[missing_count++] = strlen(working);

        char parent[GM_PATH_MAX];
        gm_result_void_t parent_result =
            gm_fs_path_dirname(working, parent, sizeof(parent));
        if (!parent_result.ok) {
            return parent_result;
        }

        if (strcmp(parent, working) == 0) {
            break;
        }

        GM_TRY(copy_c_string(working, sizeof(working), parent,
                             "parent path exceeds buffer"));
        if (working[0] == '\0') {
            break;
        }
    }

    for (size_t index = missing_count; index > 0U; --index) {
        size_t create_length = missing_offsets[index - 1U];
        if (create_length == 0U) {
            continue;
        }
        if (create_length == 1U && normalized[0] == '/') {
            continue; /* root always exists */
        }
        if (create_length >= sizeof(normalized)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "computed path exceeds buffer"));
        }

        char create_path[GM_PATH_MAX];
        for (size_t i = 0U; i < create_length; ++i) {
            create_path[i] = normalized[i];
        }
        create_path[create_length] = '\0';

        if (mkdir(create_path, DIR_PERMS_NORMAL) == 0) {
            continue;
        }
        if (errno == EEXIST) {
            continue;
        }
        if (errno == ENOENT) {
            /* Parent unexpectedly missing; retry in next iteration */
            continue;
        }
        return errno_to_result("mkdir", create_path);
    }

    if (stat(normalized, &metadata) == 0 && S_ISDIR(metadata.st_mode)) {
        return gm_ok_void();
    }
    return errno_to_result("stat", normalized);
}

static gm_result_void_t resolve_home(char *buffer, size_t buffer_size) {
    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        GM_TRY(copy_c_string(buffer, buffer_size, home,
                             "HOME path exceeds buffer"));
        return gm_ok_void();
    }

    struct passwd *passwd_entry = getpwuid(getuid());
    if (passwd_entry == NULL || passwd_entry->pw_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND, "unable to resolve HOME"));
    }
    GM_TRY(copy_c_string(buffer, buffer_size, passwd_entry->pw_dir,
                         "pw_dir exceeds buffer"));
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
    if (home_result.ok) {
        size_t home_length = strlen(home);
        bool home_is_root = (home_length == 1U && home[0] == '/');
        if (home_length > 0U && !home_is_root) {
            if (gm_snprintf(state->base_state, sizeof(state->base_state),
                            "%s/.gitmind", home) < 0) {
                return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                            "failed to compose state base"));
            }

            if (ensure) {
                gm_result_void_t ensure_result =
                    ensure_dir_exists(state->base_state);
                if (!ensure_result.ok) {
                    if (ensure_result.u.err != NULL) {
                        gm_error_free(ensure_result.u.err);
                    }
                } else {
                    state->base_state_ready = true;
                    return gm_ok_void();
                }
            } else {
                state->base_state_ready = true;
                return gm_ok_void();
            }
        }
    } else if (home_result.u.err != NULL) {
        gm_error_free(home_result.u.err);
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

    GM_TRY(copy_c_string(state->base_temp, sizeof(state->base_temp), tmp,
                         "TMPDIR exceeds buffer"));

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
    for (const char *cursor = component; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            return true;
        }
    }
    return false;
}

static gm_result_void_t format_repo_component(const gm_posix_fs_state_t *state,
                                              gm_repo_id_t repo, char *out,
                                              size_t out_size) {
    (void)state;
    if (out_size < kRepoIdHexBufferSize) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_LENGTH, "repo id buffer too small"));
    }
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

    char repo_component[kRepoIdHexBufferSize];
    GM_TRY(format_repo_component(state, repo, repo_component,
                                 sizeof(repo_component)));

    const char *segments[] = {repo_component};
    GM_TRY(copy_c_string(state->scratch, sizeof(state->scratch), base_state,
                         "failed copying base state"));

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
        GM_TRY(copy_c_string(state->scratch, sizeof(state->scratch),
                             template_path,
                             "copying deterministic temp dir failed"));
        out_dir->path = state->scratch;
        return gm_ok_void();
    }

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    for (unsigned int attempt = 0; attempt < kTempSuffixAttempts; ++attempt) {
        seed = (seed * kTempRandomMultiplier) + kTempRandomIncrement;
        unsigned int suffix = seed & kTempRandomMask;
        if (gm_snprintf(template_path, sizeof(template_path), "%s/%s-%06X",
                        state->scratch, component, suffix) < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "failed to format temp dir suffix"));
        }

        if (mkdir(template_path, DIR_PERMS_NORMAL) == 0) {
            gm_result_void_t random_copy =
                copy_c_string(state->scratch, sizeof(state->scratch), template_path,
                              "copying temp dir path failed");
            if (!random_copy.ok) {
                /* Attempt to clean up before returning */
                (void)rmdir(template_path);
                return random_copy;
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

static gm_result_void_t remove_tree_at(int directory_fd, const char *entry_name);

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

static gm_result_void_t remove_tree_at(int directory_fd, const char *entry_name) {
    struct stat stat_info = {0};
    // Use fstatat to check stat info relative to dirfd
    if (fstatat(directory_fd, entry_name, &stat_info, AT_SYMLINK_NOFOLLOW) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "fstatat failed for %s: %s", entry_name,
                                    strerror(errno)));
    }

    if (!S_ISDIR(stat_info.st_mode)) {
        // Unlink regular file or symlink or device, etc.
        if (unlinkat(directory_fd, entry_name, 0) != 0) {
            if (errno == ENOENT) {
                return gm_ok_void();
            }
            return gm_err_void(
                GM_ERROR(GM_ERR_IO_FAILED, "unlinkat failed for %s: %s", entry_name,
                         strerror(errno)));
        }
        return gm_ok_void();
    }

    // Open the directory itself
    int child_directory_fd = openat(directory_fd, entry_name, O_RDONLY | O_DIRECTORY);
    if (child_directory_fd < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "openat (directory) failed for %s: %s", entry_name,
                     strerror(errno)));
    }

    DIR *directory_stream = fdopendir(child_directory_fd);
    if (directory_stream == NULL) {
        (void)close(child_directory_fd); // Ensure descriptor closed
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "fdopendir failed for %s: %s", entry_name,
                     strerror(errno)));
    }

    struct dirent *directory_entry = NULL;
    while ((directory_entry = readdir(directory_stream)) != NULL) {
        if (is_dot_entry(directory_entry)) {
            continue;
        }

        // Call recursively with subdirfd as dirfd and entry->d_name
        gm_result_void_t child_result =
            remove_tree_at(child_directory_fd, directory_entry->d_name);
        if (!child_result.ok) {
            (void)closedir(directory_stream); // This closes child_directory_fd as well
            return child_result;
        }
    }

    (void)closedir(directory_stream); // This closes child_directory_fd
    // Remove the directory itself. Use unlinkat with AT_REMOVEDIR.
    if (unlinkat(directory_fd, entry_name, AT_REMOVEDIR) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "unlinkat (rmdir) failed for %s: %s", entry_name,
                     strerror(errno)));
    }

    return gm_ok_void();
}

static gm_result_void_t join_under_base(gm_posix_fs_state_t *state,
                                        gm_fs_base_t base, gm_repo_id_t repo,
                                        const char *segment1,
                                        const char *segment2,
                                        const char *segment3,
                                        const char *segment4,
                                        const char *segment5,
                                        const char **out_abs_path) {
    if (out_abs_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "output path pointer missing"));
    }

    const char *base_path = NULL;
    GM_TRY(base_dir_dispatch(state, base, true, &base_path));

    GM_TRY(copy_c_string(state->scratch, sizeof(state->scratch), base_path,
                         "failed copying base path"));

    char repo_component[kRepoIdHexBufferSize];
    GM_TRY(format_repo_component(state, repo, repo_component,
                                 sizeof(repo_component)));

    const char *path_segments[] = {
        repo_component, segment1, segment2, segment3, segment4, segment5};
    GM_TRY(join_segments(state->scratch, sizeof(state->scratch),
                         strlen(state->scratch), path_segments,
                         sizeof(path_segments) / sizeof(path_segments[0])));

    *out_abs_path = state->scratch;
    return gm_ok_void();
}

static gm_result_void_t canonicalize_existing_path(gm_posix_fs_state_t *state,
                                                   const char *abs_path_in,
                                                   const char **out_abs_path) {
    if (realpath(abs_path_in, state->scratch) == NULL) {
        return errno_to_result("realpath", abs_path_in);
    }
    *out_abs_path = state->scratch;
    return gm_ok_void();
}

static gm_result_void_t canonicalize_create_ok_path(gm_posix_fs_state_t *state,
                                                    const char *abs_path_in,
                                                    const char **out_abs_path) {
    char normalized[GM_PATH_MAX];
    GM_TRY(gm_fs_path_normalize_logical(abs_path_in, normalized,
                                        sizeof(normalized)));

    if (normalized[0] != '/') {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                     "create-ok canonicalize requires absolute path"));
    }

    char parent_path[GM_PATH_MAX];
    GM_TRY(gm_fs_path_dirname(normalized, parent_path, sizeof(parent_path)));

    if (realpath(parent_path, state->scratch) == NULL) {
        return errno_to_result("realpath", parent_path);
    }

    size_t base_length = strlen(state->scratch);
    GM_TRY(gm_fs_path_basename_append(state->scratch, sizeof(state->scratch),
                                      &base_length, normalized));
    *out_abs_path = state->scratch;
    return gm_ok_void();
}

static gm_result_void_t canonicalize_logical_path(gm_posix_fs_state_t *state,
                                                  const char *abs_path_in,
                                                  const char **out_abs_path) {
    GM_TRY(gm_fs_path_normalize_logical(abs_path_in, state->scratch,
                                        sizeof(state->scratch)));
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
    case GM_FS_CANON_PHYSICAL_EXISTING:
        return canonicalize_existing_path(state, abs_path_in, out_abs_path);
    case GM_FS_CANON_PHYSICAL_CREATE_OK:
        return canonicalize_create_ok_path(state, abs_path_in, out_abs_path);
    case GM_FS_CANON_LOGICAL:
    default:
        return canonicalize_logical_path(state, abs_path_in, out_abs_path);
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
                                               const char *segment1,
                                               const char *segment2,
                                               const char *segment3,
                                               const char *segment4,
                                               const char *segment5,
                                               const char **out_abs_path) {
    return join_under_base((gm_posix_fs_state_t *)self, base, repo, segment1,
                           segment2, segment3, segment4, segment5, out_abs_path);
}

static gm_result_void_t canonicalize_bridge(void *self, const char *abs_path_in,
                                            gm_fs_canon_opts_t opts,
                                            const char **out_abs_path) {
    return canonicalize_impl((gm_posix_fs_state_t *)self, abs_path_in, opts,
                             out_abs_path);
}

static const gm_fs_temp_port_vtbl_t GmPosixFsTempVtbl = {
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

    out_port->vtbl = &GmPosixFsTempVtbl;
    out_port->self = state;

    if (out_state != NULL) {
        *out_state = state;
    }
    if (out_dispose != NULL) {
        *out_dispose = dispose_port;
    }

    return gm_ok_void();
}
