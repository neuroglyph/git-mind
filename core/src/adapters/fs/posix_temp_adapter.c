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
#include "gitmind/util/errno_compat.h"

#ifndef GM_HAVE_REALPATH_DECL
#define GM_HAVE_REALPATH_DECL 1
extern char *realpath(const char *path, char *resolved_path);
#endif

#if defined(_WIN32)
static char *gm_strtok_port(char *str, const char *delim, char **saveptr) {
    return strtok_s(str, delim, saveptr);
}
#else
static char *gm_strtok_port(char *str, const char *delim, char **saveptr) {
    return strtok_r(str, delim, saveptr);
}
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

static void release_error(gm_error_t *error) {
    if (error != NULL) {
        gm_error_free(error);
    }
}

static gm_result_void_t ignore_error_code(gm_result_void_t result,
                                          int32_t code) {
    if (!result.ok && result.u.err != NULL && result.u.err->code == code) {
        gm_error_free(result.u.err);
        return gm_ok_void();
    }
    return result;
}

typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
} gm_path_builder_t;

static void path_builder_init(gm_path_builder_t *builder, char *storage,
                              size_t storage_size, bool absolute) {
    builder->buffer = storage;
    builder->capacity = storage_size;
    if (absolute) {
        builder->buffer[0] = '/';
        builder->buffer[1] = '\0';
        builder->length = 1U;
    } else {
        builder->buffer[0] = '\0';
        builder->length = 0U;
    }
}

static gm_result_void_t path_builder_append(gm_path_builder_t *builder,
                                            const char *segment) {
    if (builder == NULL || segment == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "path builder missing inputs"));
    }

    size_t segment_len = strlen(segment);
    if (segment_len == 0U) {
        return gm_ok_void();
    }

    size_t needs_separator = 0U;
    if (builder->length > 0U && builder->buffer[builder->length - 1U] != '/') {
        needs_separator = 1U;
    }
    size_t separator_len = needs_separator;
    size_t required = builder->length + separator_len + segment_len + 1U;
    if (required > builder->capacity) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "path builder exceeds capacity"));
    }

    if (needs_separator == 1U) {
        builder->buffer[builder->length++] = '/';
    }

    if (gm_memcpy_span(builder->buffer + builder->length,
                       builder->capacity - builder->length, segment,
                       segment_len) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "segment copy exceeded capacity"));
    }

    builder->length += segment_len;
    builder->buffer[builder->length] = '\0';
    return gm_ok_void();
}

static gm_result_void_t ensure_directory_component(const char *path) {
    struct stat path_stat = {0};
    if (stat(path, &path_stat) == 0) {
        if (!S_ISDIR(path_stat.st_mode)) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH,
                                        "path exists but is not a directory"));
        }
        return gm_ok_void();
    }

    int stat_err = errno;
    gm_result_void_t stat_result = gm_errno_to_result("stat", path, stat_err);
    if (!stat_result.ok) {
        if (stat_result.u.err == NULL) {
            return stat_result;
        }
        if (stat_result.u.err->code != GM_ERR_NOT_FOUND) {
            return stat_result;
        }
        release_error(stat_result.u.err);
    }

    if (mkdir(path, DIR_PERMS_NORMAL) == 0) {
        return gm_ok_void();
    }

    int mkdir_err = errno;

    struct stat created = {0};
    if (stat(path, &created) == 0 && S_ISDIR(created.st_mode)) {
        return gm_ok_void();
    }

    return gm_errno_to_result("mkdir", path, mkdir_err);
}

