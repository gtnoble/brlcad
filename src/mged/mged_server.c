#include "bu.h"
#include "mged.h"
#include "mged_server.h"
#include "mged_client.h"
#include "mged_protocol.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

// Forward declaration
static void update_poll_events(struct mged_server *server, int fd, int events);

int mged_server_init(struct mged_server *server, const char *socket_path) {
    if (!server || !socket_path) {
        return -1;
    }
    
    // Initialize server structure
    memset(server, 0, sizeof(*server));
    // Set socket path after memset to avoid clearing it
    strncpy(server->socket_path, socket_path, sizeof(server->socket_path) - 1);
    server->socket_path[sizeof(server->socket_path) - 1] = '\0';  // Ensure null termination
    
    // Create Unix domain socket
    server->server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        bu_log("Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Set up server address
    memset(&server->server_addr, 0, sizeof(server->server_addr));
    server->server_addr.sun_family = AF_UNIX;
    strncpy(server->server_addr.sun_path, server->socket_path, 
             sizeof(server->server_addr.sun_path) - 1);
    
    // Remove existing socket file
    unlink(server->socket_path);
    
    // Bind socket
    if (bind(server->server_fd, (struct sockaddr *)&server->server_addr, 
              sizeof(server->server_addr)) < 0) {
        bu_log("Failed to bind socket: %s\n", strerror(errno));
        close(server->server_fd);
        return -1;
    }
    
    // Start listening
    if (listen(server->server_fd, SOMAXCONN) < 0) {
        bu_log("Failed to listen: %s\n", strerror(errno));
        close(server->server_fd);
        unlink(server->socket_path);
        return -1;
    }
    
    // Initialize poll array
    server->max_fds = MGED_SERVER_MAX_CLIENTS + 1;  // +1 for server socket
    server->poll_fds = (struct pollfd *)calloc(server->max_fds, sizeof(struct pollfd));
    server->client_sessions = (struct client_session **)calloc(server->max_fds, sizeof(struct client_session*));
    
    if (!server->poll_fds || !server->client_sessions) {
        bu_log("Failed to allocate memory for poll arrays\n");
        free(server->poll_fds);
        free(server->client_sessions);
        close(server->server_fd);
        unlink(server->socket_path);
        return -1;
    }
    
    // Add server socket to poll array
    server->poll_fds[0].fd = server->server_fd;
    server->poll_fds[0].events = POLLIN;
    server->num_fds = 1;
    
    server->running = 1;
    server->max_clients = MGED_SERVER_MAX_CLIENTS;
    
    return 0;
}

int mged_server_start(struct mged_server *server) {
    if (!server) {
        return -1;
    }
    
    // Validate that server is properly initialized
    if (server->server_fd < 0) {
        bu_log("ERROR: Server not properly initialized - invalid server_fd\n");
        return -1;
    }
    
    if (strlen(server->socket_path) == 0) {
        bu_log("ERROR: Server not properly initialized - empty socket path\n");
        return -1;
    }
    
    if (!server->poll_fds || !server->client_sessions) {
        bu_log("ERROR: Server not properly initialized - NULL poll arrays\n");
        return -1;
    }
    
    server->running = 1;
    bu_log("MGED server listening on %s\n", server->socket_path);
    return 0;
}

int mged_server_stop(struct mged_server *server) {
    if (!server) {
        return -1;
    }
    
    server->running = 0;
    return 0;
}

void mged_server_cleanup(struct mged_server *server) {
    if (!server) return;
    
    // Close all client connections
    for (int i = 1; i < server->num_fds; i++) {
        if (server->client_sessions[i]) {
            free_client_session(server->client_sessions[i]);
            server->client_sessions[i] = NULL;
        }
    }
    
    // Close server socket
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    // Remove socket file
    unlink(server->socket_path);
    
    // Free arrays
    free(server->poll_fds);
    free(server->client_sessions);
    
    memset(server, 0, sizeof(*server));
}

int mged_server_poll(struct mged_server *server, int timeout_ms) {
    if (!server || !server->running) {
        return -1;
    }
    
    int ret = poll(server->poll_fds, server->num_fds, timeout_ms);
    
    if (ret < 0) {
        if (errno == EINTR) {
            return 0;  // Interrupted, try again
        }
        bu_log("Poll error: %s\n", strerror(errno));
        return -1;
    }
    
    if (ret == 0) {
        return 0;  // Timeout, no events
    }
    
    // Check for new connections
    if (server->poll_fds[0].revents & POLLIN) {
        accept_new_client(server);
    }
    
    // Handle existing clients
    for (int i = 1; i < server->num_fds; i++) {
        if (server->poll_fds[i].revents & POLLIN) {
            handle_client_read(server, i);
        }
        if (server->poll_fds[i].revents & POLLOUT) {
            handle_client_write(server, i);
        }
        if (server->poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            close_client_connection(server, i);
        }
    }
    
    return ret;
}

int accept_new_client(struct mged_server *server) {
    if (!server || server->num_fds >= server->max_fds) {
        return -1;
    }
    
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server->server_fd, 
                          (struct sockaddr *)&client_addr, 
                          &client_len);
    
    if (client_fd < 0) {
        bu_log("Failed to accept client: %s\n", strerror(errno));
        return -1;
    }
    
    // Create client session
    struct client_session *client = create_client_session(client_fd);
    if (!client) {
        bu_log("Failed to create client session\n");
        close(client_fd);
        return -1;
    }
    
    // Add to poll array
    int poll_index = server->num_fds;
    server->poll_fds[poll_index].fd = client_fd;
    server->poll_fds[poll_index].events = POLLIN;
    server->poll_fds[poll_index].revents = 0;
    
    server->client_sessions[poll_index] = client;
    client->poll_index = poll_index;
    
    server->num_fds++;
    
    bu_log("Client connected: fd=%d\n", client_fd);
    return 0;
}

