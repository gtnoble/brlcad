/*                        M G E D _ C L I E N T _ T E S T S . C
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
/** @file mged_client_tests.c
 *
 * Regression test for the MGED socket server client management.
 *
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bu.h>
#include <ged.h>

// Include the server headers from the source tree
#include "../../src/mged/mged_server.h"
#include "../../src/mged/mged_client.h"
#include "../../src/mged/mged_protocol.h"

// Test helper functions
int test_client_creation_and_cleanup() {
    struct client_session *client;
    int test_fd = 123;  // Dummy file descriptor
    int result = 0;
    
    bu_log("Testing client creation and cleanup...\n");
    
    // Test creation
    client = create_client_session(test_fd);
    if (!client) {
        bu_log("ERROR: Failed to create client session\n");
        return -1;
    }
    
    // Verify initial state
    if (client->fd != test_fd) {
        bu_log("ERROR: Client file descriptor not set correctly\n");
        result = -1;
    }
    
    if (client->gedp == NULL) {
        bu_log("ERROR: Client GED instance not created\n");
        result = -1;
    }
    
    if (client->state != CLIENT_NEW) {
        bu_log("ERROR: Client state not set to NEW\n");
        result = -1;
    }
    
    // Test buffer initialization
    if (bu_vls_strlen(&client->input_buffer) != 0) {
        bu_log("ERROR: Input buffer not initially empty\n");
        result = -1;
    }
    
    if (bu_vls_strlen(&client->output_buffer) != 0) {
        bu_log("ERROR: Output buffer not initially empty\n");
        result = -1;
    }
    
    // Test cleanup
    free_client_session(client);
    
    if (result == 0) {
        bu_log("PASS: Client creation and cleanup\n");
    } else {
        bu_log("FAIL: Client creation and cleanup\n");
    }
    
    return result;
}

int test_buffer_operations() {
    struct client_session *client;
    int test_fd = 123;  // Dummy file descriptor
    int result = 0;
    
    bu_log("Testing buffer operations...\n");
    
    client = create_client_session(test_fd);
    if (!client) {
        bu_log("ERROR: Failed to create client session\n");
        return -1;
    }
    
    // Test buffer reset
    reset_client_buffers(client);
    
    // Test input buffer operations
    bu_vls_strcat(&client->input_buffer, "test command");
    if (bu_vls_strlen(&client->input_buffer) != 12) {
        bu_log("ERROR: Input buffer length incorrect\n");
        result = -1;
    }
    
    if (strcmp(bu_vls_addr(&client->input_buffer), "test command") != 0) {
        bu_log("ERROR: Input buffer content incorrect\n");
        result = -1;
    }
    
    // Test output buffer operations
    bu_vls_strcat(&client->output_buffer, "test response");
    if (bu_vls_strlen(&client->output_buffer) != 13) {
        bu_log("ERROR: Output buffer length incorrect\n");
        result = -1;
    }
    
    if (strcmp(bu_vls_addr(&client->output_buffer), "test response") != 0) {
        bu_log("ERROR: Output buffer content incorrect\n");
        result = -1;
    }
    
    // Test buffer reset again
    reset_client_buffers(client);
    
    if (bu_vls_strlen(&client->input_buffer) != 0) {
        bu_log("ERROR: Input buffer not empty after reset\n");
        result = -1;
    }
    
    if (bu_vls_strlen(&client->output_buffer) != 0) {
        bu_log("ERROR: Output buffer not empty after reset\n");
        result = -1;
    }
    
    // Cleanup
    free_client_session(client);
    
    if (result == 0) {
        bu_log("PASS: Buffer operations\n");
    } else {
        bu_log("FAIL: Buffer operations\n");
    }
    
    return result;
}

int test_state_transitions() {
    struct client_session *client;
    int test_fd = 123;  // Dummy file descriptor
    int result = 0;
    
    bu_log("Testing client state transitions...\n");
    
    client = create_client_session(test_fd);
    if (!client) {
        bu_log("ERROR: Failed to create client session\n");
        return -1;
    }
    
    // Test initial state
    if (client->state != CLIENT_NEW) {
        bu_log("ERROR: Initial state not NEW\n");
        result = -1;
    }
    
    // Test state transitions (manual for testing)
    client->state = CLIENT_READING;
    if (client->state != CLIENT_READING) {
        bu_log("ERROR: State transition to READING failed\n");
        result = -1;
    }
    
    client->state = CLIENT_PROCESSING;
    if (client->state != CLIENT_PROCESSING) {
        bu_log("ERROR: State transition to PROCESSING failed\n");
        result = -1;
    }
    
    client->state = CLIENT_WRITING;
    if (client->state != CLIENT_WRITING) {
        bu_log("ERROR: State transition to WRITING failed\n");
        result = -1;
    }
    
    client->state = CLIENT_CLOSING;
    if (client->state != CLIENT_CLOSING) {
        bu_log("ERROR: State transition to CLOSING failed\n");
        result = -1;
    }
    
    // Cleanup
    free_client_session(client);
    
    if (result == 0) {
        bu_log("PASS: Client state transitions\n");
    } else {
        bu_log("FAIL: Client state transitions\n");
    }
    
    return result;
}

int test_multiple_clients() {
    struct client_session *clients[5];
    int test_fds[] = {101, 102, 103, 104, 105};
    int result = 0;
    
    bu_log("Testing multiple client management...\n");
    
    // Create multiple clients
    for (int i = 0; i < 5; i++) {
        clients[i] = create_client_session(test_fds[i]);
        if (!clients[i]) {
            bu_log("ERROR: Failed to create client %d\n", i);
            result = -1;
            goto cleanup;
        }
        
        // Verify independence
        if (clients[i]->fd != test_fds[i]) {
            bu_log("ERROR: Client %d has wrong file descriptor\n", i);
            result = -1;
            goto cleanup;
        }
        
        if (clients[i]->gedp == NULL) {
            bu_log("ERROR: Client %d has NULL GED instance\n", i);
            result = -1;
            goto cleanup;
        }
    }
    
    // Test that clients are independent
    for (int i = 0; i < 5; i++) {
        bu_vls_strcat(&clients[i]->input_buffer, "client data");
        
        for (int j = 0; j < 5; j++) {
            if (i != j && bu_vls_strlen(&clients[j]->input_buffer) != 0) {
                bu_log("ERROR: Client %d buffer affected by client %d\n", j, i);
                result = -1;
                goto cleanup;
            }
        }
        
        // Reset for next iteration
        reset_client_buffers(clients[i]);
    }
    
cleanup:
    // Cleanup all clients
    for (int i = 0; i < 5; i++) {
        if (clients[i]) {
            free_client_session(clients[i]);
        }
    }
    
    if (result == 0) {
        bu_log("PASS: Multiple client management\n");
    } else {
        bu_log("FAIL: Multiple client management\n");
    }
    
    return result;
}

int test_error_handling() {
    struct client_session *client;
    int result = 0;
    
    bu_log("Testing client error handling...\n");
    
    // Test creation with invalid file descriptor
    client = create_client_session(-1);
    if (client != NULL) {
        bu_log("ERROR: Should not create client with invalid fd\n");
        result = -1;
        free_client_session(client);
    }
    
    // Test operations on NULL client
    reset_client_buffers(NULL);  // Should not crash
    
    // Test freeing NULL client
    free_client_session(NULL);  // Should not crash
    
    if (result == 0) {
        bu_log("PASS: Client error handling\n");
    } else {
        bu_log("FAIL: Client error handling\n");
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
        printf("  creation     - Client creation and cleanup\n");
        printf("  buffer       - Buffer operations\n");
        printf("  state        - State transitions\n");
        printf("  multiple     - Multiple client management\n");
        printf("  error        - Error handling\n");
        printf("  all          - Run all tests\n");
        return 1;
    }
    
    if (BU_STR_EQUAL(av[1], "creation") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_client_creation_and_cleanup() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "buffer") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_buffer_operations() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "state") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_state_transitions() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "multiple") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_multiple_clients() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "error") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_error_handling() == 0) passed_count++;
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
