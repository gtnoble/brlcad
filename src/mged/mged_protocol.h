#ifndef MGED_PROTOCOL_H
#define MGED_PROTOCOL_H

#include "bu.h"
#include <stddef.h>

// Forward declarations for bu_vls
struct bu_vls;

// Protocol control characters
#define PROTOCOL_FS '\x1C'  // File Separator
#define PROTOCOL_GS '\x1D'  // Group Separator
#define PROTOCOL_RS '\x1E'  // Record Separator

// Command request structure
struct command_request {
    char *command;
    int argc;
    char **argv;
};

// Protocol API
struct command_request *parse_protocol_request(const char *buffer, size_t len);
void free_command_request(struct command_request *req);
int format_protocol_response(struct bu_vls *response, const char *status, 
                             const char *result, const char *error);

#endif /* MGED_PROTOCOL_H */
