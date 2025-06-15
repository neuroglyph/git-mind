// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Create a link using the holy architecture
int gm_link_create_v2(const char* source, const char* target, 
                     const char* type, float confidence) {
    if (!source || !target || !type) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Ensure orphan ref exists
    int ret = gm_orphan_ref_create();
    if (ret != GM_OK) return ret;
    
    // Get blob SHAs
    char src_sha[41], tgt_sha[41];
    ret = gm_path_to_sha(source, src_sha);
    if (ret != GM_OK) return ret;
    
    ret = gm_path_to_sha(target, tgt_sha);
    if (ret != GM_OK) return ret;
    
    // Generate edge ID
    char edge_id[GM_ULID_SIZE];
    ret = gm_ulid_generate(edge_id);
    if (ret != GM_OK) return ret;
    
    // Encode edge to CBOR
    unsigned char cbor_buf[256];
    size_t cbor_len;
    ret = gm_cbor_encode_edge(tgt_sha, confidence, time(NULL), 
                             cbor_buf, &cbor_len, sizeof(cbor_buf));
    if (ret != GM_OK) return ret;
    
    // Write CBOR blob using safe function
    char blob_sha[41];
    ret = gm_git_hash_object(cbor_buf, cbor_len, "blob", blob_sha);
    if (ret != GM_OK) return ret;
    
    // Build edge path
    char edge_path[GM_MAX_PATH];
    ret = gm_build_edge_path(src_sha, type, edge_id, edge_path, sizeof(edge_path));
    if (ret != GM_OK) return ret;
    
    // Get current graph tree
    char current_tree[41];
    ret = gm_get_graph_tree(current_tree);
    if (ret != GM_OK) {
        current_tree[0] = '\0';  // Empty tree
    }
    
    // Build new tree with edge
    char new_tree[41];
    ret = gm_build_edge_tree(edge_path, blob_sha, current_tree, new_tree);
    if (ret != GM_OK) return ret;
    
    // Update graph ref
    char message[256];
    snprintf(message, sizeof(message), "Add edge: %s -[%s]-> %s", 
             source, type, target);
    
    ret = gm_update_graph_ref(new_tree, message);
    if (ret != GM_OK) return ret;
    
    return GM_OK;
}

// List outgoing edges
int gm_link_list_v2(const char* source_path, gm_link_set_t** out_set) {
    if (!source_path || !out_set) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Get source SHA
    char src_sha[41];
    int ret = gm_path_to_sha(source_path, src_sha);
    if (ret != GM_OK) return ret;
    
    // Get graph tree
    char graph_tree[41];
    ret = gm_get_graph_tree(graph_tree);
    if (ret == GM_ERR_NOT_FOUND) {
        // No graph yet
        *out_set = gm_link_set_new();
        return GM_OK;
    }
    if (ret != GM_OK) return ret;
    
    // Compute source fan-out
    char src_fanout[6];
    gm_compute_fanout(src_sha, src_fanout, sizeof(src_fanout));
    
    // List edges under source
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), 
             "git ls-tree -r %s:%s/%s/ 2>/dev/null | grep '\\.cbor$'",
             graph_tree, src_fanout, src_sha);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        *out_set = gm_link_set_new();
        return GM_OK;
    }
    
    *out_set = gm_link_set_new();
    if (!*out_set) {
        pclose(fp);
        return GM_ERR_MEMORY;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Parse: "100644 blob <sha>\t<path>"
        char* blob_start = strstr(line, "blob ");
        if (!blob_start) continue;
        blob_start += 5;
        
        char blob_sha[41];
        strncpy(blob_sha, blob_start, 40);
        blob_sha[40] = '\0';
        
        // Extract relationship type from path
        char* path_start = strchr(blob_start, '\t');
        if (!path_start) continue;
        path_start++;
        
        // Path format: src_fanout/src_sha/rel_hash/edge_fanout/edge_id.cbor
        // We want the rel_hash part
        char* rel_start = strstr(path_start, src_sha);
        if (!rel_start) continue;
        rel_start += strlen(src_sha) + 1;  // Skip SHA and /
        
        char rel_hash[9];
        strncpy(rel_hash, rel_start, 8);
        rel_hash[8] = '\0';
        
        // TODO: Reverse lookup rel_hash to type name
        // For now, use the hash as the type
        
        // Read CBOR blob to get target using safe function
        unsigned char cbor_data[256];
        size_t cbor_len;
        
        if (gm_git_cat_file_blob(blob_sha, cbor_data, sizeof(cbor_data), &cbor_len) != GM_OK) {
            continue;
        }
        
        // Decode CBOR to get target SHA
        char target_sha[41];
        float conf;
        time_t ts;
        
        if (gm_cbor_decode_edge(cbor_data, cbor_len, target_sha, &conf, &ts) == GM_OK) {
            // Add to link set
            gm_link_t link;
            strncpy(link.type, rel_hash, sizeof(link.type) - 1);  // TODO: map to name
            strncpy(link.source, source_path, sizeof(link.source) - 1);
            strncpy(link.target, target_sha, sizeof(link.target) - 1);  // TODO: map to path
            link.timestamp = ts;
            
            gm_link_set_add(*out_set, &link);
        }
    }
    
    pclose(fp);
    return GM_OK;
}