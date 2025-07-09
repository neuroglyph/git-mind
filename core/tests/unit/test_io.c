/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include <gitmind/io/io.h>
#include <gitmind/error.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Test constants */
static const char *TEST_FILE = "test_io_file.tmp";
static const char *TEST_FILE2 = "test_io_file2.tmp";
static const char *TEST_DIR = "test_io_dir.tmp";
static const char *TEST_SYMLINK = "test_io_symlink.tmp";
static const char *TEST_CONTENT = "Hello, I/O World!";
static const mode_t TEST_DIR_MODE = 0755;

/* Cleanup helper */
static void cleanup_test_files(void) {
    remove(TEST_FILE);
    remove(TEST_FILE2);
    remove(TEST_SYMLINK);
    rmdir(TEST_DIR);
}

/* Test file operations */
static void test_file_operations(void) {
    printf("test_file_operations... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    assert(io != NULL);
    assert(io->file != NULL);
    
    /* Test fopen/fclose */
    gm_result_file_ptr_t open_result = io->file->fopen(TEST_FILE, "w");
    assert(open_result.ok);
    assert(open_result.u.val != NULL);
    
    FILE *file = open_result.u.val;
    
    /* Test fwrite */
    size_t content_len = strlen(TEST_CONTENT);
    gm_result_size_t write_result = io->file->fwrite(TEST_CONTENT, 1, content_len, file);
    assert(write_result.ok);
    assert(write_result.u.val == content_len);
    
    /* Test fprintf */
    gm_result_int_t fprintf_result = io->file->fprintf(file, "\nFormatted: %d", 42);
    assert(fprintf_result.ok);
    assert(fprintf_result.u.val > 0);
    
    /* Test fflush */
    gm_result_void_t flush_result = io->file->fflush(file);
    assert(flush_result.ok);
    
    /* Test fclose */
    gm_result_void_t close_result = io->file->fclose(file);
    assert(close_result.ok);
    
    /* Test fopen for reading */
    open_result = io->file->fopen(TEST_FILE, "r");
    assert(open_result.ok);
    file = open_result.u.val;
    
    /* Test fread */
    char buffer[256] = {0};
    gm_result_size_t read_result = io->file->fread(buffer, 1, content_len, file);
    assert(read_result.ok);
    assert(read_result.u.val == content_len);
    assert(strncmp(buffer, TEST_CONTENT, content_len) == 0);
    
    close_result = io->file->fclose(file);
    assert(close_result.ok);
    
    /* Test rename */
    gm_result_void_t rename_result = io->file->rename(TEST_FILE, TEST_FILE2);
    assert(rename_result.ok);
    
    /* Test remove */
    gm_result_void_t remove_result = io->file->remove(TEST_FILE2);
    assert(remove_result.ok);
    
    /* Test error cases */
    open_result = io->file->fopen("/nonexistent/path/file.txt", "r");
    assert(!open_result.ok);
    assert(open_result.u.err != NULL);
    gm_error_free(open_result.u.err);
    
    printf("OK\n");
}

/* Test directory operations */
static void test_dir_operations(void) {
    printf("test_dir_operations... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    assert(io->dir != NULL);
    
    /* Test mkdir */
    gm_result_void_t mkdir_result = io->dir->mkdir(TEST_DIR, TEST_DIR_MODE);
    assert(mkdir_result.ok);
    
    /* Test getcwd */
    char cwd_buffer[1024];
    gm_result_string_t getcwd_result = io->dir->getcwd(cwd_buffer, sizeof(cwd_buffer));
    assert(getcwd_result.ok);
    assert(getcwd_result.u.val != NULL);
    assert(strlen(getcwd_result.u.val) > 0);
    
    /* Test chdir */
    gm_result_void_t chdir_result = io->dir->chdir(TEST_DIR);
    assert(chdir_result.ok);
    
    /* Change back */
    chdir_result = io->dir->chdir("..");
    assert(chdir_result.ok);
    
    /* Test rmdir */
    gm_result_void_t rmdir_result = io->dir->rmdir(TEST_DIR);
    assert(rmdir_result.ok);
    
    /* Test error cases */
    mkdir_result = io->dir->mkdir("/proc/invalid_path", TEST_DIR_MODE);
    assert(!mkdir_result.ok);
    gm_error_free(mkdir_result.u.err);
    
    printf("OK\n");
}

/* Test file system operations */
static void test_fs_operations(void) {
    printf("test_fs_operations... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    assert(io->fs != NULL);
    
    /* Create a test file */
    FILE *file = fopen(TEST_FILE, "w");
    assert(file != NULL);
    fwrite(TEST_CONTENT, 1, strlen(TEST_CONTENT), file);
    fclose(file);
    
    /* Test stat */
    struct stat st;
    gm_result_void_t stat_result = io->fs->stat(TEST_FILE, &st);
    assert(stat_result.ok);
    assert(S_ISREG(st.st_mode));
    assert(st.st_size == (off_t)strlen(TEST_CONTENT));
    
    /* Test lstat (same as stat for regular files) */
    gm_result_void_t lstat_result = io->fs->lstat(TEST_FILE, &st);
    assert(lstat_result.ok);
    
    /* Test access */
    gm_result_void_t access_result = io->fs->access(TEST_FILE, F_OK);
    assert(access_result.ok);
    
    access_result = io->fs->access(TEST_FILE, R_OK);
    assert(access_result.ok);
    
    /* Test chmod */
    gm_result_void_t chmod_result = io->fs->chmod(TEST_FILE, 0600);
    assert(chmod_result.ok);
    
    /* Test symlink */
    gm_result_void_t symlink_result = io->fs->symlink(TEST_FILE, TEST_SYMLINK);
    assert(symlink_result.ok);
    
    /* Test readlink */
    char link_buffer[256];
    gm_result_ssize_t readlink_result = io->fs->readlink(TEST_SYMLINK, link_buffer, sizeof(link_buffer));
    assert(readlink_result.ok);
    assert(readlink_result.u.val > 0);
    link_buffer[readlink_result.u.val] = '\0';
    assert(strcmp(link_buffer, TEST_FILE) == 0);
    
    /* Test unlink */
    gm_result_void_t unlink_result = io->fs->unlink(TEST_SYMLINK);
    assert(unlink_result.ok);
    
    unlink_result = io->fs->unlink(TEST_FILE);
    assert(unlink_result.ok);
    
    /* Test error cases */
    stat_result = io->fs->stat("/nonexistent/file", &st);
    assert(!stat_result.ok);
    gm_error_free(stat_result.u.err);
    
    printf("OK\n");
}

/* Test process operations */
static void test_process_operations(void) {
    printf("test_process_operations... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    assert(io->process != NULL);
    
    /* Test system */
    gm_result_int_t system_result = io->process->system("true");
    assert(system_result.ok);
    assert(system_result.u.val == 0);
    
    /* Test fork and waitpid */
    gm_result_pid_t fork_result = io->process->fork();
    assert(fork_result.ok);
    
    if (fork_result.u.val == 0) {
        /* Child process */
        _exit(0);
    } else {
        /* Parent process */
        int status;
        gm_result_pid_t wait_result = io->process->waitpid(fork_result.u.val, &status, 0);
        assert(wait_result.ok);
        assert(wait_result.u.val == fork_result.u.val);
        assert(WIFEXITED(status));
        assert(WEXITSTATUS(status) == 0);
    }
    
    printf("OK\n");
}

/* Test error handling */
static void test_error_handling(void) {
    printf("test_error_handling... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    
    /* Test file errors */
    gm_result_file_ptr_t open_result = io->file->fopen("/dev/null/impossible", "r");
    assert(!open_result.ok);
    assert(open_result.u.err != NULL);
    assert(open_result.u.err->code == 1001); /* GM_ERROR_FILE_OPERATION */
    gm_error_free(open_result.u.err);
    
    /* Test directory errors */
    gm_result_void_t mkdir_result = io->dir->mkdir("/dev/null/impossible", 0755);
    assert(!mkdir_result.ok);
    assert(mkdir_result.u.err != NULL);
    assert(mkdir_result.u.err->code == 2001); /* GM_ERROR_DIR_OPERATION */
    gm_error_free(mkdir_result.u.err);
    
    /* Test filesystem errors */
    struct stat st;
    gm_result_void_t stat_result = io->fs->stat("/nonexistent/path", &st);
    assert(!stat_result.ok);
    assert(stat_result.u.err != NULL);
    assert(stat_result.u.err->code == 3001); /* GM_ERROR_FS_OPERATION */
    gm_error_free(stat_result.u.err);
    
    printf("OK\n");
}

/* Test default operations availability */
static void test_default_operations(void) {
    printf("test_default_operations... ");
    
    const gm_io_ops_t *io = gm_io_ops_default();
    assert(io != NULL);
    
    /* Check all operation groups are available */
    assert(io->file != NULL);
    assert(io->dir != NULL);
    assert(io->fs != NULL);
    assert(io->process != NULL);
    
    /* Check file operations */
    assert(io->file->fopen != NULL);
    assert(io->file->fclose != NULL);
    assert(io->file->fread != NULL);
    assert(io->file->fwrite != NULL);
    assert(io->file->fprintf != NULL);
    assert(io->file->fflush != NULL);
    assert(io->file->remove != NULL);
    assert(io->file->rename != NULL);
    
    /* Check directory operations */
    assert(io->dir->mkdir != NULL);
    assert(io->dir->rmdir != NULL);
    assert(io->dir->chdir != NULL);
    assert(io->dir->getcwd != NULL);
    
    /* Check filesystem operations */
    assert(io->fs->stat != NULL);
    assert(io->fs->lstat != NULL);
    assert(io->fs->access != NULL);
    assert(io->fs->chmod != NULL);
    assert(io->fs->unlink != NULL);
    assert(io->fs->readlink != NULL);
    assert(io->fs->symlink != NULL);
    
    /* Check process operations */
    assert(io->process->system != NULL);
    assert(io->process->fork != NULL);
    assert(io->process->execvp != NULL);
    assert(io->process->waitpid != NULL);
    
    printf("OK\n");
}

int main(void) {
    printf("Running I/O operation tests...\n");
    
    /* Clean up any leftover test files */
    cleanup_test_files();
    
    /* Run tests */
    test_default_operations();
    test_file_operations();
    test_dir_operations();
    test_fs_operations();
    test_process_operations();
    test_error_handling();
    
    /* Clean up */
    cleanup_test_files();
    
    printf("All tests passed!\n");
    return 0;
}