void close_client_connection(struct mged_server *server, int poll_index) {
    if (!server || poll_index <= 0 || poll_index >= server->num_fds) {
        return;
    }
    
    struct client_session *client = server->client_sessions[poll_index];
    if (!client) {
        return;
    }
    
    bu_log("Client disconnected: fd=%d\n", client->fd);
    
    // Free client session
    free_client_session(client);
    server->client_sessions[poll_index] = NULL;
    
    // Remove from poll array by shifting remaining entries
    for (int i = poll_index; i < server->num_fds - 1; i++) {
        server->poll_fds[i] = server->poll_fds[i + 1];
        server->client_sessions[i] = server->client_sessions[i + 1];
        if (server->client_sessions[i]) {
            server->client_sessions[i]->poll_index = i;
        }
    }
    
    server->num_fds--;
}

void handle_client_read(struct mged_server *server, int poll_index) {
    if (!server || poll_index <= 0 || poll_index >= server->num_fds) {
        return;
    }
    
    struct client_session *client = server->client_sessions[poll_index];
    if (!client) {
        return;
    }
    
    char buffer[4096];
    ssize_t bytes_read = read(client->fd, buffer, sizeof(buffer));
    
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            bu_log("Read error on client %d: %s\n", client->fd, strerror(errno));
            close_client_connection(server, poll_index);
        }
        return;
    }
    
    if (bytes_read == 0) {
        // Client disconnected
        close_client_connection(server, poll_index);
        return;
    }
    
    // Append to input buffer
    bu_vls_strncat(&client->input_buffer, buffer, bytes_read);
    
    // Check if we have a complete request (look for FS)
    const char *data = bu_vls_addr(&client->input_buffer);
    size_t len = bu_vls_strlen(&client->input_buffer);
    
    const char *fs_pos = (const char *)memchr(data, PROTOCOL_FS, len);
    if (fs_pos) {
        // We have a complete request
        size_t request_len = fs_pos - data + 1;
        
        // Parse request
        struct command_request *req = parse_protocol_request(data, request_len);
        if (!req) {
            format_protocol_response(&client->output_buffer, "ERR", "", "ERR_NOMEM");
            update_poll_events(server, client->fd, POLLOUT);
            return;
        }
        
        // Execute command
        // Clear result buffer before command execution
        if (MGED_STATE && MGED_STATE->gedp && MGED_STATE->gedp->ged_result_str) {
            bu_vls_trunc(MGED_STATE->gedp->ged_result_str, 0);
        }
        
        // Check if gedp is valid
        if (!MGED_STATE || !MGED_STATE->gedp) {
            format_protocol_response(&client->output_buffer, "ERR", "", "ERR_INTERNAL");
            update_poll_events(server, client->fd, POLLOUT);
            free_command_request(req);
            return;
        }
        
        // Make a defensive copy of argv since ged_exec may modify it
        // (specifically, it may replace argv[0] with a static string if NULL)
        const char **argv_copy = (const char **)bu_calloc(req->argc + 1, sizeof(char*), "argv copy");
        for (int i = 0; i < req->argc; i++) {
            argv_copy[i] = req->argv[i];
        }
        argv_copy[req->argc] = NULL;
        
        int result = ged_exec(MGED_STATE->gedp, req->argc, argv_copy);
        
        // Free the copy (but not the strings, which are still owned by req)
        bu_free(argv_copy, "argv copy");
        
        // Format response with additional output from command
        const char *result_text = "";
        if (MGED_STATE && MGED_STATE->gedp && MGED_STATE->gedp->ged_result_str && bu_vls_strlen(MGED_STATE->gedp->ged_result_str) > 0) {
            result_text = bu_vls_cstr(MGED_STATE->gedp->ged_result_str);
        }
        
        if (result == 0) {
            format_protocol_response(&client->output_buffer, "OK", result_text, "");
        } else {
            format_protocol_response(&client->output_buffer, "ERR", result_text, "ERR_COMMAND");
        }
        
        // Remove processed request from input buffer
        bu_vls_nibble(&client->input_buffer, request_len);
        
        // Update poll events to send response
        update_poll_events(server, client->fd, POLLOUT);
        
        free_command_request(req);
    }
}

void handle_client_write(struct mged_server *server, int poll_index) {
    if (!server || poll_index <= 0 || poll_index >= server->num_fds) {
        return;
    }
    
    struct client_session *client = server->client_sessions[poll_index];
    if (!client) {
        return;
    }
    
    const char *data = bu_vls_addr(&client->output_buffer);
    size_t len = bu_vls_strlen(&client->output_buffer);
    
    if (len == 0) {
        // Nothing to write, go back to reading
        update_poll_events(server, client->fd, POLLIN);
        return;
    }
    
    ssize_t bytes_written = write(client->fd, data, len);
    
    if (bytes_written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            bu_log("Write error on client %d: %s\n", client->fd, strerror(errno));
            close_client_connection(server, poll_index);
        }
        return;
    }
    
    if (bytes_written == 0) {
        // Shouldn't happen, but treat as disconnect
        close_client_connection(server, poll_index);
        return;
    }
    
    // Remove written data from buffer
    bu_vls_nibble(&client->output_buffer, bytes_written);
    
    if (bu_vls_strlen(&client->output_buffer) == 0) {
        // Response sent, go back to reading
        update_poll_events(server, client->fd, POLLIN);
    }
}

static void update_poll_events(struct mged_server *server, int fd, int events) {
    // Find the poll entry for this fd and update it
    for (int i = 0; i < server->num_fds; i++) {
        if (server->poll_fds[i].fd == fd) {
            server->poll_fds[i].events = events;
            break;
        }
    }
}
