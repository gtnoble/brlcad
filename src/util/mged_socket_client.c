/*                    M G E D _ S O C K E T _ C L I E N T . C
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
/** @file util/mged_socket_client.c
 *
 * Client utility for connecting to MGED socket server.
 * Supports both interactive line-based input and batch mode for scripting.
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

#ifdef HAVE_SYS_UN_H
#  include <sys/un.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "bu.h"

/* Protocol control characters */
#define PROTOCOL_FS '\x1C'  /* File Separator (message terminator) */
#define PROTOCOL_GS '\x1D'  /* Group Separator (field separator) */

static void
build_protocol_message(struct bu_vls *msg, const char *line)
{
    char *argv[256];  /* Reasonable limit for command arguments */
    char *line_copy;
    size_t argc;
    size_t i;

    bu_vls_trunc(msg, 0);

    /* bu_argv_from_string modifies the input, so make a copy */
    line_copy = bu_strdup(line);
    argc = bu_argv_from_string(argv, 255, line_copy);  /* 255 to leave room for NULL */

    if (argc == 0) {
        /* Empty command - just send FS */
        bu_vls_putc(msg, PROTOCOL_FS);
        bu_free(line_copy, "line_copy");
        return;
    }

    /* Build protocol message: command␝arg1␝arg2␝...␜ */
    for (i = 0; i < argc; i++) {
        if (i > 0) {
            bu_vls_putc(msg, PROTOCOL_GS);
        }
        bu_vls_strcat(msg, argv[i]);
    }
    bu_vls_putc(msg, PROTOCOL_FS);

    bu_free(line_copy, "line_copy");
}

static int
send_command(int sock_fd, const char *command, size_t len)
{
    ssize_t sent = 0;
    size_t total_sent = 0;

    while (total_sent < len) {
        sent = send(sock_fd, command + total_sent, len - total_sent, 0);
        if (sent < 0) {
            perror("send");
            return -1;
        }
        total_sent += (size_t)sent;
    }

    return 0;
}

static int
receive_response(int sock_fd)
{
    char buffer[8192];
    struct bu_vls response = BU_VLS_INIT_ZERO;
    int found_terminator = 0;

    /* Read until we get FS terminator */
    while (!found_terminator) {
        ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes < 0) {
            perror("recv");
            bu_vls_free(&response);
            return -1;
        }

        if (bytes == 0) {
            fprintf(stderr, "Server disconnected\n");
            bu_vls_free(&response);
            return -1;
        }

        buffer[bytes] = '\0';
        bu_vls_strncat(&response, buffer, (size_t)bytes);

        /* Check for FS terminator */
        for (ssize_t i = 0; i < bytes; i++) {
            if (buffer[i] == PROTOCOL_FS) {
                found_terminator = 1;
                break;
            }
        }
    }

    /* Parse and display response: status␝result␝error␜ */
    const char *data = bu_vls_cstr(&response);
    size_t len = bu_vls_strlen(&response);

    /* Strip trailing FS */
    if (len > 0 && data[len - 1] == PROTOCOL_FS) {
        len--;
    }

    /* Split by GS to get fields */
    struct bu_vls status = BU_VLS_INIT_ZERO;
    struct bu_vls result = BU_VLS_INIT_ZERO;
    struct bu_vls error = BU_VLS_INIT_ZERO;

    int field = 0;
    size_t field_start = 0;

    for (size_t i = 0; i <= len; i++) {
        if (i == len || data[i] == PROTOCOL_GS) {
            size_t field_len = i - field_start;
            const char *field_data = data + field_start;

            switch (field) {
            case 0:
                bu_vls_strncpy(&status, field_data, (int)field_len);
                break;
            case 1:
                bu_vls_strncpy(&result, field_data, (int)field_len);
                break;
            case 2:
                bu_vls_strncpy(&error, field_data, (int)field_len);
                break;
            }

            field++;
            field_start = i + 1;
        }
    }

    /* Display response */
    printf("\n");

    if (bu_vls_strlen(&status) > 0) {
        if (strncmp(bu_vls_cstr(&status), "OK", 2) == 0) {
            printf("Status: OK\n");
        } else {
            printf("Status: %s\n", bu_vls_cstr(&status));
        }
    }

    if (bu_vls_strlen(&result) > 0) {
        printf("Result:\n%s\n", bu_vls_cstr(&result));
    }

    if (bu_vls_strlen(&error) > 0) {
        printf("Error: %s\n", bu_vls_cstr(&error));
    }

    bu_vls_free(&status);
    bu_vls_free(&result);
    bu_vls_free(&error);
    bu_vls_free(&response);

    return 0;
}

