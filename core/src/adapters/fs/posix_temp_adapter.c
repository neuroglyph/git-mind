/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/fs/posix_temp_adapter.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
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

#if defined(_WIN32)
#define gm_strtok_r strtok_s
#else
#define gm_strtok_r strtok_r
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

static const uint64_t FnvOffsetBasis = 1469598103934665603ULL;
static const uint64_t FnvPrime = 1099511628211ULL;
static const uint64_t FnvMixConstant = 0x9E3779B97F4A7C15ULL;
enum {
    RepoIdHexLength = 32,
    RepoIdHexBufferSize = RepoIdHexLength + 1,
    TempSuffixAttempts = 64
};
static const unsigned int TempRandomMultiplier = 1103515245U;
static const unsigned int TempRandomIncrement = 12345U;
static const unsigned int TempRandomMask = 0x00FFFFFFU;

static inline uint64_t fnv1a64(uint64_t seed, const char *data) {
    uint64_t hash = seed;
    for (const unsigned char *cursor = (const unsigned char *)data; *cursor != '\0';
         ++cursor) {
        hash ^= (uint64_t)(*cursor);
        hash *= FnvPrime;
    }
    return hash;
}

GM_NODISCARD gm_result_void_t gm_repo_id_from_path(const char *abs_repo_path,
                                                   gm_repo_id_t *out_id) {
    if (abs_repo_path == NULL || out_id == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repo path or output is null"));
    }

    uint64_t high_hash = fnv1a64(FnvOffsetBasis, abs_repo_path);
    /* Mix suffix to differentiate high/low halves deterministically */
    uint64_t low_hash = fnv1a64(high_hash ^ FnvMixConstant, abs_repo_path);

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
    gm_result_void_t normalize_rc =
        gm_fs_path_normalize_logical(path, normalized, sizeof(normalized));
    if (!normalize_rc.ok) {
        return normalize_rc;
    }

    if (normalized[0] == '\0' || (normalized[0] == '.' && normalized[1] == '\0')) {
        return gm_ok_void();
    }

    char working[GM_PATH_MAX];
    if (gm_strcpy_safe(working, sizeof(working), normalized) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG,
                     "normalized path exceeds buffer while ensuring dir"));
    }

    size_t working_len = strlen(working);
    bool is_absolute = (working[0] == '/');
    if (is_absolute && working_len == 1U) {
        return gm_ok_void(); /* root */
    }

    char building[GM_PATH_MAX];
    size_t building_len = 0U;
    if (is_absolute) {
        building[0] = '/';
        building[1] = '\0';
        building_len = 1U;
    } else {
        building[0] = '\0';
    }

    char *saveptr = NULL;
    char *segment = gm_strtok_r(working, "/", &saveptr);
    while (segment != NULL) {
        size_t segment_len = strlen(segment);
        if (segment_len == 0U) {
            segment = gm_strtok_r(NULL, "/", &saveptr);
            continue;
        }

        bool needs_separator = (building_len > 0U && building[building_len - 1U] != '/');
        size_t required = building_len + (needs_separator ? 1U : 0U) + segment_len + 1U;
        if (required > sizeof(building)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "computed path exceeds buffer"));
        }

        if (needs_separator) {
            building[building_len++] = '/';
        }
        if (gm_memcpy_span(building + building_len, sizeof(building) - building_len,
                           segment, segment_len) != 0) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "segment copy exceeded buffer"));
        }
        building_len += segment_len;
        building[building_len] = '\0';

        struct stat st = {0};
        if (stat(building, &st) != 0) {
            if (errno != ENOENT) {
                return errno_to_result("stat", building);
            }
            if (mkdir(building, DIR_PERMS_NORMAL) != 0 && errno != EEXIST) {
                return errno_to_result("mkdir", building);
            }
        } else if (!S_ISDIR(st.st_mode)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_INVALID_PATH, "path exists but is not a directory"));
        }

        segment = gm_strtok_r(NULL, "/", &saveptr);
    }

    return gm_ok_void();
}

