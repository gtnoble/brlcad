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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Include mged client headers - use relative path from build directory */
#include "mged_client.h"
#include "mged_protocol.h"

int
main(int argc, char *argv[])
{
    struct mged_client client;
    const char *socket_path = "/tmp/mged.sock";
    const char *command = "help";
    int ret;

    if (argc > 1) {
	socket_path = argv[1];
    }
    if (argc > 2) {
	command = argv[2];
    }

    printf("MGED Client Example\n");
    printf("Connecting to: %s\n", socket_path);
    printf("Command to send: %s\n", command);

    /* Initialize client */
    if (mged_client_init(&client) < 0) {
	fprintf(stderr, "Failed to initialize client\n");
	return EXIT_FAILURE;
    }

    /* Connect to server */
    if (mged_client_connect(&client, socket_path) < 0) {
	fprintf(stderr, "Failed to connect to server at %s\n", socket_path);
	mged_client_cleanup(&client);
	return EXIT_FAILURE;
    }

    printf("Connected to MGED server\n");

    /* Send command */
    ret = mged_client_send_command(&client, command);
    if (ret < 0) {
	fprintf(stderr, "Failed to send command\n");
    } else {
	printf("Command sent successfully\n");

	/* Wait for and display response */
	struct mged_response response;
	if (mged_client_receive_response(&client, &response, 5000) > 0) {
	    printf("Response received:\n");
	    printf("  Type: %d\n", response.type);
	    printf("  Status: %d\n", response.status);
	    if (response.payload_len > 0) {
		printf("  Payload: %.*s\n", (int)response.payload_len, response.payload);
	    }
	} else {
	    printf("No response received (timeout)\n");
	}
    }

    /* Disconnect and cleanup */
    mged_client_disconnect(&client);
    mged_client_cleanup(&client);

    printf("Client disconnected\n");
    return EXIT_SUCCESS;
}
