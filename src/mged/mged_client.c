#include "mged_client.h"
#include "mged_protocol.h"
#include <stdlib.h>
#include <unistd.h>

struct client_session *create_client_session(int fd) {
    struct client_session *client = (struct client_session *)calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }
    
    client->fd = fd;
    client->state = CLIENT_NEW;
    
    // Initialize buffers
    bu_vls_init(&client->input_buffer);
    bu_vls_init(&client->output_buffer);
    
    // Create GED instance for this client
    client->gedp = ged_create();
    if (!client->gedp) {
        bu_vls_free(&client->input_buffer);
        bu_vls_free(&client->output_buffer);
        free(client);
        return NULL;
    }
    
    return client;
}

void free_client_session(struct client_session *client) {
    if (!client) return;
    
    // Close socket
    if (client->fd >= 0) {
        close(client->fd);
    }
    
    // Free GED instance
    if (client->gedp) {
        ged_close(client->gedp);
    }
    
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
