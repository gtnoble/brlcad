#ifndef MGED_MOCK_CLIENT_H
#define MGED_MOCK_CLIENT_H

#include <sys/socket.h>
#include <sys/un.h>
#include <bu.h>

// Mock client structure for testing
struct mock_client {
    int sock_fd;
    struct sockaddr_un server_addr;
    struct bu_vls send_buffer;
    struct bu_vls recv_buffer;
};

// Mock response structure
struct mock_response {
    char *status;
    char *result;
    char *error;
};

// Mock client API
int mock_client_init(struct mock_client *client);
int mock_client_connect(struct mock_client *client, const char *socket_path);
int mock_client_send_command(struct mock_client *client, const char *command, 
                           int argc, const char **argv);
int mock_client_receive_response(struct mock_client *client, struct mock_response *response,
                               int timeout_ms);
void mock_client_free_response(struct mock_response *response);
void mock_client_disconnect(struct mock_client *client);
void mock_client_cleanup(struct mock_client *client);

#endif /* MGED_MOCK_CLIENT_H */