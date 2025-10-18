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
extern cl_object ecl_generic_mged_dispatcher(cl_narg narg, ...);
extern cl_object ecl_mged_cmd_dispatcher(cl_narg narg, ...);

/* Forward declaration for mged_cmdtab from setup.c */
extern struct cmdtab mged_cmdtab[];


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
 * Iterates through mged_cmdtab and dynamically creates Lisp wrapper
 * functions for each command with a ged_exec_* function. This approach
 * eliminates the need for per-command wrapper functions and automatically
 * includes new commands added to setup.c.
 *
 * All commands are registered in the MGED package to avoid conflicts with
 * Common Lisp built-in functions (e.g., DEBUG).
 */
void
ecl_register_commands(struct mged_state *s)
{
    struct cmdtab *ctp;
    int count = 0;
    struct bu_vls lisp_code = BU_VLS_INIT_ZERO;
    struct bu_vls upper_name = BU_VLS_INIT_ZERO;
    struct bu_vls exports = BU_VLS_INIT_ZERO;
    cl_object dispatcher_sym;
    size_t i;
    
    if (!s) {
	bu_log("ERROR: NULL mged_state passed to ecl_register_commands\n");
	return;
    }

    /* Build export list for MGED package - collect ALL command names */
    /* Shadow MGED commands that conflict with Common Lisp built-ins */
    bu_vls_strcpy(&exports, "(defpackage :mged (:use :cl) "
	"(:shadow #:debug #:get #:set #:time #:search #:sleep #:push #:t) "
	"(:export");
    
    /* Export ALL commands from mged_cmdtab */
    for (ctp = mged_cmdtab; ctp->name != NULL; ctp++) {
	/* Convert to uppercase and sanitize for Lisp (replace commas with hyphens) */
	bu_vls_strcpy(&upper_name, ctp->name);
	for (i = 0; i < bu_vls_strlen(&upper_name); i++) {
	    char c = bu_vls_addr(&upper_name)[i];
	    if (c >= 'a' && c <= 'z') {
		bu_vls_addr(&upper_name)[i] = c - ('a' - 'A');
	    } else if (c == ',') {
		bu_vls_addr(&upper_name)[i] = '-';  /* Replace comma with hyphen for Lisp */
	    }
	}
	
	bu_vls_printf(&exports, " #:%s", bu_vls_addr(&upper_name));
    }
    bu_vls_strcat(&exports, "))");
    
    /* Create MGED package with all command exports */
    ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	cl_eval(ecl_read_from_cstring(bu_vls_addr(&exports)));
    } ECL_CATCH_ALL_IF_CAUGHT {
	bu_log("ERROR: Failed to create MGED package\n");
	bu_vls_free(&lisp_code);
	bu_vls_free(&upper_name);
	bu_vls_free(&exports);
	return;
    } ECL_CATCH_ALL_END;
    
    /* Note: We do NOT switch to the MGED package here. This keeps users in CL-USER
     * and requires them to use fully-qualified names like (mged:ls) instead of (ls).
     * This prevents namespace pollution and follows Common Lisp best practices. */

    /* Store MGED state in ECL global variable (in MGED package) */
    {
	cl_object state_sym = ecl_read_from_cstring("MGED::*MGED-STATE*");
	cl_object state_ptr = ecl_make_unsigned_integer((uintptr_t)s);
	cl_set(state_sym, state_ptr);
    }

    /* Temporarily switch to MGED package to define dispatcher functions and commands there */
    cl_eval(ecl_read_from_cstring("(in-package :mged)"));

    /* Register the generic ged_exec dispatcher as a C function in MGED package */
    dispatcher_sym = ecl_read_from_cstring("ECL-MGED-DISPATCHER");
    ecl_def_c_function_va(dispatcher_sym, ecl_generic_mged_dispatcher, 1);

    /* Register the custom command dispatcher as a C function in MGED package */
    dispatcher_sym = ecl_read_from_cstring("ECL-MGED-CMD-DISPATCHER");
    ecl_def_c_function_va(dispatcher_sym, ecl_mged_cmd_dispatcher, 1);

    /* Iterate through mged_cmdtab and register ALL commands in MGED package */
    for (ctp = mged_cmdtab; ctp->name != NULL; ctp++) {
	const char *dispatcher_name;
	
	/* Select dispatcher based on command type */
	if (ctp->ged_func == GED_FUNC_PTR_NULL) {
	    /* Custom MGED command (uses Tcl func) */
	    dispatcher_name = "ecl-mged-cmd-dispatcher";
	} else {
	    /* Standard ged_exec command */
	    dispatcher_name = "ecl-mged-dispatcher";
	}
	
	/* Convert command name to uppercase and sanitize for Lisp (replace commas with hyphens) */
	bu_vls_strcpy(&upper_name, ctp->name);
	for (i = 0; i < bu_vls_strlen(&upper_name); i++) {
	    char c = bu_vls_addr(&upper_name)[i];
	    if (c >= 'a' && c <= 'z') {
		bu_vls_addr(&upper_name)[i] = c - ('a' - 'A');
	    } else if (c == ',') {
		bu_vls_addr(&upper_name)[i] = '-';  /* Replace comma with hyphen for Lisp */
	    }
	}
	
	/* Create a Lisp wrapper function in MGED package */
	bu_vls_sprintf(&lisp_code,
	    "(defun %s (&rest args) "
	    "  \"MGED command: %s\" "
	    "  (apply #'%s \"%s\" args))",
	    bu_vls_addr(&upper_name), ctp->name, dispatcher_name, ctp->name);
	
	/* Evaluate the Lisp code to define the function */
	ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	    cl_eval(ecl_read_from_cstring(bu_vls_addr(&lisp_code)));
	    count++;
	} ECL_CATCH_ALL_IF_CAUGHT {
	    bu_log("Warning: Failed to register command '%s'\n", ctp->name);
	} ECL_CATCH_ALL_END;
    }
    
    /* Switch back to CL-USER package so the REPL starts in the default package */
    cl_eval(ecl_read_from_cstring("(in-package :cl-user)"));
    
    bu_log("Registered %d total ECL commands in MGED package (ged_exec + custom)\n", count);
    bu_vls_free(&lisp_code);
    bu_vls_free(&upper_name);
    bu_vls_free(&exports);
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

    /* Register all MGED commands as ECL functions */
    ecl_register_commands(s);

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

    /* Restore terminal to normal mode for ECL REPL */
    /* MGED disables echo with clr_Echo() for its own command-line editing,
     * but ECL's REPL expects the terminal to echo characters normally */