static gm_result_void_t ensure_directory_segment(gm_path_builder_t *builder,
                                                 const char *segment) {
    if (segment == NULL || segment[0] == '\0') {
        return gm_ok_void();
    }

    gm_result_void_t append_result = path_builder_append(builder, segment);
    if (!append_result.ok) {
        return append_result;
    }

    return ensure_directory_component(builder->buffer);
}

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

    if (normalized[0] == '\0' ||
        (normalized[0] == '.' && normalized[1] == '\0') ||
        (normalized[0] == '/' && normalized[1] == '\0')) {
        return gm_ok_void();
    }

    char working[GM_PATH_MAX];
    if (gm_strcpy_safe(working, sizeof(working), normalized) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG,
                     "normalized path exceeds buffer while ensuring dir"));
    }

    bool is_absolute = (working[0] == '/');
    char building[GM_PATH_MAX];
    gm_path_builder_t builder;
    path_builder_init(&builder, building, sizeof(building), is_absolute);

    char *saveptr = NULL;
    for (char *segment = gm_strtok_port(working, "/", &saveptr); segment != NULL;
         segment = gm_strtok_port(NULL, "/", &saveptr)) {
        gm_result_void_t segment_result =
            ensure_directory_segment(&builder, segment);
        if (!segment_result.ok) {
            return segment_result;
        }
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

static gm_result_void_t compose_child_path(char *buffer, size_t buffer_size,
                                           const char *parent,
                                           const char *child) {
    if (buffer == NULL || parent == NULL || child == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "compose child path arguments missing"));
    }

    int wrote = gm_snprintf(buffer, buffer_size, "%s/%s", parent, child);
    if (wrote < 0 || (size_t)wrote >= buffer_size) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "failed to compose child path"));
    }

    return gm_ok_void();
}

static gm_result_void_t try_configure_state_base_from_home(
    gm_posix_fs_state_t *state, bool ensure, bool *configured) {
    if (configured == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "configured flag pointer missing"));
    }

    *configured = false;

    char home[GM_PATH_MAX];
    gm_result_void_t home_result = resolve_home(home, sizeof(home));
    if (!home_result.ok) {
        release_error(home_result.u.err);
        return gm_ok_void();
    }

    size_t home_length = strlen(home);
    if (home_length == 0U) {
        return gm_ok_void();
    }
    if (home_length == 1U && home[0] == '/') {
        return gm_ok_void();
    }

    gm_result_void_t compose_result =
        compose_child_path(state->base_state, sizeof(state->base_state), home,
                           ".gitmind");
    if (!compose_result.ok) {
        return compose_result;
    }

    if (ensure) {
        gm_result_void_t ensure_result = ensure_dir_exists(state->base_state);
        if (!ensure_result.ok) {
            release_error(ensure_result.u.err);
            return gm_ok_void();
        }
    }

    state->base_state_ready = true;
    *configured = true;
    return gm_ok_void();
}

static gm_result_void_t state_base_dir(gm_posix_fs_state_t *state,
                                       bool ensure) {
    if (state->base_state_ready) {
        return gm_ok_void();
    }

    bool configured = false;
    gm_result_void_t home_result =
        try_configure_state_base_from_home(state, ensure, &configured);
    if (!home_result.ok) {
        return home_result;
    }
    if (configured) {
        return gm_ok_void();
    }

    gm_result_void_t temp_result = temp_base_dir(state, ensure);
    if (!temp_result.ok) {
        return temp_result;
    }

    gm_result_void_t compose_result = compose_child_path(
        state->base_state, sizeof(state->base_state), state->base_temp,
        "gitmind-state");
    if (!compose_result.ok) {
        return compose_result;
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
                                      const char *segments[],
                                      size_t segment_count) {
    size_t idx = strlen(buffer);
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

static gm_result_void_t ensure_repo_root(gm_posix_fs_state_t *state,
                                         gm_repo_id_t repo, char *out_root,
                                         size_t out_size) {
    const char *base_state = NULL;
    gm_result_void_t dispatch_result =
        base_dir_dispatch(state, GM_FS_BASE_STATE, true, &base_state);
    if (!dispatch_result.ok) {
        return dispatch_result;
    }

    if (gm_strcpy_safe(out_root, out_size, base_state) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed copying base state"));
    }

    char repo_component[RepoIdHexBufferSize];
    gm_result_void_t format_result =
        format_repo_component(state, repo, repo_component,
                              sizeof(repo_component));
    if (!format_result.ok) {
        return format_result;
    }

    const char *segments[] = {repo_component};
    gm_result_void_t join_result =
        join_segments(out_root, out_size, segments, 1);
    if (!join_result.ok) {
        return join_result;
    }

    return ensure_dir_exists(out_root);
}

static gm_result_void_t copy_path_to_state(gm_posix_fs_state_t *state,
                                           const char *path,
                                           gm_tempdir_t *out_dir) {
    if (gm_strcpy_safe(state->scratch, sizeof(state->scratch), path) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "copying temp dir path failed"));
    }
    if (out_dir != NULL) {
        out_dir->path = state->scratch;
    }
    return gm_ok_void();
}

