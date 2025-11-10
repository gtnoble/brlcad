/*                        M G E D _ M O C K _ C L I E N T . C
 * BRL-CAD
 *
 * Copyright (c) 2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file mged_mock_client.c
 *
 * Mock client for connecting to MGED socket server
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <bu.h>

#include "mged_mock_client.h"
#include "../../src/mged/mged_protocol.h"

int mock_client_init(struct mock_client *client) {
    if (!client) {
        return -1;
    }
    
    memset(client, 0, sizeof(*client));
    client->sock_fd = -1;
    
    // Initialize buffers
    bu_vls_init(&client->send_buffer);
    bu_vls_init(&client->recv_buffer);
    
    return 0;
}

int mock_client_connect(struct mock_client *client, const char *socket_path) {
    if (!client || !socket_path) {
        return -1;
    }
    
    // Create socket
    client->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client->sock_fd < 0) {
        bu_log("Failed to create client socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Set up server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sun_family = AF_UNIX;
    strncpy(client->server_addr.sun_path, socket_path, 
             sizeof(client->server_addr.sun_path) - 1);
    
    // Connect to server
    if (connect(client->sock_fd, (struct sockaddr *)&client->server_addr,
               sizeof(client->server_addr)) < 0) {
        bu_log("Failed to connect to server: %s\n", strerror(errno));
        close(client->sock_fd);
        client->sock_fd = -1;
        return -1;
    }
    
    return 0;
}

int mock_client_send_command(struct mock_client *client, const char *command, 
                           int argc, const char **argv) {
    if (!client || !command || client->sock_fd < 0) {
        return -1;
    }
    
    // Build protocol message
    bu_vls_trunc(&client->send_buffer, 0);
    bu_vls_strcat(&client->send_buffer, command);
    
    // Add arguments
    for (int i = 0; i < argc; i++) {
        bu_vls_putc(&client->send_buffer, PROTOCOL_GS);
        bu_vls_strcat(&client->send_buffer, argv[i]);
    }
    
    // Add terminator
    bu_vls_putc(&client->send_buffer, PROTOCOL_GS);
    bu_vls_putc(&client->send_buffer, PROTOCOL_FS);
    
    // Send message
    const char *msg = bu_vls_addr(&client->send_buffer);
    size_t len = bu_vls_strlen(&client->send_buffer);
    
    ssize_t sent = send(client->sock_fd, msg, len, 0);
    if (sent < 0) {
        bu_log("Failed to send command: %s\n", strerror(errno));
        return -1;
    }
    
    if ((size_t)sent != len) {
        bu_log("Partial send: %zd of %zu bytes\n", sent, len);
        return -1;
    }
    
    return 0;
}

int mock_client_receive_response(struct mock_client *client, struct mock_response *response,
                               int timeout_ms) {
    if (!client || !response || client->sock_fd < 0) {
        return -1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    char buffer[4096];
    int found_terminator = 0;
    
    // Clear receive buffer
    bu_vls_trunc(&client->recv_buffer, 0);
    
    // Set up timeout
    FD_ZERO(&read_fds);
    FD_SET(client->sock_fd, &read_fds);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Read until we get FS terminator or timeout
    while (!found_terminator) {
        // Wait for data with timeout
        int sel_result = select(client->sock_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (sel_result < 0) {
            bu_log("Select error: %s\n", strerror(errno));
            return -1;
        }
        
        if (sel_result == 0) {
            bu_log("Timeout waiting for response\n");
            return -1;
        }
        
        // Read available data
        ssize_t bytes = recv(client->sock_fd, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            bu_log("Recv error: %s\n", strerror(errno));
            return -1;
        }
        
        if (bytes == 0) {
            bu_log("Server disconnected\n");
            return -1;
        }
        
        // Append to buffer
        bu_vls_strncat(&client->recv_buffer, buffer, bytes);
        
        // Check for FS terminator
        const char *data = bu_vls_addr(&client->recv_buffer);
        size_t len = bu_vls_strlen(&client->recv_buffer);
        
        for (size_t i = 0; i < len; i++) {
            if (data[i] == PROTOCOL_FS) {
                found_terminator = 1;
                break;
            }
        }
    }
    
    // Parse response
    const char *data = bu_vls_addr(&client->recv_buffer);
    size_t len = bu_vls_strlen(&client->recv_buffer);
    
    // Format: status␝result␝error␝␝
    response->status = NULL;
    response->result = NULL;
    response->error = NULL;
    
    // Find status (first field)
    size_t pos = 0;
    size_t start = pos;
    
    while (pos < len && data[pos] != PROTOCOL_GS && data[pos] != PROTOCOL_FS) {
        pos++;
    }
    
    if (pos > start) {
        response->status = (char *)bu_malloc(pos - start + 1, "status");
        strncpy(response->status, &data[start], pos - start);
        response->status[pos - start] = '\0';
    }
    
    // Find result (second field)
    if (pos < len && data[pos] == PROTOCOL_GS) {
        pos++;
        start = pos;
        
        while (pos < len && data[pos] != PROTOCOL_GS && data[pos] != PROTOCOL_FS) {
            pos++;
        }
        
        if (pos > start) {
            response->result = (char *)bu_malloc(pos - start + 1, "result");
            strncpy(response->result, &data[start], pos - start);
            response->result[pos - start] = '\0';
        }
    }
    
    // Find error (third field)
    if (pos < len && data[pos] == PROTOCOL_GS) {
        pos++;
        start = pos;
        
        while (pos < len && data[pos] != PROTOCOL_GS && data[pos] != PROTOCOL_FS) {
            pos++;
        }
        
        if (pos > start) {
            response->error = (char *)bu_malloc(pos - start + 1, "error");
            strncpy(response->error, &data[start], pos - start);
            response->error[pos - start] = '\0';
        }
    }
    
    return 0;
}

void mock_client_free_response(struct mock_response *response) {
    if (!response) {
        return;
    }
    
    if (response->status) {
        bu_free(response->status, "status");
        response->status = NULL;
    }
    
    if (response->result) {
        bu_free(response->result, "result");
        response->result = NULL;
    }
    
    if (response->error) {
        bu_free(response->error, "error");
        response->error = NULL;
    }
}

void mock_client_disconnect(struct mock_client *client) {
    if (!client) {
        return;
    }
    
    if (client->sock_fd >= 0) {
        close(client->sock_fd);
        client->sock_fd = -1;
    }
}

void mock_client_cleanup(struct mock_client *client) {
    if (!client) {
        return;
    }
    
    mock_client_disconnect(client);
    
    bu_vls_free(&client->send_buffer);
    bu_vls_free(&client->recv_buffer);
    
    memset(client, 0, sizeof(*client));
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */