#include "mged_protocol.h"
#include <string.h>
#include <stdlib.h>

struct command_request *parse_protocol_request(const char *buffer, size_t len) {
    struct command_request *req = (struct command_request *)calloc(1, sizeof(*req));
    if (!req) return NULL;
    
    // Remove trailing FS if present
    size_t parse_len = len;
    if (parse_len > 0 && buffer[parse_len - 1] == PROTOCOL_FS) {
        parse_len--;
    }
    
    if (parse_len == 0) {
        // Empty command
        req->command = strdup("");
        req->argc = 1;
        req->argv = (char **)calloc(2, sizeof(char*));
        if (!req->argv || !req->command) {
            free(req->command);
            free(req->argv);
            free(req);
            return NULL;
        }
        req->argv[0] = strdup("");
        req->argv[1] = NULL;
        return req;
    }
    
    // Count GS separators to determine field count
    int field_count = 1;  // At least the command
    for (size_t i = 0; i < parse_len; i++) {
        if (buffer[i] == PROTOCOL_GS) field_count++;
    }
    
    // Allocate argv (all fields + NULL terminator)
    req->argc = field_count;
    req->argv = (char **)calloc(field_count + 1, sizeof(char*));
    if (!req->argv) {
        free(req);
        return NULL;
    }
    
    // Parse fields separated by GS
    int field_idx = 0;
    const char *field_start = buffer;
    
    for (size_t i = 0; i <= parse_len; i++) {
        if (i == parse_len || buffer[i] == PROTOCOL_GS) {
            size_t field_len = (i < parse_len && buffer[i] == PROTOCOL_GS) ? 
                              (buffer + i - field_start) : (buffer + parse_len - field_start);
            
            req->argv[field_idx] = (char *)malloc(field_len + 1);
            if (!req->argv[field_idx]) {
                // Cleanup on allocation failure
                for (int j = 0; j < field_idx; j++) {
                    free(req->argv[j]);
                }
                free(req->argv);
                free(req);
                return NULL;
            }
            
            memcpy(req->argv[field_idx], field_start, field_len);
            req->argv[field_idx][field_len] = '\0';
            
            // Command is the first field (argv[0])
            if (field_idx == 0) {
                req->command = strdup(req->argv[0]);
                if (!req->command) {
                    for (int j = 0; j <= field_idx; j++) {
                        free(req->argv[j]);
                    }
                    free(req->argv);
                    free(req);
                    return NULL;
                }
            }
            
            field_idx++;
            if (i < parse_len) field_start = buffer + i + 1;
        }
    }
    
    req->argv[field_idx] = NULL;
    return req;
}

void free_command_request(struct command_request *req) {
    if (!req) return;
    
    free(req->command);
    
    if (req->argv) {
        for (int i = 0; i < req->argc; i++) {
            free(req->argv[i]);
        }
        free(req->argv);
    }
    
    free(req);
}

int format_protocol_response(struct bu_vls *response, const char *status, 
                             const char *result, const char *error) {
    bu_vls_trunc(response, 0);
    
    bu_vls_printf(response, "%s", status);
    
    if (result && strlen(result) > 0) {
        bu_vls_printf(response, "%c%s", PROTOCOL_GS, result);
    } else {
        bu_vls_printf(response, "%c", PROTOCOL_GS);
    }
    
    if (error && strlen(error) > 0) {
        bu_vls_printf(response, "%c%s", PROTOCOL_GS, error);
    } else {
        bu_vls_printf(response, "%c", PROTOCOL_GS);
    }
    
    bu_vls_printf(response, "%c", PROTOCOL_FS);
    
    return 0;
}