static gm_result_void_t format_template_base(char *buffer, size_t buffer_size,
                                             const char *directory,
                                             const char *component) {
    int wrote = gm_snprintf(buffer, buffer_size, "%s/%s", directory, component);
    if (wrote < 0 || (size_t)wrote >= buffer_size) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "failed to format temp dir base"));
    }
    return gm_ok_void();
}

static gm_result_void_t format_template_with_suffix(char *buffer,
                                                    size_t buffer_size,
                                                    const char *directory,
                                                    const char *component,
                                                    unsigned int suffix) {
    int wrote = gm_snprintf(buffer, buffer_size, "%s/%s-%06X", directory,
                             component, suffix);
    if (wrote < 0 || (size_t)wrote >= buffer_size) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "failed to format temp dir suffix"));
    }
    return gm_ok_void();
}

static gm_result_void_t create_randomized_temp_dir(
    gm_posix_fs_state_t *state, const char *base_dir, const char *component,
    char *template_path, size_t template_size, gm_tempdir_t *out_dir) {
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();

    for (unsigned int attempt = 0; attempt < TempSuffixAttempts; ++attempt) {
        seed = (seed * TempRandomMultiplier) + TempRandomIncrement;
        unsigned int suffix = seed & TempRandomMask;

        gm_result_void_t format_result = format_template_with_suffix(
            template_path, template_size, base_dir, component, suffix);
        if (!format_result.ok) {
            return format_result;
        }

        if (mkdir(template_path, DIR_PERMS_NORMAL) == 0) {
            gm_result_void_t copy_result =
                copy_path_to_state(state, template_path, out_dir);
            if (!copy_result.ok) {
                (void)rmdir(template_path);
                return copy_result;
            }
            return gm_ok_void();
        }

        int mkdir_err = errno;
        gm_result_void_t mkdir_result =
            gm_errno_to_result("mkdir", template_path, mkdir_err);
        mkdir_result = ignore_error_code(mkdir_result, GM_ERR_ALREADY_EXISTS);
        if (mkdir_result.ok) {
            continue;
        }
        return mkdir_result;
    }

    return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                "unable to allocate temp dir after retries"));
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
    if (component == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "temp dir component missing"));
    }
    if (component[0] == '\0') {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "temp dir component empty"));
    }
    if (has_path_separator(component)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "component must be non-empty without separators"));
    }

    char repo_root[GM_PATH_MAX];
    gm_result_void_t root_result =
        ensure_repo_root(state, repo, repo_root, sizeof(repo_root));
    if (!root_result.ok) {
        return root_result;
    }

    char template_path[GM_PATH_MAX];
    gm_result_void_t format_result =
        format_template_base(template_path, sizeof(template_path), repo_root,
                             component);
    if (!format_result.ok) {
        return format_result;
    }

    if (!suffix_random) {
        gm_result_void_t ensure_result = ensure_dir_exists(template_path);
        if (!ensure_result.ok) {
            return ensure_result;
        }
        return copy_path_to_state(state, template_path, out_dir);
    }

    return create_randomized_temp_dir(state, repo_root, component,
                                      template_path, sizeof(template_path),
                                      out_dir);
}