static gm_result_void_t resolve_home(char *buffer, size_t buffer_size) {
    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        if (gm_strcpy_safe(buffer, buffer_size, home) != GM_OK) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "HOME path exceeds buffer"));
        }
        return gm_ok_void();
    }

    struct passwd *passwd_entry = getpwuid(getuid());
    if (passwd_entry == NULL || passwd_entry->pw_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND, "unable to resolve HOME"));
    }
    if (gm_strcpy_safe(buffer, buffer_size, passwd_entry->pw_dir) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "pw_dir exceeds buffer"));
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
    if (home_result.ok) {
        size_t home_length = strlen(home);
        bool home_is_root = (home_length == 1U && home[0] == '/');
        if (home_length > 0U && !home_is_root) {
            int home_written = gm_snprintf(state->base_state,
                                           sizeof(state->base_state),
                                           "%s/.gitmind", home);
            if (home_written < 0 ||
                (size_t)home_written >= sizeof(state->base_state)) {
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

    int fallback_written = gm_snprintf(state->base_state,
                                       sizeof(state->base_state),
                                       "%s/gitmind-state",
                                       state->base_temp);
    if (fallback_written < 0 ||
        (size_t)fallback_written >= sizeof(state->base_state)) {
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

    if (gm_strcpy_safe(state->base_temp, sizeof(state->base_temp), tmp) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "TMPDIR exceeds buffer"));
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
    if (out_size < RepoIdHexBufferSize) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_LENGTH, "repo id buffer too small"));
    }
    int formatted =
        gm_snprintf(out, out_size, "%016" PRIx64 "%016" PRIx64, repo.hi,
                    repo.lo);
    if (formatted < 0 || (size_t)formatted >= out_size) {
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
        size_t available = buffer_size - idx;
        int wrote = gm_snprintf(buffer + idx, available, "%s%s",
                                (idx > 0) ? "/" : "", seg);
        if (wrote < 0 || (size_t)wrote >= available) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "unable to join path segment"));
        }
        idx += (size_t)wrote;
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

    char repo_component[RepoIdHexBufferSize];
    GM_TRY(format_repo_component(state, repo, repo_component,
                                 sizeof(repo_component)));

    const char *segments[] = {repo_component};
    if (gm_strcpy_safe(state->scratch, sizeof(state->scratch), base_state) !=
        GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed copying base state"));
    }

    GM_TRY(join_segments(state->scratch, sizeof(state->scratch),
                         strlen(state->scratch), segments, 1));
    GM_TRY(ensure_dir_exists(state->scratch));

    char template_path[GM_PATH_MAX];
    int template_written = gm_snprintf(template_path, sizeof(template_path),
                                       "%s/%s", state->scratch, component);
    if (template_written < 0 ||
        (size_t)template_written >= sizeof(template_path)) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed to format temp dir base"));
    }

    if (!suffix_random) {
        GM_TRY(ensure_dir_exists(template_path));
        if (gm_strcpy_safe(state->scratch, sizeof(state->scratch),
                           template_path) != GM_OK) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG,
                         "copying deterministic temp dir failed"));
        }
        out_dir->path = state->scratch;
        return gm_ok_void();
    }

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    for (unsigned int attempt = 0; attempt < TempSuffixAttempts; ++attempt) {
        seed = (seed * TempRandomMultiplier) + TempRandomIncrement;
        unsigned int suffix = seed & TempRandomMask;
        int suffix_written = gm_snprintf(template_path, sizeof(template_path),
                                         "%s/%s-%06X", state->scratch,
                                         component, suffix);
        if (suffix_written < 0 ||
            (size_t)suffix_written >= sizeof(template_path)) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG,
                         "failed to format temp dir suffix"));
        }

        if (mkdir(template_path, DIR_PERMS_NORMAL) == 0) {
            if (gm_strcpy_safe(state->scratch, sizeof(state->scratch),
                               template_path) != GM_OK) {
                /* Attempt to clean up before returning */
                (void)rmdir(template_path);
                return gm_err_void(
                    GM_ERROR(GM_ERR_PATH_TOO_LONG,
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

typedef struct {
    DIR *dir;
    size_t path_len;
} dir_stack_frame_t;

static void close_dir_stack(dir_stack_frame_t *stack, size_t depth) {
    while (depth > 0U) {
        DIR *dir = stack[--depth].dir;
        if (dir != NULL) {
            closedir(dir);
        }
    }
}

static gm_result_void_t remove_tree_impl(const char *path) {
    if (path == NULL) {
        return gm_ok_void();
    }

    char normalized[GM_PATH_MAX];
    gm_result_void_t normalize_rc =
        gm_fs_path_normalize_logical(path, normalized, sizeof(normalized));
    if (!normalize_rc.ok) {
        return normalize_rc;
    }
    if (normalized[0] == '\0') {
        return gm_ok_void();
    }

    struct stat root_stat = {0};
    if (stat(normalized, &root_stat) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return errno_to_result("stat", normalized);
    }

    if (!S_ISDIR(root_stat.st_mode)) {
        if (unlink(normalized) != 0 && errno != ENOENT) {
            return errno_to_result("unlink", normalized);
        }
        return gm_ok_void();
    }

    dir_stack_frame_t stack[GM_PATH_MAX];
    size_t depth = 0U;

    char current[GM_PATH_MAX];
    if (gm_strcpy_safe(current, sizeof(current), normalized) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG,
                     "normalized path exceeds buffer while removing dir"));
    }
    size_t current_len = strlen(current);

    DIR *root_dir = opendir(current);
    if (root_dir == NULL) {
        return errno_to_result("opendir", current);
    }
    stack[depth++] = (dir_stack_frame_t){.dir = root_dir, .path_len = current_len};

    while (depth > 0U) {
        dir_stack_frame_t *frame = &stack[depth - 1U];
        struct dirent *entry = readdir(frame->dir);
        if (entry != NULL) {
            if (is_dot_entry(entry)) {
                continue;
            }

            size_t parent_len = frame->path_len;
            bool needs_separator =
                (parent_len > 0U && current[parent_len - 1U] != '/');
            size_t name_len = strlen(entry->d_name);
            size_t required = parent_len + (needs_separator ? 1U : 0U) +
                              name_len + 1U;
            if (required >= sizeof(current)) {
                close_dir_stack(stack, depth);
                return gm_err_void(
                    GM_ERROR(GM_ERR_PATH_TOO_LONG,
                             "child path exceeds buffer"));
            }

            size_t append_pos = parent_len;
            if (needs_separator) {
                current[append_pos++] = '/';
            }
            if (gm_memcpy_span(current + append_pos,
                               sizeof(current) - append_pos, entry->d_name,
                               name_len) != 0) {
                close_dir_stack(stack, depth);
                return gm_err_void(
                    GM_ERROR(GM_ERR_PATH_TOO_LONG,
                             "child path exceeds buffer"));
            }
            current_len = append_pos + name_len;
            current[current_len] = '\0';

            struct stat entry_stat = {0};
            if (stat(current, &entry_stat) != 0) {
                if (errno == ENOENT) {
                    current[parent_len] = '\0';
                    continue;
                }
                close_dir_stack(stack, depth);
                return errno_to_result("stat", current);
            }

            if (S_ISDIR(entry_stat.st_mode)) {
                DIR *child_dir = opendir(current);
                if (child_dir == NULL) {
                    close_dir_stack(stack, depth);
                    return errno_to_result("opendir", current);
                }
                if (depth >= (sizeof(stack) / sizeof(stack[0]))) {
                    closedir(child_dir);
                    close_dir_stack(stack, depth);
                    return gm_err_void(
                        GM_ERROR(GM_ERR_INVALID_PATH,
                                 "directory depth exceeds stack capacity"));
                }
                stack[depth++] =
                    (dir_stack_frame_t){.dir = child_dir, .path_len = current_len};
            } else {
                if (unlink(current) != 0 && errno != ENOENT) {
                    close_dir_stack(stack, depth);
                    return errno_to_result("unlink", current);
                }
                current[parent_len] = '\0';
            }
        } else {
            closedir(frame->dir);
            depth--;

            if (rmdir(current) != 0 && errno != ENOENT) {
                close_dir_stack(stack, depth);
                return errno_to_result("rmdir", current);
            }

            if (depth > 0U) {
                dir_stack_frame_t *parent = &stack[depth - 1U];
                current[parent->path_len] = '\0';
            }
        }
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

    if (gm_strcpy_safe(state->scratch, sizeof(state->scratch), base_path) !=
        GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed copying base path"));
    }

    char repo_component[RepoIdHexBufferSize];
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

#ifdef gm_strtok_r
#undef gm_strtok_r
#endif
