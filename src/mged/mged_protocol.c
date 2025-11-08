#include "mged_protocol.h"
#include <string.h>
#include <stdlib.h>

struct command_request *parse_protocol_request(const char *buffer, size_t len) {
    struct command_request *req = (struct command_request *)calloc(1, sizeof(*req));
    if (!req) return NULL;
    
    // Find GS separator between command and arguments
    const char *gs_pos = (const char *)memchr(buffer, PROTOCOL_GS, len);
    if (!gs_pos) {
        // No arguments, command is entire buffer (minus FS at end)
        size_t cmd_len = len;
        if (cmd_len > 0 && buffer[cmd_len - 1] == PROTOCOL_FS) {
            cmd_len--;
        }
        
        req->command = (char *)malloc(cmd_len + 1);
        if (!req->command) {
            free(req);
            return NULL;
        }
        memcpy(req->command, buffer, cmd_len);
        req->command[cmd_len] = '\0';
        req->argc = 0;
        req->argv = NULL;
        return req;
    }
    
    // Extract command
    size_t cmd_len = gs_pos - buffer;
    req->command = (char *)malloc(cmd_len + 1);
    if (!req->command) {
        free(req);
        return NULL;
    }
    memcpy(req->command, buffer, cmd_len);
    req->command[cmd_len] = '\0';
    
    // Parse arguments
    const char *args_start = gs_pos + 1;
    size_t args_len = len - (args_start - buffer);
    
    // Remove trailing FS if present
    if (args_len > 0 && args_start[args_len - 1] == PROTOCOL_FS) {
        args_len--;
    }
    
    if (args_len == 0) {
        req->argc = 0;
        req->argv = NULL;
        return req;
    }
    
    // Count arguments
    req->argc = 1;  // At least one argument
    for (const char *p = args_start; p < args_start + args_len; p++) {
        if (*p == PROTOCOL_RS) req->argc++;
    }
    
    // Allocate argv array
    req->argv = (char **)calloc(req->argc + 1, sizeof(char*));
    if (!req->argv) {
        free(req->command);
        free(req);
        return NULL;
    }
    
    // Extract arguments
    int arg_idx = 0;
    const char *arg_start = args_start;
    for (const char *p = args_start; p <= args_start + args_len; p++) {
        if (*p == PROTOCOL_RS || p == args_start + args_len) {
            size_t arg_len = p - arg_start;
            
            req->argv[arg_idx] = (char *)malloc(arg_len + 1);
            if (!req->argv[arg_idx]) {
                // Cleanup on allocation failure
                for (int i = 0; i < arg_idx; i++) {
                    free(req->argv[i]);
                }
                free(req->argv);
                free(req->command);
                free(req);
                return NULL;
            }
            
            memcpy(req->argv[arg_idx], arg_start, arg_len);
            req->argv[arg_idx][arg_len] = '\0';
            arg_idx++;
            arg_start = p + 1;
        }
    }
    
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