static bool is_dot_entry(const struct dirent *entry) {
    if (entry == NULL) {
        return false;
    }
    if (strcmp(entry->d_name, ".") == 0) {
        return true;
    }
    if (strcmp(entry->d_name, "..") == 0) {
        return true;
    }
    return false;
}

typedef struct {
    DIR *dir;
    size_t path_len;
} dir_stack_frame_t;

typedef struct {
    dir_stack_frame_t frames[GM_PATH_MAX];
    size_t depth;
    char current[GM_PATH_MAX];
} remove_tree_context_t;

static void remove_tree_cleanup(remove_tree_context_t *ctx) {
    while (ctx->depth > 0U) {
        DIR *dir = ctx->frames[--ctx->depth].dir;
        if (dir != NULL) {
            closedir(dir);
        }
    }
}

static gm_result_void_t remove_tree_context_init(remove_tree_context_t *ctx,
                                                 const char *root_path) {
    ctx->depth = 0U;
    if (gm_strcpy_safe(ctx->current, sizeof(ctx->current), root_path) != GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path exceeds buffer while removing dir"));
    }

    DIR *root_dir = opendir(ctx->current);
    if (root_dir == NULL) {
        int opendir_err = errno;
        return gm_errno_to_result("opendir", ctx->current, opendir_err);
    }

    size_t initial_len = strlen(ctx->current);
    ctx->frames[ctx->depth++] =
        (dir_stack_frame_t){.dir = root_dir, .path_len = initial_len};
    return gm_ok_void();
}

static gm_result_void_t remove_tree_append_entry(remove_tree_context_t *ctx,
                                                 size_t parent_len,
                                                 const char *name,
                                                 size_t *out_length) {
    size_t name_len = strlen(name);
    size_t needs_separator =
        (parent_len > 0U && ctx->current[parent_len - 1U] != '/') ? 1U : 0U;
    size_t required = parent_len + needs_separator + name_len + 1U;
    if (required >= sizeof(ctx->current)) {
        ctx->current[parent_len] = '\0';
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "child path exceeds buffer"));
    }

    size_t cursor = parent_len;
    if (needs_separator == 1U) {
        ctx->current[cursor++] = '/';
    }
    if (gm_memcpy_span(ctx->current + cursor,
                       sizeof(ctx->current) - cursor, name, name_len) != 0) {
        ctx->current[parent_len] = '\0';
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "child path exceeds buffer"));
    }
    cursor += name_len;
    ctx->current[cursor] = '\0';
    if (out_length != NULL) {
        *out_length = cursor;
    }
    return gm_ok_void();
}

static gm_result_void_t remove_tree_descend(remove_tree_context_t *ctx,
                                            size_t path_len) {
    DIR *child_dir = opendir(ctx->current);
    if (child_dir == NULL) {
        int opendir_err = errno;
        return gm_errno_to_result("opendir", ctx->current, opendir_err);
    }
    size_t capacity = sizeof(ctx->frames) / sizeof(ctx->frames[0]);
    if (ctx->depth >= capacity) {
        closedir(child_dir);
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH,
                                    "directory depth exceeds stack capacity"));
    }
    ctx->frames[ctx->depth++] =
        (dir_stack_frame_t){.dir = child_dir, .path_len = path_len};
    return gm_ok_void();
}

static gm_result_void_t remove_tree_remove_file(remove_tree_context_t *ctx,
                                                size_t parent_len) {
    gm_result_void_t result = gm_ok_void();
    if (unlink(ctx->current) != 0) {
        int unlink_err = errno;
        result = gm_errno_to_result("unlink", ctx->current, unlink_err);
        result = ignore_error_code(result, GM_ERR_NOT_FOUND);
    }
    ctx->current[parent_len] = '\0';
    return result;
}

