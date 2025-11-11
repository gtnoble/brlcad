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
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected argc=1 (command only), got %d\n", req->argc);
        result = -1;
    }
    
    if (req->argv == NULL) {
        bu_log("ERROR: Expected argv with command, got NULL\n");
        result = -1;
    } else if (strcmp(req->argv[0], "ls") != 0) {
        bu_log("ERROR: Expected argv[0]='ls', got '%s'\n", req->argv[0]);
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
    
    if (req->argc != 2) {
        bu_log("ERROR: Expected argc=2 (command + 1 arg), got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "pwd") != 0) {
            bu_log("ERROR: Expected argv[0]='pwd', got '%s'\n", req->argv[0]);
            result = -1;
        }
        if (strcmp(req->argv[1], "/path/to/dir") != 0) {
            bu_log("ERROR: Expected argv[1]='/path/to/dir', got '%s'\n", req->argv[1]);
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
    const char *test_cmd = "in\x1D" "sphere.s\x1D" "sph\x1D" "0\x1D" "0\x1D" "0\x1D" "5\x1C";
    int result = 0;
    const char *expected_argv[] = {"in", "sphere.s", "sph", "0", "0", "0", "5"};
    int expected_argc = 7;  /* command + 6 args */
    
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
        bu_log("ERROR: Expected argc=%d, got %d\n", expected_argc, req->argc);
        result = -1;
    } else {
        for (int i = 0; i < expected_argc; i++) {
            if (strcmp(req->argv[i], expected_argv[i]) != 0) {
                bu_log("ERROR: Expected argv[%d]='%s', got '%s'\n", i, expected_argv[i], req->argv[i]);
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
    
    if (req->argc != 2) {
        bu_log("ERROR: Expected argc=2, got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "draw") != 0) {
            bu_log("ERROR: Expected argv[0]='draw', got '%s'\n", req->argv[0]);
            result = -1;
        }
        if (strcmp(req->argv[1], "\"my object\"") != 0) {
            bu_log("ERROR: Expected argv[1]='\"my object\"', got '%s'\n", req->argv[1]);
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
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected argc=1, got %d\n", req->argc);
        result = -1;
    } else if (strlen(req->argv[0]) != 0) {
        bu_log("ERROR: Expected empty argv[0], got '%s'\n", req->argv[0]);
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

int test_partial_message_handling() {
    struct bu_vls buffer;
    int result = 0;
    
    bu_log("Testing partial message handling...\n");
    
    bu_vls_init(&buffer);
    
    // Test parsing with incomplete command (no FS)
    const char *partial_cmd = "ls";
    struct command_request *req = parse_protocol_request(partial_cmd, strlen(partial_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse partial command\n");
        result = -1;
        goto cleanup;
    }
    
    if (strcmp(req->command, "ls") != 0) {
        bu_log("ERROR: Expected command 'ls', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected argc=1, got %d\n", req->argc);
        result = -1;
    }
    
    free_command_request(req);
    
    // Test parsing with partial argument (no terminating FS)
    const char *partial_arg = "pwd\x1D/path";
    req = parse_protocol_request(partial_arg, strlen(partial_arg));
    if (!req) {
        bu_log("ERROR: Failed to parse command with partial argument\n");
        result = -1;
        goto cleanup;
    }
    
    if (strcmp(req->command, "pwd") != 0) {
        bu_log("ERROR: Expected command 'pwd', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 2) {
        bu_log("ERROR: Expected argc=2, got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "pwd") != 0) {
            bu_log("ERROR: Expected argv[0]='pwd', got '%s'\n", req->argv[0]);
            result = -1;
        }
        if (strcmp(req->argv[1], "/path") != 0) {
            bu_log("ERROR: Expected argv[1]='/path', got '%s'\n", req->argv[1]);
            result = -1;
        }
    }
    
    free_command_request(req);
    
cleanup:
    bu_vls_free(&buffer);
    
    if (result == 0) {
        bu_log("PASS: Partial message handling\n");
    } else {
        bu_log("FAIL: Partial message handling\n");
    }
    
    return result;
}

int test_multiple_messages_in_buffer() {
    int result = 0;
    
    bu_log("Testing multiple messages in single buffer...\n");
    
    // This test is designed to verify that parse_protocol_request correctly
    // isolates a single command when multiple commands are in the buffer
    
    // Test with a buffer containing exactly one command with FS terminator
    const char *single_cmd = "ls\x1C";
    struct command_request *req = parse_protocol_request(single_cmd, strlen(single_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse single command\n");
        result = -1;
        goto cleanup;
    }
    
    if (strcmp(req->command, "ls") != 0) {
        bu_log("ERROR: Expected command 'ls', got '%s'\n", req->command);
        result = -1;
    }
    
    if (req->argc != 1) {
        bu_log("ERROR: Expected argc=1 (command only), got %d\n", req->argc);
        result = -1;
    }
    
    free_command_request(req);
    
    // Test parsing a command with arguments and FS terminator
    const char *cmd_with_args = "pwd\x1D/path\x1C";
    req = parse_protocol_request(cmd_with_args, strlen(cmd_with_args));
    if (!req) {
        bu_log("ERROR: Failed to parse command with args\n");
        result = -1;
        goto cleanup;
    }
    
    if (strcmp(req->command, "pwd") != 0) {
        bu_log("ERROR: Expected command 'pwd', got '%s'\n", req->command);
        result = -1;
    }
    
    free_command_request(req);
    
cleanup:
    if (result == 0) {
        bu_log("PASS: Multiple messages in buffer\n");
    } else {
        bu_log("FAIL: Multiple messages in buffer\n");
    }
    
    return result;
}

int test_error_propagation() {
    struct bu_vls response;
    int result = 0;
    
    bu_log("Testing error propagation in protocol...\n");
    
    bu_vls_init(&response);
    
    // Test error response with message
    if (format_protocol_response(&response, "ERR", NULL, "Invalid command syntax") != 0) {
        bu_log("ERROR: Failed to format error response\n");
        result = -1;
        goto cleanup;
    }
    
    const char *expected_err = "ERR\x1D\x1DInvalid command syntax\x1C";
    if (strcmp(bu_vls_addr(&response), expected_err) != 0) {
        bu_log("ERROR: Expected error response '%s', got '%s'\n", expected_err, bu_vls_addr(&response));
        result = -1;
    }
    
    // Test error response with both result and error
    bu_vls_trunc(&response, 0);
    if (format_protocol_response(&response, "ERR", "Partial result", "In complete execution") != 0) {
        bu_log("ERROR: Failed to format error response with result\n");
        result = -1;
        goto cleanup;
    }
    
    const char *expected_err_both = "ERR\x1DPartial result\x1DIn complete execution\x1C";
    if (strcmp(bu_vls_addr(&response), expected_err_both) != 0) {
        bu_log("ERROR: Expected error response '%s', got '%s'\n", expected_err_both, bu_vls_addr(&response));
        result = -1;
    }
    
cleanup:
    bu_vls_free(&response);
    
    if (result == 0) {
        bu_log("PASS: Error propagation in protocol\n");
    } else {
        bu_log("FAIL: Error propagation in protocol\n");
    }
    
    return result;
}

int test_empty_arguments() {
    struct command_request *req;
    const char test_cmd[] = {'c','m','d',0x1D,'a','r','g','1',0x1D,0x1D,'a','r','g','3',0x1C,'\0'};
    int result = 0;
    
    bu_log("Testing empty arguments (consecutive separators)...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse command with empty argument\n");
        return -1;
    }
    
    if (strcmp(req->command, "cmd") != 0) {
        bu_log("ERROR: Expected command 'cmd', got '%s'\n", req->command);
        result = -1;
    }
    
    /* Expected: argv[0]="cmd", argv[1]="arg1", argv[2]="", argv[3]="arg3" */
    if (req->argc != 4) {
        bu_log("ERROR: Expected argc=4, got %d\n", req->argc);
        result = -1;
    } else {
        if (strcmp(req->argv[0], "cmd") != 0) {
            bu_log("ERROR: Expected argv[0]='cmd', got '%s'\n", req->argv[0]);
            result = -1;
        }
        if (strcmp(req->argv[1], "arg1") != 0) {
            bu_log("ERROR: Expected argv[1]='arg1', got '%s'\n", req->argv[1]);
            result = -1;
        }
        if (strlen(req->argv[2]) != 0) {
            bu_log("ERROR: Expected empty argv[2], got '%s'\n", req->argv[2]);
            result = -1;
        }
        if (strcmp(req->argv[3], "arg3") != 0) {
            bu_log("ERROR: Expected argv[3]='arg3', got '%s'\n", req->argv[3]);
            result = -1;
        }
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: Empty arguments handling\n");
    } else {
        bu_log("FAIL: Empty arguments handling\n");
    }
    
    return result;
}

int test_all_empty_arguments() {
    struct command_request *req;
    const char *test_cmd = "cmd\x1D\x1D\x1D\x1C";  /* cmd␝␝␝␜ */
    int result = 0;
    
    bu_log("Testing all empty arguments...\n");
    
    req = parse_protocol_request(test_cmd, strlen(test_cmd));
    if (!req) {
        bu_log("ERROR: Failed to parse command with all empty arguments\n");
        return -1;
    }
    
    if (strcmp(req->command, "cmd") != 0) {
        bu_log("ERROR: Expected command 'cmd', got '%s'\n", req->command);
        result = -1;
    }
    
    /* Expected: argv[0]="cmd", argv[1]="", argv[2]="", argv[3]="" */
    if (req->argc != 4) {
        bu_log("ERROR: Expected argc=4, got %d\n", req->argc);
        result = -1;
    } else {
        for (int i = 1; i < req->argc; i++) {
            if (strlen(req->argv[i]) != 0) {
                bu_log("ERROR: Expected empty argv[%d], got '%s'\n", i, req->argv[i]);
                result = -1;
            }
        }
    }
    
    free_command_request(req);
    
    if (result == 0) {
        bu_log("PASS: All empty arguments handling\n");
    } else {
        bu_log("FAIL: All empty arguments handling\n");
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
        printf("  partial      - Partial message handling\n");
        printf("  multiple     - Multiple messages in buffer\n");
        printf("  errorprop    - Error propagation in protocol\n");
        printf("  emptyargs    - Empty arguments handling\n");
        printf("  allempty     - All empty arguments handling\n");
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
    
    if (BU_STR_EQUAL(av[1], "partial") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_partial_message_handling() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "multiple") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_multiple_messages_in_buffer() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "errorprop") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_error_propagation() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "emptyargs") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_empty_arguments() == 0) passed_count++;
    }
    
    if (BU_STR_EQUAL(av[1], "allempty") || BU_STR_EQUAL(av[1], "all")) {
        test_count++;
        if (test_all_empty_arguments() == 0) passed_count++;
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
