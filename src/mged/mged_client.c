#include "mged_client.h"
#include "mged_protocol.h"
#include "mged.h"
#include <stdlib.h>
#include <unistd.h>

struct client_session *create_client_session(int fd) {
    // Validate file descriptor
    if (fd < 0) {
        bu_log("ERROR: Should not create client with invalid fd\n");
        return NULL;
    }
    
    struct client_session *client = (struct client_session *)calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }
    
    client->fd = fd;
    client->state = CLIENT_NEW;
    
    // Initialize buffers
    bu_vls_init(&client->input_buffer);
    bu_vls_init(&client->output_buffer);
    
    // Note: Clients use the shared MGED_STATE->gedp for command execution
    // No per-client database instance created here
    
    return client;
}

void free_client_session(struct client_session *client) {
    if (!client) return;
    
    // Close socket
    if (client->fd >= 0) {
        close(client->fd);
    }
    
    // Don't free GED instance - it's shared with main MGED
    // The client just references the main MGED_STATE->gedp
    
    // Free buffers
    bu_vls_free(&client->input_buffer);
    bu_vls_free(&client->output_buffer);
    
    free(client);
}

void reset_client_buffers(struct client_session *client) {
    if (!client) return;
    
    bu_vls_trunc(&client->input_buffer, 0);
    bu_vls_trunc(&client->output_buffer, 0);
}