static gm_result_void_t remove_tree_finalize(remove_tree_context_t *ctx) {
    dir_stack_frame_t *frame = &ctx->frames[ctx->depth - 1U];
    ctx->current[frame->path_len] = '\0';
    closedir(frame->dir);
    ctx->depth -= 1U;

    if (rmdir(ctx->current) != 0) {
        int rmdir_err = errno;
        gm_result_void_t rmdir_result =
            gm_errno_to_result("rmdir", ctx->current, rmdir_err);
        rmdir_result = ignore_error_code(rmdir_result, GM_ERR_NOT_FOUND);
        if (!rmdir_result.ok) {
            return rmdir_result;
        }
    }

    if (ctx->depth > 0U) {
        size_t parent_len = ctx->frames[ctx->depth - 1U].path_len;
        ctx->current[parent_len] = '\0';
    }

    return gm_ok_void();
}

static gm_result_void_t remove_tree_step(remove_tree_context_t *ctx) {
    dir_stack_frame_t *frame = &ctx->frames[ctx->depth - 1U];
    ctx->current[frame->path_len] = '\0';

    struct dirent *entry = readdir(frame->dir);
    if (entry == NULL) {
        return remove_tree_finalize(ctx);
    }

    if (is_dot_entry(entry)) {
        return gm_ok_void();
    }

    size_t child_len = 0U;
    gm_result_void_t append_result = remove_tree_append_entry(
        ctx, frame->path_len, entry->d_name, &child_len);
    if (!append_result.ok) {
        return append_result;
    }

    struct stat entry_stat = {0};
    if (stat(ctx->current, &entry_stat) != 0) {
        int stat_err = errno;
        gm_result_void_t stat_result =
            gm_errno_to_result("stat", ctx->current, stat_err);
        stat_result = ignore_error_code(stat_result, GM_ERR_NOT_FOUND);
        if (!stat_result.ok) {
            return stat_result;
        }
        ctx->current[frame->path_len] = '\0';
        return gm_ok_void();
    }

    if (S_ISDIR(entry_stat.st_mode)) {
        return remove_tree_descend(ctx, child_len);
    }

    return remove_tree_remove_file(ctx, frame->path_len);
}

static gm_result_void_t remove_tree_directory(const char *normalized) {
    remove_tree_context_t ctx = {0};
    gm_result_void_t init_result = remove_tree_context_init(&ctx, normalized);
    if (!init_result.ok) {
        return init_result;
    }

    gm_result_void_t step_result = gm_ok_void();
    while (ctx.depth > 0U) {
        step_result = remove_tree_step(&ctx);
        if (!step_result.ok) {
            break;
        }
    }

    if (!step_result.ok) {
        remove_tree_cleanup(&ctx);
        return step_result;
    }

    return gm_ok_void();
}

static gm_result_void_t remove_tree_non_directory(const char *normalized) {
    if (unlink(normalized) == 0) {
        return gm_ok_void();
    }

    int unlink_err = errno;
    gm_result_void_t unlink_result =
        gm_errno_to_result("unlink", normalized, unlink_err);
    return ignore_error_code(unlink_result, GM_ERR_NOT_FOUND);
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
        int stat_err = errno;
        gm_result_void_t stat_result =
            gm_errno_to_result("stat", normalized, stat_err);
        stat_result = ignore_error_code(stat_result, GM_ERR_NOT_FOUND);
        return stat_result;
    }

    if (!S_ISDIR(root_stat.st_mode)) {
        return remove_tree_non_directory(normalized);
    }

    return remove_tree_directory(normalized);
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
    GM_TRY(join_segments(state->scratch, sizeof(state->scratch), path_segments,
                         sizeof(path_segments) / sizeof(path_segments[0])));

    *out_abs_path = state->scratch;
    return gm_ok_void();
}

static gm_result_void_t canonicalize_existing_path(gm_posix_fs_state_t *state,
                                                   const char *abs_path_in,
                                                   const char **out_abs_path) {
    if (realpath(abs_path_in, state->scratch) == NULL) {
        int realpath_err = errno;
        return gm_errno_to_result("realpath", abs_path_in, realpath_err);
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
        int realpath_err = errno;
        return gm_errno_to_result("realpath", parent_path, realpath_err);
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
