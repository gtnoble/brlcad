/*                        M G E D _ I N T E G R A T I O N _ T E S T S . C
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
/** @file mged_integration_tests.c
 *
 * Integration tests for the MGED socket server
 *
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <bu.h>
#include <ged.h>

#include "mged_mock_client.h"
#include "../../src/mged/mged_server.h"

// Test helper functions
int test_server_startup(void) {
    const char *socket_path = "/tmp/test_mged_integration.sock";
    const char *mged_cmd = NULL;
    pid_t mged_pid = -1;
    int result = 0;
    
    bu_log("Testing MGED server startup...\n");
    
    // Clean up any existing socket
    unlink(socket_path);
    
    // Find MGED executable
    mged_cmd = bu_dir(NULL, 0, BU_DIR_BIN, "mged", NULL);
    if (!mged_cmd) {
        bu_log("ERROR: Could not find mged executable\n");
        return -1;
    }
    
    // Fork MGED process with server mode
    mged_pid = fork();
    if (mged_pid < 0) {
        bu_log("ERROR: Failed to fork MGED process\n");
        result = -1;
        goto cleanup;
    }
    
    if (mged_pid == 0) {
        // Child process: start MGED with server mode
        // Exec MGED with server mode and a temporary database
        // Use 'nu' display manager (null display) to avoid display initialization
        // Use 'nu' display manager (null display) to avoid display initialization
        execl(mged_cmd, "mged", "-c", "-a", "nu", "-s", socket_path, "/tmp/test_db.g", NULL);
        // If we reach here, exec failed
        perror("exec failed");
        exit(1);
    }
    
    // Parent process: wait for server to start and create socket
    // Give server more time to fully initialize
    for (int i = 0; i < 10; i++) {
        sleep(1);
        if (access(socket_path, F_OK) == 0) {
            // Socket file exists, server is running
            break;
        }
        
        // Check if child process is still running
        if (waitpid(mged_pid, NULL, WNOHANG) != 0) {
            // Child process has exited prematurely
            bu_log("ERROR: MGED server process exited unexpectedly\n");
            result = -1;
            goto cleanup;
        }
    }
    
    // Final check for socket file
    if (access(socket_path, F_OK) != 0) {
        bu_log("ERROR: Socket file not created after waiting\n");
        result = -1;
        goto cleanup;
    }
    
    bu_log("PASS: MGED server startup\n");
    
cleanup:
    // Clean up MGED process
    if (mged_pid > 0) {
        kill(mged_pid, SIGTERM);
        waitpid(mged_pid, NULL, 0);
    }
    
    unlink(socket_path);
    
    return result;
}

int test_basic_client_server_communication(void) {
    const char *socket_path = "/tmp/test_mged_comm.sock";
    const char *mged_cmd = NULL;
    pid_t mged_pid = -1;
    struct mock_client client;
    struct mock_response response;
    int result = 0;
    
    bu_log("Testing basic client-server communication...\n");
    
    // Clean up any existing socket
    unlink(socket_path);
    
    // Find MGED executable
    mged_cmd = bu_dir(NULL, 0, BU_DIR_BIN, "mged", NULL);
    if (!mged_cmd) {
        bu_log("ERROR: Could not find mged executable\n");
        return -1;
    }
    
    // Start MGED server
    mged_pid = fork();
    if (mged_pid < 0) {
        bu_log("ERROR: Failed to fork MGED process\n");
        result = -1;
        goto cleanup;
    }
    
    if (mged_pid == 0) {
        // Child process: start MGED with server mode
        // Create a temporary database file
        char temp_db[256];
        strncpy(temp_db, "/tmp/test_db.g", sizeof(temp_db));
        
        // Exec MGED
        // Use 'nu' display manager (null display) to avoid display initialization
        execl(mged_cmd, "mged", "-c", "-a", "nu", "-s", socket_path, temp_db, NULL);
        
        // If we reach here, exec failed
        perror("exec failed");
        exit(1);
    }
    
    // Parent: wait for server to start up and create socket
    for (int i = 0; i < 10; i++) {
        sleep(1);
        if (access(socket_path, F_OK) == 0) {
            // Socket file exists, server is running
            break;
        }
        
        // Check if child process is still running
        if (waitpid(mged_pid, NULL, WNOHANG) != 0) {
            // Child process has exited prematurely
            bu_log("ERROR: MGED server process exited unexpectedly\n");
            result = -1;
            goto cleanup;
        }
    }
    
    // Final check for socket file
    if (access(socket_path, F_OK) != 0) {
        bu_log("ERROR: Socket file not created after waiting\n");
        result = -1;
        goto cleanup;
    }
    
    // Initialize mock client
    if (mock_client_init(&client) != 0) {
        bu_log("ERROR: Failed to initialize mock client\n");
        result = -1;
        goto cleanup;
    }
    
    // Connect to server
    if (mock_client_connect(&client, socket_path) != 0) {
        bu_log("ERROR: Failed to connect to server\n");
        result = -1;
        goto cleanup;
    }
    
    // Send a simple command
    if (mock_client_send_command(&client, "units", 0, NULL) != 0) {
        bu_log("ERROR: Failed to send units command\n");
        result = -1;
        goto cleanup;
    }
    
    // Receive response
    if (mock_client_receive_response(&client, &response, 5000) != 0) {
        bu_log("ERROR: Failed to receive units response\n");
        result = -1;
        goto cleanup;
    }
    
    // Verify response
    if (!response.status) {
        bu_log("ERROR: Response has no status\n");
        result = -1;
        goto cleanup;
    }
    
    if (strcmp(response.status, "OK") != 0) {
        bu_log("ERROR: Expected OK status, got '%s'\n", response.status);
        if (response.result) {
            bu_log("  Result: %s\n", response.result);
        }
        if (response.error) {
            bu_log("  Error: %s\n", response.error);
        }
        result = -1;
        goto cleanup;
    }
    
    if (result == 0) {
        bu_log("PASS: Basic client-server communication\n");
    } else {
        bu_log("FAIL: Basic client-server communication\n");
    }
    
cleanup:
    // Clean up client
    mock_client_free_response(&response);
    mock_client_cleanup(&client);
    
    // Clean up MGED process
    if (mged_pid > 0) {
        kill(mged_pid, SIGTERM);
        waitpid(mged_pid, NULL, 0);
    }
    
    unlink(socket_path);
    
    return result;
}

int test_error_propagation(void) {
    const char *socket_path = "/tmp/test_mged_error.sock";
    const char *mged_cmd = NULL;
    pid_t mged_pid = -1;
    struct mock_client client;
    struct mock_response response;
    int result = 0;
    
    bu_log("Testing error propagation through protocol...\n");
    
    // Clean up any existing socket
    unlink(socket_path);
    
    // Find MGED executable
    mged_cmd = bu_dir(NULL, 0, BU_DIR_BIN, "mged", NULL);
    if (!mged_cmd) {
        bu_log("ERROR: Could not find mged executable\n");
        return -1;
    }
    
    // Start MGED server
    mged_pid = fork();
    if (mged_pid < 0) {
        bu_log("ERROR: Failed to fork MGED process\n");
        result = -1;
        goto cleanup;
    }
    
    if (mged_pid == 0) {
        // Child process: start MGED with server mode
        // Create a temporary database file
        char temp_db[256];
        strncpy(temp_db, "/tmp/test_db.g", sizeof(temp_db));
        
        // Exec MGED
        // Use 'nu' display manager (null display) to avoid display initialization
        execl(mged_cmd, "mged", "-c", "-a", "nu", "-s", socket_path, temp_db, NULL);
        
        // If we reach here, exec failed
        perror("exec failed");
        exit(1);
    }
    
    // Parent: wait for server to start up and create socket
    for (int i = 0; i < 10; i++) {
        sleep(1);
        if (access(socket_path, F_OK) == 0) {
            // Socket file exists, server is running
            break;
        }
        
        // Check if child process is still running
        if (waitpid(mged_pid, NULL, WNOHANG) != 0) {
            // Child process has exited prematurely
            bu_log("ERROR: MGED server process exited unexpectedly\n");
            result = -1;
            goto cleanup;
        }
    }
    
    // Final check for socket file
    if (access(socket_path, F_OK) != 0) {
        bu_log("ERROR: Socket file not created after waiting\n");
        result = -1;
        goto cleanup;
    }
    
    // Initialize mock client
    if (mock_client_init(&client) != 0) {
        bu_log("ERROR: Failed to initialize mock client\n");
        result = -1;
        goto cleanup;
    }
    
    // Connect to server
    if (mock_client_connect(&client, socket_path) != 0) {
        bu_log("ERROR: Failed to connect to server\n");
        result = -1;
        goto cleanup;
    }
    
    // Send an invalid command
    if (mock_client_send_command(&client, "invalid_command_that_does_not_exist", 0, NULL) != 0) {
        bu_log("ERROR: Failed to send invalid command\n");
        result = -1;
        goto cleanup;
    }
    
    // Receive response
    if (mock_client_receive_response(&client, &response, 5000) != 0) {
        bu_log("ERROR: Failed to receive error response\n");
        result = -1;
        goto cleanup;
    }
    
    // Verify error response
    if (!response.status) {
        bu_log("ERROR: Response has no status\n");
        result = -1;
        goto cleanup;
    }
    
    if (strncmp(response.status, "ERR", 3) != 0) {
        bu_log("ERROR: Expected ERR status, got '%s'\n", response.status);
        result = -1;
        goto cleanup;
    }
    
    if (!response.error) {
        bu_log("ERROR: Error response has no error message\n");
        result = -1;
        goto cleanup;
    }
    
    if (result == 0) {
        bu_log("PASS: Error propagation through protocol\n");
    } else {
        bu_log("FAIL: Error propagation through protocol\n");
    }
    
cleanup:
    // Clean up client
    mock_client_free_response(&response);
    mock_client_cleanup(&client);
    
    // Clean up MGED process
    if (mged_pid > 0) {
        kill(mged_pid, SIGTERM);
        waitpid(mged_pid, NULL, 0);
    }
    
    unlink(socket_path);
    
    return result;
}

int test_concurrent_connections(void) {
    const char *socket_path = "/tmp/test_mged_concurrent.sock";
    const char *mged_cmd = NULL;
    pid_t mged_pid = -1;
    struct mock_client clients[3];
    struct mock_response responses[3];
    int result = 0;
    
    bu_log("Testing concurrent connections...\n");
    
    // Clean up any existing socket
    unlink(socket_path);
    
    // Find MGED executable
    mged_cmd = bu_dir(NULL, 0, BU_DIR_BIN, "mged", NULL);
    if (!mged_cmd) {
        bu_log("ERROR: Could not find mged executable\n");
        return -1;
    }
    
    // Start MGED server
    mged_pid = fork();
    if (mged_pid < 0) {
        bu_log("ERROR: Failed to fork MGED process\n");
        result = -1;
        goto cleanup;
    }
    
    if (mged_pid == 0) {
        // Child process: start MGED with server mode
        // Create a temporary database file
        char temp_db[256];
        strncpy(temp_db, "/tmp/test_db.g", sizeof(temp_db));
        
        // Exec MGED
        // Use 'nu' display manager (null display) to avoid display initialization
        execl(mged_cmd, "mged", "-c", "-a", "nu", "-s", socket_path, temp_db, NULL);
        
        // If we reach here, exec failed
        perror("exec failed");
        exit(1);
    }
    
    // Parent: wait for server to start up and create socket
    for (int i = 0; i < 10; i++) {
        sleep(1);
        if (access(socket_path, F_OK) == 0) {
            // Socket file exists, server is running
            break;
        }
        
        // Check if child process is still running
        if (waitpid(mged_pid, NULL, WNOHANG) != 0) {
            // Child process has exited prematurely
            bu_log("ERROR: MGED server process exited unexpectedly\n");
            result = -1;
            goto cleanup;
        }
    }
    
    // Final check for socket file
    if (access(socket_path, F_OK) != 0) {
        bu_log("ERROR: Socket file not created after waiting\n");
        result = -1;
        goto cleanup;
    }
    
    // Initialize and connect multiple clients
    for (int i = 0; i < 3; i++) {
        if (mock_client_init(&clients[i]) != 0) {
            bu_log("ERROR: Failed to initialize mock client %d\n", i);
            result = -1;
            goto cleanup;
        }
        
        if (mock_client_connect(&clients[i], socket_path) != 0) {
            bu_log("ERROR: Failed to connect client %d\n", i);
            result = -1;
            goto cleanup;
        }
        
        // Send a simple command
        if (mock_client_send_command(&clients[i], "units", 0, NULL) != 0) {
            bu_log("ERROR: Failed to send command from client %d\n", i);
            result = -1;
            goto cleanup;
        }
    }
    
    // Receive responses from all clients
    for (int i = 0; i < 3; i++) {
        if (mock_client_receive_response(&clients[i], &responses[i], 5000) != 0) {
            bu_log("ERROR: Failed to receive response from client %d\n", i);
            result = -1;
            goto cleanup;
        }
        
        // Verify response
        if (!responses[i].status) {
            bu_log("ERROR: Client %d response has no status\n", i);
            result = -1;
            goto cleanup;
        }
        
        if (strcmp(responses[i].status, "OK") != 0) {
            bu_log("ERROR: Client %d expected OK status, got '%s'\n", i, responses[i].status);
            result = -1;
            goto cleanup;
        }
    }
    
    if (result == 0) {
        bu_log("PASS: Concurrent connections\n");
    } else {
        bu_log("FAIL: Concurrent connections\n");
    }
    
cleanup:
    // Clean up clients
    for (int i = 0; i < 3; i++) {
        mock_client_free_response(&responses[i]);
        mock_client_cleanup(&clients[i]);
    }
    
    // Clean up MGED process
    if (mged_pid > 0) {
        kill(mged_pid, SIGTERM);
        waitpid(mged_pid, NULL, 0);
    }
    
    unlink(socket_path);
    
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
        printf("  startup      - Server startup test\n");
        printf("  communication - Basic client-server communication\n");
        printf("  errors       - Error propagation testing\n");
        printf("  concurrent   - Concurrent connection handling\n");
        printf("  all          - Run all tests\n");
        return 1;
    }
    
    if (BU_STR_EQUAL(av[1], "startup") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_server_startup() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "communication") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_basic_client_server_communication() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "errors") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_error_propagation() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "concurrent") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_concurrent_connections() == 0) passed_count++;
    }
    
    if (test_count == 0) {
        bu_log("ERROR: Unknown test '%s'\n", av[1]);
        return 1;
    }
    
    bu_log("\n=== Integration Test Summary ===\n");
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