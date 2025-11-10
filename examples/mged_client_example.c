/*                          M G E D _ C L I E N T _ E X A M P L E
 * BRL-CAD
 *
 * Copyright (c) 2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file examples/mged_client_example.c
 *
 * Example client for connecting to MGED socket server
 *
 */

/*
 * MGED Socket Client Example 
 * 
 * Example client for connecting to MGED socket server using the
 * Unicode control character protocol.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <bu.h>

/* Protocol control characters from mged_protocol.h */
#define PROTOCOL_FS '\x1C'  // File Separator
#define PROTOCOL_GS '\x1D'  // Group Separator

int send_command(int sock_fd, const char *command, int argc, const char **argv) {
    struct bu_vls msg;
    ssize_t sent;
    
    bu_vls_init(&msg);
    
    // Build protocol message: command␝arg1␞arg2␞...␞argN␞␝
    bu_vls_strcat(&msg, command);
    
    for (int i = 0; i < argc; i++) {
        bu_vls_putc(&msg, PROTOCOL_GS);
        bu_vls_strcat(&msg, argv[i]);
    }
    
    bu_vls_putc(&msg, PROTOCOL_GS);
    bu_vls_putc(&msg, PROTOCOL_FS);
    
    // Send the message
    sent = send(sock_fd, bu_vls_addr(&msg), bu_vls_strlen(&msg), 0);
    
    bu_vls_free(&msg);
    
    if (sent < 0) {
        perror("Failed to send command");
        return -1;
    }
    
    return 0;
}

int receive_response(int sock_fd, int timeout_ms) {
    fd_set read_fds;
    struct timeval timeout;
    char buffer[4096];
    int found_terminator = 0;
    struct bu_vls response;
    
    bu_vls_init(&response);
    
    // Set up timeout
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Read until we get FS terminator or timeout
    while (!found_terminator) {
        // Wait for data with timeout
        int sel_result = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (sel_result < 0) {
            perror("Select error");
            bu_vls_free(&response);
            return -1;
        }
        
        if (sel_result == 0) {
            fprintf(stderr, "Timeout waiting for response\n");
            bu_vls_free(&response);
            return -1;
        }
        
        // Read available data
        ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            perror("Recv error");
            bu_vls_free(&response);
            return -1;
        }
        
        if (bytes == 0) {
            fprintf(stderr, "Server disconnected\n");
            bu_vls_free(&response);
            return -1;
        }
        
        // Append to buffer
        bu_vls_strncat(&response, buffer, bytes);
        
        // Check for FS terminator
        const char *data = bu_vls_addr(&response);
        size_t len = bu_vls_strlen(&response);
        
        for (size_t i = 0; i < len; i++) {
            if (data[i] == PROTOCOL_FS) {
                found_terminator = 1;
                break;
            }
        }
    }
    
    // Parse and display response
    const char *data = bu_vls_addr(&response);
    size_t len = bu_vls_strlen(&response);
    
    // Simple parsing for display: look for GS separators
    printf("Response received:\n");
    
    size_t pos = 0;
    size_t field_num = 0;
    
    while (pos < len && data[pos] != PROTOCOL_FS) {
        size_t start = pos;
        while (pos < len && data[pos] != PROTOCOL_GS && data[pos] != PROTOCOL_FS) {
            pos++;
        }
        
        if (pos > start) {
            printf("  Field %zu: %.*s\n", field_num, (int)(pos - start), &data[start]);
            field_num++;
        }
        
        if (pos < len && data[pos] == PROTOCOL_GS) {
            pos++;  // Skip separator
        }
    }
    
    bu_vls_free(&response);
    return 0;
}

int
main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un server_addr;
    const char *socket_path = "/tmp/mged.sock";
    const char *command = "help";

    if (argc > 1) {
	socket_path = argv[1];
    }
    if (argc > 2) {
	command = argv[2];
    }

    printf("MGED Client Example\n");
    printf("Connecting to: %s\n", socket_path);
    printf("Command to send: %s\n", command);

    // Create socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);
    
    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to connect to server at %s\n", socket_path);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Connected to MGED server\n");

    // Send command with no arguments
    if (send_command(sock_fd, command, 0, NULL) == 0) {
        printf("Command sent successfully\n");

	// Wait for and display response
	if (receive_response(sock_fd, 5000) == 0) {
	    printf("Response processed successfully\n");
	} else {
	    printf("Failed to receive response\n");
	}
    } else {
        printf("Failed to send command\n");
    }

    // Cleanup
    close(sock_fd);

    printf("Client disconnected\n");
    return EXIT_SUCCESS;
}
