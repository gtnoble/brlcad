/*                 E C L _ I N T E R F A C E . C
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
/** @file mged/ecl_interface.c
 *
 * ECL (Embeddable Common Lisp) integration for MGED.
 *
 * This file provides the implementation of ECL integration, allowing
 * MGED to be used with Common Lisp via ECL's native REPL as an
 * alternative to the Tcl extension language.
 */

#include "common.h"

#ifdef HAVE_ECL

#include <stdlib.h>
#include <string.h>
#include <ecl/ecl.h>

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef HAVE_WINDOWS_H
#  include <conio.h>  /* for _kbhit() */
#endif

#include "bio.h"
#include "bu/app.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/str.h"
#include "ged.h"

#ifndef HAVE_WINDOWS_H
#  include "libtermio.h"
#endif

#include "./mged.h"
#include "./cmd.h"

/* Forward declaration for mged_finish from mged.c */
extern void mged_finish(struct mged_state *s, int exitcode);

/* Forward declarations from ecl_cmds.c */
extern cl_object ecl_call_ged_exec(cl_object cmd_name, cl_object args_list);
extern cl_object ecl_call_tcl_func(cl_object cmd_name, cl_object args_list);

/* Forward declaration for mged_cmdtab from setup.c */
extern struct cmdtab mged_cmdtab[];

/* Forward declarations for ECL-generated init functions from compiled Lisp files */
extern void init_mged_repl(cl_object);
extern void init_mged_init(cl_object);
extern void init_mged_commands(cl_object);
extern void init_mged_api(cl_object);


/**
 * Retrieve the MGED state from the ECL global variable.
 *
 * @return Pointer to mged_state, or NULL on error
 */
static struct mged_state *
ecl_get_mged_state(void)
{
    cl_object state_sym = ecl_read_from_cstring("MGED::*MGED-STATE*");
    cl_object state_val = ecl_symbol_value(state_sym);
    
    if (ecl_unlikely(state_val == ECL_NIL)) {
	bu_log("ERROR: *MGED-STATE* not initialized\n");
	return NULL;
    }

    return (struct mged_state *)(uintptr_t)ecl_to_unsigned_integer(state_val);
}


/**
 * Get the count of commands in mged_cmdtab.
 *
 * @return Number of commands (not including NULL terminator)
 */
static cl_object
ecl_cmdtab_count(void)
{
    struct cmdtab *ctp;
    int count = 0;
    
    for (ctp = mged_cmdtab; ctp->name != NULL; ctp++) {
	count++;
    }
    
    return ecl_make_integer(count);
}


/**
 * Get the name of a command at the specified index.
 *
 * @param index_obj ECL integer object specifying the index
 * @return ECL string containing the command name, or NIL if out of bounds
 */
static cl_object
ecl_cmdtab_get_name(cl_object index_obj)
{
    int index;
    struct cmdtab *ctp;
    
    if (!cl_integerp(index_obj)) {
	return ECL_NIL;
    }
    
    index = ecl_to_int(index_obj);
    if (index < 0) {
	return ECL_NIL;
    }
    
    /* Navigate to the specified index */
    ctp = &mged_cmdtab[index];
    
    /* Check if we're past the end of the table */
    if (ctp->name == NULL) {
	return ECL_NIL;
    }
    
    return ecl_make_simple_base_string((char *)ctp->name, -1);
}


/**
 * Check if a command at the specified index is a custom command.
 *
 * @param index_obj ECL integer object specifying the index
 * @return ECL T if custom command (ged_func is NULL), NIL otherwise
 */
static cl_object
ecl_cmdtab_is_custom(cl_object index_obj)
{
    int index;
    struct cmdtab *ctp;
    
    if (!cl_integerp(index_obj)) {
	return ECL_NIL;
    }
    
    index = ecl_to_int(index_obj);
    if (index < 0) {
	return ECL_NIL;
    }
    
    /* Navigate to the specified index */
    ctp = &mged_cmdtab[index];
    
    /* Check if we're past the end of the table */
    if (ctp->name == NULL) {
	return ECL_NIL;
    }
    
    /* Return T if ged_func is NULL (custom command), NIL otherwise */
    return (ctp->ged_func == GED_FUNC_PTR_NULL) ? ECL_T : ECL_NIL;
}


