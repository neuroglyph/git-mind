/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>

// Store a type mapping in git notes
int gm_type_mapping_store(git_repository *repo, const char *hash, const char *type) {
    git_signature *sig = NULL;
    git_oid hash_oid;
    int ret = GM_OK;
    
    if (!repo || !hash || !type) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Convert hash string to OID (using first 8 chars padded with zeros)
    char padded_hash[41] = {0};
    strncpy(padded_hash, hash, 8);
    // Pad with zeros to make a valid SHA1
    for (int i = 8; i < 40; i++) {
        padded_hash[i] = '0';
    }
    
    if (git_oid_fromstr(&hash_oid, padded_hash) < 0) {
        gm_set_error("Invalid hash format");
        return GM_ERR_INVALID_ARG;
    }
    
    // Get signature
    ret = gm_git_backend_signature_default(repo, &sig);
    if (ret != GM_OK) {
        return ret;
    }
    
    // Create or update note
    if (git_note_create(NULL, repo, GM_NOTES_TYPES_REF, sig, sig, &hash_oid, type, 1) < 0) {
        const git_error *e = git_error_last();
        gm_set_error("Failed to create type note: %s", e ? e->message : "unknown error");
        ret = GM_ERR_GIT;
    }
    
    git_signature_free(sig);
    return ret;
}

// Get type name from hash using notes
int gm_type_mapping_get(git_repository *repo, const char *hash, char *out_type, size_t type_size) {
    git_note *note = NULL;
    git_oid hash_oid;
    
    if (!repo || !hash || !out_type || type_size == 0) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Convert hash string to OID
    char padded_hash[41] = {0};
    strncpy(padded_hash, hash, 8);
    for (int i = 8; i < 40; i++) {
        padded_hash[i] = '0';
    }
    
    if (git_oid_fromstr(&hash_oid, padded_hash) < 0) {
        strncpy(out_type, hash, type_size - 1);
        out_type[type_size - 1] = '\0';
        return GM_OK;
    }
    
    // Read note
    if (git_note_read(&note, repo, GM_NOTES_TYPES_REF, &hash_oid) < 0) {
        // No note found, use hash as type
        strncpy(out_type, hash, type_size - 1);
        out_type[type_size - 1] = '\0';
        return GM_OK;
    }
    
    const char *msg = git_note_message(note);
    if (msg) {
        strncpy(out_type, msg, type_size - 1);
        out_type[type_size - 1] = '\0';
        // Remove newline if present
        char *nl = strchr(out_type, '\n');
        if (nl) *nl = '\0';
    } else {
        strncpy(out_type, hash, type_size - 1);
        out_type[type_size - 1] = '\0';
    }
    
    git_note_free(note);
    return GM_OK;
}