static void
run_interactive(int sock_fd)
{
    struct bu_vls line = BU_VLS_INIT_ZERO;
    struct bu_vls command = BU_VLS_INIT_ZERO;

    printf("MGED Socket Client (Interactive Mode)\n");
    printf("======================================\n\n");
    printf("Enter MGED commands one per line. Commands are sent automatically.\n");
    printf("Special commands: 'exit', 'quit', or 'help'\n");
    printf("Press Ctrl+D (Unix) or Ctrl+Z (Windows) to exit.\n\n");

    while (1) {
        printf("mged> ");
        fflush(stdout);

        /* Clear line buffer before reading */
        bu_vls_trunc(&line, 0);

        /* Read line from stdin */
        if (bu_vls_gets(&line, stdin) < 0) {
            /* EOF or error */
            break;
        }

        bu_vls_trimspace(&line);

        if (bu_vls_strlen(&line) == 0) {
            continue;
        }

        /* Check for special commands */
        if (BU_STR_EQUAL(bu_vls_cstr(&line), "exit") || 
            BU_STR_EQUAL(bu_vls_cstr(&line), "quit")) {
            break;
        }

        if (BU_STR_EQUAL(bu_vls_cstr(&line), "help")) {
            printf("\nMGED Socket Client Help\n");
            printf("======================\n\n");
            printf("This client sends commands to an MGED socket server.\n");
            printf("Type any MGED command and press Enter to execute it.\n\n");
            printf("Examples:\n");
            printf("  ls                       - List objects\n");
            printf("  make sph.s sph 0 0 0 5   - Create a sphere\n");
            printf("  draw sph.s               - Draw the sphere\n\n");
            printf("Special commands:\n");
            printf("  exit, quit  - Exit this client\n");
            printf("  help        - Show this help\n\n");
            continue;
        }

        /* Build protocol message: command␝arg1␝arg2␝...␜ */
        build_protocol_message(&command, bu_vls_cstr(&line));

        /* Send command and receive response */
        if (send_command(sock_fd, bu_vls_cstr(&command), bu_vls_strlen(&command)) == 0) {
            receive_response(sock_fd);
        }
    }

    printf("\nGoodbye!\n");
    bu_vls_free(&line);
    bu_vls_free(&command);
}

static void
run_batch(int sock_fd)
{
    struct bu_vls line = BU_VLS_INIT_ZERO;
    struct bu_vls command = BU_VLS_INIT_ZERO;

    while (bu_vls_gets(&line, stdin) >= 0) {
        bu_vls_trimspace(&line);

        if (bu_vls_strlen(&line) == 0) {
            continue;
        }

        /* Skip comment lines */
        if (bu_vls_cstr(&line)[0] == '#') {
            continue;
        }

        /* Build protocol message: command␝arg1␝arg2␝...␜ */
        build_protocol_message(&command, bu_vls_cstr(&line));

        /* Send and receive */
        if (send_command(sock_fd, bu_vls_cstr(&command), bu_vls_strlen(&command)) == 0) {
            receive_response(sock_fd);
        }
    }

    bu_vls_free(&line);
    bu_vls_free(&command);
}

static void
print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-b] <socket_path>\n", prog);
    fprintf(stderr, "Connect to MGED socket server\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -b           Batch mode (read commands from stdin)\n");
    fprintf(stderr, "  -h           Show this help\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  Interactive mode:\n");
    fprintf(stderr, "    %s /tmp/mged.sock\n\n", prog);
    fprintf(stderr, "  Batch mode:\n");
    fprintf(stderr, "    echo 'ls' | %s -b /tmp/mged.sock\n", prog);
    fprintf(stderr, "    %s -b /tmp/mged.sock < commands.txt\n", prog);
}

int
main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un server_addr;
    const char *socket_path = NULL;
    int batch_mode = 0;
    int opt;

    bu_setprogname(argv[0]);

    /* Parse options */
    while ((opt = bu_getopt(argc, argv, "bh")) != -1) {
        switch (opt) {
        case 'b':
            batch_mode = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (bu_optind >= argc) {
        fprintf(stderr, "Error: socket path required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    socket_path = argv[bu_optind];

    /* Create socket */
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Setup server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    if (strlen(socket_path) >= sizeof(server_addr.sun_path)) {
        fprintf(stderr, "Error: socket path too long\n");
        close(sock_fd);
        return 1;
    }
    bu_strlcpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path));

    /* Connect to server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to connect to %s: %s\n", socket_path, strerror(errno));
        close(sock_fd);
        return 1;
    }

    /* Run in appropriate mode */
    if (batch_mode) {
        run_batch(sock_fd);
    } else {
        run_interactive(sock_fd);
    }

    close(sock_fd);
    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
