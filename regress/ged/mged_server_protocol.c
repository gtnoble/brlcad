/*                        M G E D _ S E R V E R _ P R O T O C O L . C
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
/** @file mged_server_protocol.c
 *
 * Regression test for the MGED socket server protocol parsing.
 *
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bu.h>
#include <ged.h>
#include "../../src/mged/mged_protocol.h"

// Test helper function
int test_basic_command_parsing() {
    struct command_request *req;
    const char *test_cmd = "ls\x1C";
    int result = 0;
    
    bu_log("Testing basic command parsing...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse basic command\n");
        return -1;
    }
    
    if (strcmp(req->command, "ls") != 0) {
        bu_log("ERROR: Expected command 'ls', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 0) {
        bu_log("ERROR: Expected 0 arguments, got %d\n", req->argc);
        result = -1;
    }
    
    if (req->argv != NULL) {
        bu_log("ERROR: Expected NULL argv, got %p\n", (void *)req->argv);
        result = -1;
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Basic command parsing\n");
    } else {
        bu_log("FAIL: Basic command parsing\n");
    }
    
    return result;
}

int test_single_argument_parsing() {
    struct command_request *req;
    const char *test_cmd = "pwd\x1D/path/to/dir\x1C";
    int result = 0;
    
    bu_log("Testing single argument parsing...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse command with single argument\n");
        return -1;
    }
    
    if (strcmp(req->command, "pwd") != 0) {
        bu_log("ERROR: Expected command 'pwd', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected 1 argument, got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "/path/to/dir") != 0) {
            bu_log("ERROR: Expected argument '/path/to/dir', got '%s'\n", req->argv[0]);
            result = -1;
        }
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Single argument parsing\n");
    } else {
        bu_log("FAIL: Single argument parsing\n");
    }
    
    return result;
}

int test_multiple_arguments_parsing() {
    struct command_request *req;
    const char *test_cmd = "in\x1D" "sphere.s\x1E" "sph\x1E" "0\x1E" "0\x1E" "0\x1E" "5\x1C";
    int result = 0;
    const char *expected_args[] = {"sphere.s", "sph", "0", "0", "0", "5"};
    int expected_argc = 6;
    
    bu_log("Testing multiple arguments parsing...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse command with multiple arguments\n");
        return -1;
    }
    
    if (strcmp(req->command, "in") != 0) {
        bu_log("ERROR: Expected command 'in', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != expected_argc) {
        bu_log("ERROR: Expected %d arguments, got %d\n", expected_argc, req->argc);
        result = -1;
    } else {
        for (int i = 0; i < expected_argc; i++) {
            if (strcmp(req->argv[i], expected_args[i]) != 0) {
                bu_log("ERROR: Expected arg[%d] '%s', got '%s'\n", i, expected_args[i], req->argv[i]);
                result = -1;
            }
        }
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Multiple arguments parsing\n");
    } else {
        bu_log("FAIL: Multiple arguments parsing\n");
    }
    
    return result;
}

int test_special_characters() {
    struct command_request *req;
    const char *test_cmd = "draw\x1D\"my object\"\x1C";
    int result = 0;
    
    bu_log("Testing special characters in arguments...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse command with special characters\n");
        return -1;
    }
    
    if (strcmp(req->command, "draw") != 0) {
        bu_log("ERROR: Expected command 'draw', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected 1 argument, got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "\"my object\"") != 0) {
            bu_log("ERROR: Expected argument '\"my object\"', got '%s'\n", req->argv[0]);
            result = -1;
        }
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Special characters in arguments\n");
    } else {
        bu_log("FAIL: Special characters in arguments\n");
    }
    
    return result;
}

int test_empty_command() {
    struct command_request *req;
    const char *test_cmd = "\x1C";  // Just FS, empty command
    int result = 0;
    
    bu_log("Testing empty command...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse empty command\n");
        return -1;
    }
    
    if (strlen(req->command) != 0) {
        bu_log("ERROR: Expected empty command, got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 0) {
        bu_log("ERROR: Expected 0 arguments, got %d\n", req->argc);
        result = -1;
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Empty command parsing\n");
    } else {
        bu_log("FAIL: Empty command parsing\n");
    }
    
    return result;
}

int test_malformed_protocol() {
    struct command_request *req;
    const char *test_cmd = "incomplete_command";  // Missing FS
    int result = 0;
    
    bu_log("Testing malformed protocol (missing FS)...\n");
    
    // This should still parse, treating it as a command without FS terminator
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse incomplete command\n");
        return -1;
    }
    
    if (strcmp(req->command, "incomplete_command") != 0) {
        bu_log("ERROR: Expected command 'incomplete_command', got '%s'\n", req->command);
        result = -1;
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Malformed protocol handling\n");
    } else {
        bu_log("FAIL: Malformed protocol handling\n");
    }
    
    return result;
}

int test_response_formatting() {
    struct bu_vls response;
    int result = 0;
    
    bu_log("Testing response formatting...\n");
    
    bu_vls_init(&response);
    
    // Test success response
    if (format_protocol_response(&response, "OK", "Command completed", NULL) != 0) {
        bu_log("ERROR: Failed to format response\n");
        result = -1;
        goto cleanup;
    }
    
    const char *expected = "OK\x1D" "Command completed\x1D\x1C";
    if (strcmp(bu_vls_addr(&response), expected) != 0) {
        bu_log("ERROR: Expected response '%s', got '%s'\n", expected, bu_vls_addr(&response));
        result = -1;
    }
    
    // Test error response
    bu_vls_trunc(&response, 0);
    if (format_protocol_response(&response, "ERR", NULL, "Invalid command") != 0) {
        bu_log("ERROR: Failed to format error response\n");
        result = -1;
        goto cleanup;
    }
    
    const char *expected_err = "ERR\x1D\x1DInvalid command\x1C";
    if (strcmp(bu_vls_addr(&response), expected_err) != 0) {
        bu_log("ERROR: Expected error response '%s', got '%s'\n", expected_err, bu_vls_addr(&response));
        result = -1;
    }
    
cleanup:
    bu_vls_free(&response);
    
    if (result == 0) {
        bu_log("PASS: Response formatting\n");
    } else {
        bu_log("FAIL: Response formatting\n");
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
        printf("  basic        - Basic command parsing\n");
        printf("  single       - Single argument parsing\n");
        printf("  multiple     - Multiple arguments parsing\n");
        printf("  special      - Special characters handling\n");
        printf("  empty        - Empty command handling\n");
        printf("  malformed    - Malformed protocol handling\n");
        printf("  response     - Response formatting\n");
        printf("  all          - Run all tests\n");
        return 1;
    }
    
    if (BU_STR_EQUAL(av[1], "basic") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_basic_command_parsing() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "single") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_single_argument_parsing() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "multiple") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_multiple_arguments_parsing() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "special") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_special_characters() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "empty") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_empty_command() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "malformed") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_malformed_protocol() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "response") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_response_formatting() == 0) passed_count++;
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
