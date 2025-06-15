// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Execute command safely without shell interpretation
int gm_exec_git_command(const char* git_args[], char* output, size_t output_size) {
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) == -1) {
        gm_set_error("Failed to create pipe");
        return GM_ERR_IO;
    }
    
    pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        gm_set_error("Failed to fork");
        return GM_ERR_IO;
    }
    
    if (pid == 0) {
        // Child process
        close(pipefd[0]);  // Close read end
        
        // Redirect stdout to pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            exit(1);
        }
        close(pipefd[1]);
        
        // Redirect stderr to /dev/null
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        
        // Execute git command
        execvp("git", (char *const *)git_args);
        exit(1);  // execvp failed
    }
    
    // Parent process
    close(pipefd[1]);  // Close write end
    
    // Read output
    ssize_t total_read = 0;
    ssize_t bytes_read;
    char* output_ptr = output;
    size_t remaining = output_size - 1;  // Leave room for null terminator
    
    while (remaining > 0 && 
           (bytes_read = read(pipefd[0], output_ptr, remaining)) > 0) {
        total_read += bytes_read;
        output_ptr += bytes_read;
        remaining -= bytes_read;
    }
    close(pipefd[0]);
    
    // Null terminate
    if (output && output_size > 0) {
        output[total_read] = '\0';
        
        // Remove trailing newline if present
        if (total_read > 0 && output[total_read - 1] == '\n') {
            output[total_read - 1] = '\0';
        }
    }
    
    // Wait for child
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        gm_set_error("Failed to wait for child process");
        return GM_ERR_IO;
    }
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        gm_set_error("Git command failed");
        return GM_ERR_GIT;
    }
    
    return GM_OK;
}

// Hash object safely
int gm_git_hash_object(const void* data, size_t size, const char* type, char* out_sha) {
    int pipefd_in[2], pipefd_out[2];
    pid_t pid;
    
    if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
        gm_set_error("Failed to create pipes");
        return GM_ERR_IO;
    }
    
    pid = fork();
    if (pid == -1) {
        close(pipefd_in[0]);
        close(pipefd_in[1]);
        close(pipefd_out[0]);
        close(pipefd_out[1]);
        gm_set_error("Failed to fork");
        return GM_ERR_IO;
    }
    
    if (pid == 0) {
        // Child process
        close(pipefd_in[1]);   // Close write end of input
        close(pipefd_out[0]);  // Close read end of output
        
        // Redirect stdin from pipe
        if (dup2(pipefd_in[0], STDIN_FILENO) == -1) exit(1);
        close(pipefd_in[0]);
        
        // Redirect stdout to pipe
        if (dup2(pipefd_out[1], STDOUT_FILENO) == -1) exit(1);
        close(pipefd_out[1]);
        
        // Execute git hash-object
        const char* args[] = {"git", "hash-object", "-w", "--stdin", "-t", type, NULL};
        execvp("git", (char *const *)args);
        exit(1);
    }
    
    // Parent process
    close(pipefd_in[0]);   // Close read end of input
    close(pipefd_out[1]);  // Close write end of output
    
    // Write data to git
    ssize_t written = 0;
    while (written < (ssize_t)size) {
        ssize_t w = write(pipefd_in[1], (char*)data + written, size - written);
        if (w <= 0) break;
        written += w;
    }
    close(pipefd_in[1]);
    
    // Read SHA from git
    char sha_buf[41];
    ssize_t bytes_read = read(pipefd_out[0], sha_buf, 40);
    close(pipefd_out[0]);
    
    // Wait for child
    int status;
    waitpid(pid, &status, 0);
    
    if (bytes_read != 40 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        gm_set_error("Failed to hash object");
        return GM_ERR_GIT;
    }
    
    sha_buf[40] = '\0';
    strcpy(out_sha, sha_buf);
    
    return GM_OK;
}

// Read blob content safely
int gm_git_cat_file_blob(const char* sha, void* output, size_t max_size, size_t* actual_size) {
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) == -1) {
        gm_set_error("Failed to create pipe");
        return GM_ERR_IO;
    }
    
    pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        gm_set_error("Failed to fork");
        return GM_ERR_IO;
    }
    
    if (pid == 0) {
        // Child process
        close(pipefd[0]);  // Close read end
        
        // Redirect stdout to pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) exit(1);
        close(pipefd[1]);
        
        // Redirect stderr to /dev/null
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        
        // Execute git cat-file
        const char* args[] = {"git", "cat-file", "blob", sha, NULL};
        execvp("git", (char *const *)args);
        exit(1);
    }
    
    // Parent process
    close(pipefd[1]);  // Close write end
    
    // Read blob content
    ssize_t total_read = 0;
    ssize_t bytes_read;
    char* output_ptr = (char*)output;
    
    while (total_read < (ssize_t)max_size && 
           (bytes_read = read(pipefd[0], output_ptr + total_read, 
                             max_size - total_read)) > 0) {
        total_read += bytes_read;
    }
    close(pipefd[0]);
    
    // Wait for child
    int status;
    waitpid(pid, &status, 0);
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        gm_set_error("Failed to read blob");
        return GM_ERR_GIT;
    }
    
    if (actual_size) {
        *actual_size = total_read;
    }
    
    return GM_OK;
}