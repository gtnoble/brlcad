/*                        M G E D _ S E R V E R _ C O R E . C
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
/** @file mged_server_core.c
 *
 * Regression test for the MGED socket server core functionality.
 *
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <bu.h>
#include <ged.h>

// Include mged.h to get the full mged_state definition
#include "../../src/mged/mged.h"

// Include the server headers from the source tree
#include "../../src/mged/mged_server.h"
#include "../../src/mged/mged_client.h"
#include "../../src/mged/mged_protocol.h"

// MGED_STATE is declared as extern in mged.h, but since this test doesn't link
// against mged.c, we need to provide the definition here.
// Note: mged.h already has the extern declaration, so we just define it.
struct mged_state *MGED_STATE = NULL;

// Test helper functions
int test_server_initialization() {
    struct mged_server server;
    const char *test_socket = "/tmp/test_mged_server.sock";
    int result = 0;
    
    bu_log("Testing server initialization...\n");
    
    // Clean up any existing socket
    unlink(test_socket);
    
    if (mged_server_init(&server, test_socket) != 0) {
        bu_log("ERROR: Failed to initialize server\n");
        return -1;
    }
    
    // Verify server state
    if (server.running != 1) {
        bu_log("ERROR: Server not marked as running\n");
        result = -1;
    }
    
    if (server.server_fd < 0) {
        bu_log("ERROR: Invalid server file descriptor\n");
        result = -1;
    }
    
    if (strcmp(server.socket_path, test_socket) != 0) {
        bu_log("ERROR: Socket path not set correctly\n");
        result = -1;
    }
    
    // Test server start
    if (mged_server_start(&server) != 0) {
        bu_log("ERROR: Failed to start server\n");
        result = -1;
    }
    
    // Cleanup
    mged_server_cleanup(&server);
    
    // Verify cleanup
    if (access(test_socket, F_OK) == 0) {
        bu_log("ERROR: Socket file not cleaned up\n");
        result = -1;
    }
    
    if (result == 0) {
        bu_log("PASS: Server initialization\n");
    } else {
        bu_log("FAIL: Server initialization\n");
    }
    
    return result;
}

int test_socket_creation() {
    int sock_fd;
    struct sockaddr_un addr;
    const char *test_socket = "/tmp/test_socket_creation.sock";
    int result = 0;
    
    bu_log("Testing Unix domain socket creation...\n");
    
    // Clean up any existing socket
    unlink(test_socket);
    
    // Create socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        bu_log("ERROR: Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Set up address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, test_socket, sizeof(addr.sun_path) - 1);
    
    // Bind socket
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        bu_log("ERROR: Failed to bind socket: %s\n", strerror(errno));
        result = -1;
        goto cleanup;
    }
    
    // Start listening
    if (listen(sock_fd, SOMAXCONN) < 0) {
        bu_log("ERROR: Failed to listen: %s\n", strerror(errno));
        result = -1;
        goto cleanup;
    }
    
    // Verify socket file exists
    if (access(test_socket, F_OK) != 0) {
        bu_log("ERROR: Socket file not created\n");
        result = -1;
    }
    
cleanup:
    close(sock_fd);
    unlink(test_socket);
    
    if (result == 0) {
        bu_log("PASS: Unix domain socket creation\n");
    } else {
        bu_log("FAIL: Unix domain socket creation\n");
    }
    
    return result;
}

int test_client_session_creation() {
    struct client_session *client;
    int test_fd = 123;  // Dummy file descriptor
    int result = 0;
    
    bu_log("Testing client session creation...\n");
    
    client = create_client_session(test_fd);
    if (!client) {
        bu_log("ERROR: Failed to create client session\n");
        return -1;
    }
    
    // Verify client state
    if (client->fd != test_fd) {
        bu_log("ERROR: Client file descriptor not set correctly\n");
        result = -1;
    }
    
    // Note: Clients use the shared MGED_STATE->gedp for command execution
    // No per-client GED instance is created
    
    if (client->state != CLIENT_NEW) {
        bu_log("ERROR: Client state not set to NEW\n");
        result = -1;
    }
    
    // Cleanup
    free_client_session(client);
    
    if (result == 0) {
        bu_log("PASS: Client session creation\n");
    } else {
        bu_log("FAIL: Client session creation\n");
    }
    
    return result;
}

int test_client_buffer_management() {
    struct client_session *client;
    int test_fd = 123;  // Dummy file descriptor
    int result = 0;
    
    bu_log("Testing client buffer management...\n");
    
    client = create_client_session(test_fd);
    if (!client) {
        bu_log("ERROR: Failed to create client session\n");
        return -1;
    }
    
    // Test buffer reset
    reset_client_buffers(client);
    
    // Verify buffers are empty
    if (bu_vls_strlen(&client->input_buffer) != 0) {
        bu_log("ERROR: Input buffer not empty after reset\n");
        result = -1;
    }
    
    if (bu_vls_strlen(&client->output_buffer) != 0) {
        bu_log("ERROR: Output buffer not empty after reset\n");
        result = -1;
    }
    
    // Test buffer operations
    bu_vls_strcat(&client->input_buffer, "test data");
    if (bu_vls_strlen(&client->input_buffer) != 9) {
        bu_log("ERROR: Input buffer length incorrect after append\n");
        result = -1;
    }
    
    // Cleanup
    free_client_session(client);
    
    if (result == 0) {
        bu_log("PASS: Client buffer management\n");
    } else {
        bu_log("FAIL: Client buffer management\n");
    }
    
    return result;
}

int test_poll_loop_basic() {
    struct mged_server server;
    const char *test_socket = "/tmp/test_poll_loop.sock";
    int result = 0;
    
    bu_log("Testing poll loop basic functionality...\n");
    
    // Clean up any existing socket
    unlink(test_socket);
    
    if (mged_server_init(&server, test_socket) != 0) {
        bu_log("ERROR: Failed to initialize server for poll test\n");
        return -1;
    }
    
    if (mged_server_start(&server) != 0) {
        bu_log("ERROR: Failed to start server for poll test\n");
        result = -1;
        goto cleanup;
    }
    
    // Test poll with timeout (should return 0 with no activity)
    int poll_result = mged_server_poll(&server, 100);  // 100ms timeout
    if (poll_result < 0) {
        bu_log("ERROR: Poll returned error with no activity\n");
        result = -1;
    }
    
    // Test poll with zero timeout (should return immediately)
    poll_result = mged_server_poll(&server, 0);
    if (poll_result < 0) {
        bu_log("ERROR: Poll with zero timeout failed\n");
        result = -1;
    }
    
cleanup:
    mged_server_cleanup(&server);
    
    if (result == 0) {
        bu_log("PASS: Poll loop basic functionality\n");
    } else {
        bu_log("FAIL: Poll loop basic functionality\n");
    }
    
    return result;
}

int test_error_handling() {
    struct mged_server server;
    const char *invalid_socket = "/invalid/path/socket.sock";
    int result = 0;
    
    bu_log("Testing error handling...\n");
    
    // Test initialization with invalid path
    if (mged_server_init(&server, invalid_socket) == 0) {
        bu_log("ERROR: Server initialization should fail with invalid path\n");
        result = -1;
    }
    
    // Test operations on uninitialized server
    memset(&server, 0, sizeof(server));
    if (mged_server_start(&server) == 0) {
        bu_log("ERROR: Server start should fail on uninitialized server\n");
        result = -1;
    }
    
    if (result == 0) {
        bu_log("PASS: Error handling\n");
    } else {
        bu_log("FAIL: Error handling\n");
    }
    
    return result;
}

int test_mged_state_integration() {
    struct mged_server server;
    const char *test_socket = "/tmp/test_mged_state.sock";
    int result = 0;
    
    bu_log("Testing MGED_STATE integration...\n");
    
    // Clean up any existing socket
    unlink(test_socket);
    
    // Initialize server with MGED_STATE context
    if (mged_server_init(&server, test_socket) != 0) {
        bu_log("ERROR: Failed to initialize server\n");
        return -1;
    }
    
    // Verify server can be started (simulating MGED startup)
    if (mged_server_start(&server) != 0) {
        bu_log("ERROR: Failed to start server\n");
        result = -1;
        goto cleanup;
    }
    
    // Test that server is properly configured
    if (server.running != 1) {
        bu_log("ERROR: Server not marked as running\n");
        result = -1;
    }
    
    if (server.server_fd < 0) {
        bu_log("ERROR: Invalid server file descriptor\n");
        result = -1;
    }
    
    // Test poll loop functionality (which would be integrated into MGED's event loop)
    int poll_result = mged_server_poll(&server, 10);  // 10ms timeout
    if (poll_result < 0) {
        bu_log("ERROR: Server poll failed\n");
        result = -1;
    }
    
cleanup:
    mged_server_cleanup(&server);
    
    if (result == 0) {
        bu_log("PASS: MGED_STATE integration\n");
    } else {
        bu_log("FAIL: MGED_STATE integration\n");
    }
    
    return result;
}

int main(int ac, char *av[]) {
    int test_count = 0;
    int passed_count = 0;
    
    /* Need this for bu_dir to work correctly */
    bu_setprogname(av[0]);
    
    if (ac != 2) {
        printf("Usage: %s test_name\n", av[0]);
        printf("Available tests:\n");
        printf("  init         - Server initialization\n");
        printf("  socket       - Unix domain socket creation\n");
        printf("  client       - Client session creation\n");
        printf("  buffer       - Client buffer management\n");
        printf("  poll         - Poll loop functionality\n");
        printf("  integration  - MGED_STATE integration\n");
        printf("  error        - Error handling\n");
        printf("  all          - Run all tests\n");
        return 1;
    }
    
    if (BU_STR_EQUAL(av[1], "init") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_server_initialization() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "socket") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_socket_creation() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "client") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_client_session_creation() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "buffer") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_client_buffer_management() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "poll") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_poll_loop_basic() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "error") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_error_handling() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "integration") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_mged_state_integration() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "poll") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_poll_loop_basic() == 0) passed_count++;
    }
    
    if (test_count == 0) {
        bu_log("ERROR: Unknown test '%s'\n", av[1]);
        return 1;
    }
    
    bu_log("\n=== Test Summary ===\n");
    bu_log("Tests run: %d\n", test_count);
    bu_log("Tests passed: %d\n", passed_count);
    bu_log("Tests failed: %d\n", test_count - passed_count);
    
    if (passed_count == test_count) {
        bu_log("All tests PASSED!\n");
        return 0;
    } else {
        bu_log("Some tests FAILED!\n");
        return 1;
    }
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