#ifndef HAVE_WINDOWS_H
    reset_Tty(fileno(stdin));  /* Restore line mode and echo */
#endif

    /* Set up I/O streams properly for interactive REPL */
    /* Ensure standard streams are properly connected */
    cl_eval(ecl_read_from_cstring("(progn \
	(setf *standard-input* *terminal-io*) \
	(setf *standard-output* *terminal-io*) \
	(setf *error-output* *terminal-io*) \
	(setf *query-io* *terminal-io*) \
	(setf *debug-io* *terminal-io*))"));

    /* Define MGED-ERROR condition type for command failures */
    cl_eval(ecl_read_from_cstring(
	"(define-condition mged-error (error) "
	"  ((command :initarg :command :reader mged-error-command) "
	"   (message :initarg :message :reader mged-error-message) "
	"   (return-code :initarg :return-code :reader mged-error-return-code)) "
	"  (:report (lambda (condition stream) "
	"             (format stream \"MGED command '~A' failed: ~A\" "
	"                     (mged-error-command condition) "
	"                     (mged-error-message condition)))))"
    ));

    /* Define quit and exit functions that call MGED's proper cleanup path.
     * These call the registered MGED-QUIT function which invokes mged_finish()
     * for proper cleanup (closing database, releasing displays, etc.) before exit. */
    cl_eval(ecl_read_from_cstring(
	"(defun quit (&optional (status 0)) "
	"  \"Exit MGED with proper cleanup.\" "
	"  (declare (ignore status)) "
	"  (mged-quit))"
    ));
    
    cl_eval(ecl_read_from_cstring(
	"(defun exit (&optional (status 0)) "
	"  \"Exit MGED with proper cleanup.\" "
	"  (declare (ignore status)) "
	"  (mged-quit))"
    ));
    
    /* Define the non-blocking REPL step function that processes one command if input is ready.
     * Uses our C function stdin-ready instead of ECL's listen for reliable non-blocking check.
     * This reuses ECL's existing REPL machinery (tpl-prompt, tpl-read, eval-with-env). */
    cl_eval(ecl_read_from_cstring(
	"(defun mged-repl-step () "
	"  \"Do one REPL iteration if input is ready. Returns T if processed, NIL if no input.\" "
	"  (when (stdin-ready) "
	"    (setq +++ ++ ++ + + -) "
	"    (si::tpl-prompt) "
	"    (setq - (si::tpl-read)) "
	"    (let ((values (multiple-value-list (si::eval-with-env - si::*break-env*)))) "
	"      (setq /// // // / / values *** ** ** * * (car /)) "
	"      (format cl::t \"~&~{~S~^~%~}~%\" values)) "
	"    cl::t))"
    ));
    
    /* Initialize REPL state variables that tpl normally sets up */
    cl_eval(ecl_read_from_cstring(
	"(setq si::*tpl-level* 0 "
	"      si::*ihs-base* (si::ihs-top) "
	"      si::*ihs-top* (si::ihs-top) "
	"      si::*ihs-current* (si::ihs-top) "
	"      si::*break-env* nil)"
    ));
    
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