/**
 * Check if stdin has data available without blocking.
 * This function uses select() on Unix and _kbhit() on Windows.
 *
 * @return ECL T if input is available, NIL otherwise
 */
static cl_object
ecl_stdin_ready(void)
{
#ifdef HAVE_WINDOWS_H
    /* Windows: use _kbhit() to check for keyboard input */
    if (_kbhit()) {
	return ECL_T;
    }
    return ECL_NIL;
#else
    /* Unix: use select() with zero timeout for non-blocking check */
    fd_set read_fds;
    struct timeval timeout;
    int result;
    
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    
    /* Zero timeout = return immediately */
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    result = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
    
    if (result > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
	return ECL_T;  /* Input is available */
    }
    
    return ECL_NIL;  /* No input available or error */
#endif
}


/**
 * ECL wrapper for quit/exit that properly cleans up MGED.
 * This function is registered as an ECL function and callable from Lisp.
 *
 * @return Never returns
 */
static cl_object
ecl_quit_wrapper(void)
{
    struct mged_state *s;
    
    s = ecl_get_mged_state();
    if (!s) {
	bu_log("ERROR: NULL mged_state in ecl_quit_wrapper\n");
	exit(1);
    }
    
    /* Call MGED's proper cleanup and exit */
    mged_finish(s, 0);
    /* NOTREACHED - mged_finish calls Tcl_Exit */
    
    return ECL_NIL;
}


/**
 * Process one ECL REPL iteration if input is available.
 * Called from MGED's main event loop to keep the REPL responsive.
 *
 * @param s The MGED state
 * @return 1 if a command was processed, 0 if no input available
 */
int
ecl_repl_step(struct mged_state *s)
{
    cl_object result;
    
    if (!s) {
	return 0;
    }
    
    /* Call the Lisp function that does one REPL step */
    ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	result = cl_eval(ecl_read_from_cstring("(mged-repl-step)"));
    } ECL_CATCH_ALL_IF_CAUGHT {
	bu_log("Error in ECL REPL step\n");
	return 0;
    } ECL_CATCH_ALL_END;
    
    /* Result is T if we processed something, NIL if no input */
    return (result != ECL_NIL) ? 1 : 0;
}


/**
 * Register all MGED commands as ECL functions.
 *
 * Delegates to Lisp code which uses C helper functions to iterate through
 * mged_cmdtab and register all commands. This approach keeps the complex
 * package management and function generation logic in Lisp.
 *
 * All commands are registered in the MGED package to avoid conflicts with
 * Common Lisp built-in functions (e.g., DEBUG).
 */
void
ecl_register_commands(struct mged_state *s)
{
    cl_object state_ptr;
    cl_object result;
    int count;
    
    if (!s) {
	bu_log("ERROR: NULL mged_state passed to ecl_register_commands\n");
	return;
    }

    /* Register the C FFI functions in CL-USER package */
    ecl_def_c_function(
	ecl_read_from_cstring("CL-USER::CALL-GED-EXEC"),
	(cl_objectfn_fixed)ecl_call_ged_exec,
	2);  /* 2 arguments: cmd_name, args_list */
    ecl_def_c_function(
	ecl_read_from_cstring("CL-USER::CALL-TCL-FUNC"),
	(cl_objectfn_fixed)ecl_call_tcl_func,
	2);  /* 2 arguments: cmd_name, args_list */

    /* Convert state pointer to ECL object */
    state_ptr = ecl_make_unsigned_integer((uintptr_t)s);
    
    /* Call Lisp function to set up MGED environment and register all commands */
    ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	result = cl_funcall(2,
	    ecl_read_from_cstring("CL-USER::SETUP-MGED-ECL"),
	    state_ptr);
	count = ecl_to_int(result);
	bu_log("Registered %d ECL commands in MGED package\n", count);
    } ECL_CATCH_ALL_IF_CAUGHT {
	bu_log("ERROR: Failed to register MGED commands\n");
    } ECL_CATCH_ALL_END;
}


