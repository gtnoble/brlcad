#ifndef MGED_SERVER_H
#define MGED_SERVER_H

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include "ged.h"
#include "bu.h"

#define MGED_SERVER_MAX_CLIENTS 256

// Server state structure
struct mged_server {
    int server_fd;
    struct sockaddr_un server_addr;
    char socket_path[256];
    int running;
    
    // Poll array for connections
    struct pollfd *poll_fds;
    int max_fds;
    int num_fds;
    
    // Client sessions
    struct client_session **client_sessions;
    int max_clients;
};

// Forward declaration
struct client_session;

// Server API
int mged_server_init(struct mged_server *server, const char *socket_path);
int mged_server_start(struct mged_server *server);
int mged_server_stop(struct mged_server *server);
void mged_server_cleanup(struct mged_server *server);
int mged_server_poll(struct mged_server *server, int timeout_ms);

// Client management
int accept_new_client(struct mged_server *server);
void close_client_connection(struct mged_server *server, int poll_index);
void handle_client_read(struct mged_server *server, int poll_index);
void handle_client_write(struct mged_server *server, int poll_index);

#endif /* MGED_SERVER_H */
