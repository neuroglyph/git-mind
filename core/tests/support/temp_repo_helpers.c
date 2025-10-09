/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "temp_repo_helpers.h"

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define gm_test_getcwd _getcwd
#else
#include <unistd.h>
#define gm_test_getcwd getcwd
#endif

#include "gitmind/error.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"
#include "gitmind/security/string.h"

#ifndef GM_TEST_TEMPLATE_BUFFER_SIZE
#define GM_TEST_TEMPLATE_BUFFER_SIZE GM_PATH_MAX
#endif

static const char *gm_test_template_root(void) {
    const char *root = getenv("GM_TEST_TEMPLATE_ROOT");
    if (root == NULL || root[0] == '\0') {
        return NULL;
    }
    return root;
}

static bool gm_test_is_dir(const char *path) {
    struct stat st;
    if (path == NULL) return false;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static gm_result_void_t gm_test_ensure_dir(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "directory path missing"));
    }
    if (gm_test_is_dir(path)) {
        return gm_ok_void();
    }
    if (mkdir(path, 0777) == 0) {
        return gm_ok_void();
    }
    if (errno == ENOENT) {
        char parent[GM_TEST_TEMPLATE_BUFFER_SIZE];
        if (gm_strcpy_safe(parent, sizeof(parent), path) != GM_OK) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "template dir path too long"));
        }
        char *slash = strrchr(parent, '/');
        if (slash != NULL) {
            *slash = '\0';
            GM_TRY(gm_test_ensure_dir(parent));
        }
        if (mkdir(path, 0777) == 0 || errno == EEXIST) {
            return gm_ok_void();
        }
    }
    if (errno == EEXIST) {
        return gm_ok_void();
    }
    return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                "failed creating template dir %s", path));
}

static gm_result_void_t gm_test_copy_file(const char *src, const char *dst,
                                          mode_t mode) {
    int in_fd = open(src, O_RDONLY);
    if (in_fd < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "failed opening %s", src));
    }
    int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (out_fd < 0) {
        close(in_fd);
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "failed opening %s", dst));
    }
    char buffer[4096];
    ssize_t read_bytes;
    while ((read_bytes = read(in_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t offset = 0;
        while (offset < read_bytes) {
            ssize_t wrote = write(out_fd, buffer + offset,
                                  (size_t)(read_bytes - offset));
            if (wrote <= 0) {
                close(in_fd);
                close(out_fd);
                return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                            "failed writing %s", dst));
            }
            offset += wrote;
        }
    }
    if (read_bytes < 0) {
        close(in_fd);
        close(out_fd);
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "failed reading %s", src));
    }
    close(in_fd);
    close(out_fd);
    return gm_ok_void();
}

static gm_result_void_t gm_test_copy_tree(const char *src_root,
                                          const char *dst_root) {
    DIR *dir = opendir(src_root);
    if (dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "failed opening template dir %s", src_root));
    }
    GM_TRY(gm_test_ensure_dir(dst_root));
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char src_path[GM_TEST_TEMPLATE_BUFFER_SIZE];
        char dst_path[GM_TEST_TEMPLATE_BUFFER_SIZE];
        if (gm_snprintf(src_path, sizeof(src_path), "%s/%s", src_root,
                        entry->d_name) < 0) {
            closedir(dir);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "template src path too long"));
        }
        if (gm_snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_root,
                        entry->d_name) < 0) {
            closedir(dir);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "template dst path too long"));
        }
        struct stat st;
        if (stat(src_path, &st) != 0) {
            closedir(dir);
            return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                        "failed stating %s", src_path));
        }
        if (S_ISDIR(st.st_mode)) {
            GM_TRY(gm_test_copy_tree(src_path, dst_path));
        } else if (S_ISREG(st.st_mode)) {
            GM_TRY(gm_test_copy_file(src_path, dst_path, st.st_mode & 0777));
        } else {
            closedir(dir);
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "unsupported template entry %s", src_path));
        }
    }
    closedir(dir);
    return gm_ok_void();
}
GM_NODISCARD static gm_result_void_t gm_copy_temp_path(const gm_tempdir_t *temp_dir,
                                                       char *out_path,
                                                       size_t out_size) {
    if (temp_dir == NULL || temp_dir->path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE, "temp dir missing path"));
    }
    if (gm_strcpy_safe(out_path, out_size, temp_dir->path) != GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "temp dir path exceeds buffer size"));
    }
    return gm_ok_void();
}

gm_test_temp_repo_provider_fn gm_test_temp_repo_provider_storage = NULL;

static gm_result_void_t ensure_provider_initialized(void) {
    if (gm_test_temp_repo_provider_storage == NULL) {
        gm_test_temp_repo_provider_storage = gm_test_default_temp_repo_provider;
    }
    return gm_ok_void();
}

gm_result_void_t gm_test_default_temp_repo_provider(const gm_fs_temp_port_t *port,
                                                     const char *component,
                                                     char *out_path,
                                                     size_t out_size) {
    if (port == NULL || component == NULL || component[0] == '\0' ||
        out_path == NULL || out_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "temp repo helper called with invalid arguments"));
    }

    char cwd[GM_PATH_MAX];
    if (gm_test_getcwd(cwd, sizeof(cwd)) == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "getcwd failed: %s", strerror(errno)));
    }

    gm_repo_id_t repo_id = {0};
    gm_result_void_t repo_rc = gm_repo_id_from_path(cwd, &repo_id);
    if (!repo_rc.ok) {
        return repo_rc;
    }

    gm_tempdir_t temp_dir = {0};
    gm_result_void_t temp_rc = gm_fs_temp_port_make_temp_dir(port, repo_id,
                                                             component, true,
                                                             &temp_dir);
    if (!temp_rc.ok) {
        return temp_rc;
    }
    GM_TRY(gm_copy_temp_path(&temp_dir, out_path, out_size));

    const char *template_root = gm_test_template_root();
    if (template_root != NULL) {
        char template_path[GM_TEST_TEMPLATE_BUFFER_SIZE];
        if (gm_snprintf(template_path, sizeof(template_path), "%s/%s",
                        template_root, component) >= 0 &&
            gm_test_is_dir(template_path)) {
            gm_result_void_t copy_result = gm_test_copy_tree(template_path, out_path);
            if (!copy_result.ok) {
                return copy_result;
            }
        }
    }
    return gm_ok_void();
}

void gm_test_set_temp_repo_dir_provider(gm_test_temp_repo_provider_fn provider) {
    gm_test_temp_repo_provider_storage =
        (provider != NULL) ? provider : gm_test_default_temp_repo_provider;
}

gm_result_void_t gm_test_make_temp_repo_dir(const gm_fs_temp_port_t *port,
                                             const char *component,
                                             char *out_path,
                                             size_t out_size) {
    GM_TRY(ensure_provider_initialized());
    return gm_test_temp_repo_provider_storage(port, component, out_path, out_size);
}

gm_result_void_t gm_test_cleanup_temp_repo_dir(const gm_fs_temp_port_t *port,
                                                const char *path) {
    if (port == NULL || path == NULL || path[0] == '\0') {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cleanup requires port and path"));
    }
    return gm_fs_temp_port_remove_tree(port, path);
}