/**
 * Start the ECL REPL for MGED.
 *
 * This function initializes ECL, registers all MGED commands, and
 * starts ECL's native REPL (si::top-level). When the user quits the REPL,
 * this function exits the entire mged application.
 */
void
start_ecl_repl(struct mged_state *s)
{
    char *argv[] = {"mged", NULL};
    
    if (!s) {
	bu_log("ERROR: NULL mged_state passed to start_ecl_repl\n");
	exit(1);
    }

    bu_log("Starting ECL REPL...\n");
    bu_log("ECL REPL - Type (quit) or (exit) to exit\n");

    /* Initialize ECL runtime with increased stack size */
    cl_boot(1, argv);
    
    /* Increase ECL stack size to prevent overflow (32MB) */
    cl_eval(ecl_read_from_cstring("(si::set-limit 'c-stack 33554432)"));
    cl_eval(ecl_read_from_cstring("(si::set-limit 'lisp-stack 33554432)"));

    /* Register C helper functions that Lisp will use */
    
    /* Register the quit wrapper as a callable ECL function */
    ecl_def_c_function(
	ecl_read_from_cstring("MGED-QUIT"),
	(cl_objectfn_fixed)ecl_quit_wrapper,
	0  /* 0 arguments */
    );

    /* Register the stdin-ready checker as a callable ECL function */
    ecl_def_c_function(
	ecl_read_from_cstring("STDIN-READY"),
	(cl_objectfn_fixed)ecl_stdin_ready,
	0  /* 0 arguments */
    );
    
    /* Register cmdtab helper functions for Lisp */
    ecl_def_c_function(
	ecl_read_from_cstring("CMDTAB-COUNT"),
	(cl_objectfn_fixed)ecl_cmdtab_count,
	0  /* 0 arguments */
    );
    ecl_def_c_function(
	ecl_read_from_cstring("CMDTAB-GET-NAME"),
	(cl_objectfn_fixed)ecl_cmdtab_get_name,
	1  /* 1 argument: index */
    );
    ecl_def_c_function(
	ecl_read_from_cstring("CMDTAB-IS-CUSTOM"),
	(cl_objectfn_fixed)ecl_cmdtab_is_custom,
	1  /* 1 argument: index */
    );

    /* Restore terminal to normal mode for ECL REPL */
    /* MGED disables echo with clr_Echo() for its own command-line editing,
     * but ECL's REPL expects the terminal to echo characters normally */
#ifndef HAVE_WINDOWS_H
    reset_Tty(fileno(stdin));  /* Restore line mode and echo */
#endif

    /* Load compiled Lisp modules FIRST - they define functions like SETUP-MGED-ECL.
     * These files are compiled at build time and linked into the binary.
     * We must call the C-level init functions that ECL generated during compilation.
     * This registers all Lisp code (functions, variables, conditions, etc.) with ECL. */
    ecl_init_module(NULL, init_mged_init);
    ecl_init_module(NULL, init_mged_commands);
    ecl_init_module(NULL, init_mged_repl);
    ecl_init_module(NULL, init_mged_api);

    /* Now register all MGED commands as ECL functions (calls SETUP-MGED-ECL in Lisp) */
    ecl_register_commands(s);
    
    /* Initialize MGED environment (I/O streams, REPL state, conditions, quit/exit functions) */
    ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	cl_funcall(1, ecl_read_from_cstring("INIT-MGED-ENVIRONMENT"));
    } ECL_CATCH_ALL_IF_CAUGHT {
	bu_log("ERROR: Failed to initialize MGED environment\n");
	exit(1);
    } ECL_CATCH_ALL_END;
    
    /* Print initial prompt */
    cl_eval(ecl_read_from_cstring("(si::tpl-prompt)"));
    
    /* Note: We DON'T call si::top-level here. Instead, we return to mged.c's
     * main event loop which will call mged-repl-step periodically to process
     * ECL input while keeping the display responsive. */
}

#endif /* HAVE_ECL */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
