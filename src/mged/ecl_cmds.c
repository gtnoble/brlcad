/*                      E C L _ C M D S . C
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
/** @file mged/ecl_cmds.c
 *
 * ECL command wrapper implementations for MGED.
 *
 * This file contains the ECL wrapper functions that convert between
 * ECL's Lisp data types and C, allowing MGED commands to be called
 * from the ECL REPL.
 */

#include "common.h"

#ifdef HAVE_ECL

#include <stdlib.h>
#include <string.h>
#include <ecl/ecl.h>

#include "bio.h"
#include "bu/app.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/str.h"
#include "bu/vls.h"
#include "ged.h"

#include "./mged.h"

/* Forward declaration for edit_com function used by draw command */
extern int edit_com(struct mged_state *s, int argc, const char *argv[]);


/**
 * Retrieve the MGED state from the ECL global variable.
 *
 * @return Pointer to mged_state, or NULL on error
 */
static struct mged_state *
ecl_get_mged_state(void)
{
    cl_object state_sym = ecl_read_from_cstring("*MGED-STATE*");
    cl_object state_val = ecl_symbol_value(state_sym);
    
    if (ecl_unlikely(state_val == ECL_NIL)) {
	bu_log("ERROR: *MGED-STATE* not initialized\n");
	return NULL;
    }

    return (struct mged_state *)(uintptr_t)ecl_to_unsigned_integer(state_val);
}


/**
 * Convert ECL variadic arguments to a C argv array.
 *
 * @param narg Number of arguments
 * @param args Variadic argument list from ECL
 * @param argc Output: number of arguments
 * @param argv Output: array of C strings (caller must free)
 * @return BRLCAD_OK on success, BRLCAD_ERROR on failure
 */
static int
ecl_args_to_argv(cl_narg narg, cl_va_list args, int *argc, char ***argv)
{
    int i;
    
    *argc = narg;
    *argv = (char **)bu_calloc(narg + 1, sizeof(char *), "argv");
    
    for (i = 0; i < narg; i++) {
	cl_object arg = cl_va_arg(args);
	
	/* Convert ECL object to string */
	if (ecl_stringp(arg)) {
	    (*argv)[i] = bu_strdup(ecl_base_string_pointer_safe(arg));
	} else {
	    /* For non-string objects, convert to string representation */
	    cl_object str_obj = cl_princ_to_string(arg);
	    (*argv)[i] = bu_strdup(ecl_base_string_pointer_safe(str_obj));
	}
    }
    (*argv)[narg] = NULL;
    
    return BRLCAD_OK;
}


/**
 * Free argv array created by ecl_args_to_argv.
 */
static void
ecl_free_argv(int argc, char **argv)
{
    int i;
    
    if (!argv)
	return;
    
    for (i = 0; i < argc; i++) {
	if (argv[i])
	    bu_free(argv[i], "argv element");
    }
    bu_free(argv, "argv");
}


/**
 * Signal an MGED error condition.
 * This function does not return - it signals a Lisp condition.
 *
 * @param command_name Name of the command that failed
 * @param error_message Error message from ged_result_str
 * @param return_code Return code from ged_exec_* function
 */
static void
ecl_signal_mged_error(const char *command_name, const char *error_message, int return_code)
{
    cl_object error_type, cmd_key, msg_key, code_key;
    cl_object cmd_val, msg_val, code_val;
    
    error_type = ecl_read_from_cstring("MGED-ERROR");
    cmd_key = ecl_read_from_cstring(":COMMAND");
    msg_key = ecl_read_from_cstring(":MESSAGE");
    code_key = ecl_read_from_cstring(":RETURN-CODE");
    
    cmd_val = ecl_make_simple_base_string((char *)command_name, -1);
    msg_val = ecl_make_simple_base_string((char *)error_message, -1);
    code_val = ecl_make_integer(return_code);
    
    /* Signal the condition - this does not return */
    cl_error(7, error_type,
             cmd_key, cmd_val,
             msg_key, msg_val,
             code_key, code_val);
    /* NOTREACHED */
}


/**
 * ECL wrapper for the 'ls' command.
 *
 * Lists objects in the database. Can be called with no arguments
 * or with flags like "-a", "-l", etc.
 */
cl_object
ecl_cmd_ls(cl_narg narg, ...)
{
    struct mged_state *s;
    cl_va_list args;
    int argc;
    char **argv = NULL;
    int ret;
    cl_object result = ECL_NIL;
    
    cl_va_start(args, narg, narg, 0);
    
    /* Get MGED state */
    s = ecl_get_mged_state();
    if (!s || !s->gedp) {
	cl_va_end(args);
	ecl_signal_mged_error("LS", "MGED state not initialized", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Convert arguments */
    if (ecl_args_to_argv(narg, args, &argc, &argv) != BRLCAD_OK) {
	cl_va_end(args);
	ecl_signal_mged_error("LS", "Failed to convert arguments", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Call the libged function */
    ret = ged_exec_ls(s->gedp, argc, (const char **)argv);
    
    if (ret != BRLCAD_OK) {
	/* Error - extract message from ged_result_str with better fallback handling */
	const char *err_msg = "Unknown error";
	
	/* Check if we can get the actual error message from libged */
	if (s->gedp && s->gedp->ged_result_str && 
	    bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	    err_msg = bu_vls_addr(s->gedp->ged_result_str);
	} else if (!s->gedp) {
	    err_msg = "MGED state not initialized";
	} else if (!s->gedp->dbip) {
	    err_msg = "A database is not open!";
	}
	
	ecl_free_argv(argc, argv);
	cl_va_end(args);
	ecl_signal_mged_error("LS", err_msg, ret);
	/* NOTREACHED */
    }
    
    /* Success - return result or NIL */
    if (s->gedp->ged_result_str && bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	const char *result_str = bu_vls_addr(s->gedp->ged_result_str);
	result = ecl_make_simple_base_string((char *)result_str, -1);
    }
    /* else result stays as NIL */
    
    ecl_free_argv(argc, argv);
    cl_va_end(args);
    
    return result;
}


/**
 * ECL wrapper for the 'draw' command.
 *
 * Draws objects in the display. Requires at least one object name.
 */
cl_object
ecl_cmd_draw(cl_narg narg, ...)
{
    struct mged_state *s;
    cl_va_list args;
    int argc;
    char **argv = NULL;
    int ret;
    cl_object result = ECL_NIL;
    
    cl_va_start(args, narg, narg, 0);
    
    /* Validate arguments - draw requires at least one object name */
    if (narg < 1) {
	cl_va_end(args);
	ecl_signal_mged_error("DRAW", "Usage: (draw object-name [object-name ...])", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Get MGED state */
    s = ecl_get_mged_state();
    if (!s || !s->gedp) {
	cl_va_end(args);
	ecl_signal_mged_error("DRAW", "MGED state not initialized", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Convert arguments */
    if (ecl_args_to_argv(narg, args, &argc, &argv) != BRLCAD_OK) {
	cl_va_end(args);
	ecl_signal_mged_error("DRAW", "Failed to convert arguments", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Call the actual cmd_draw function through edit_com */
    ret = edit_com(s, argc, (const char **)argv);
    
    if (ret != 0) {
	/* Error - extract message from ged_result_str with better fallback handling */
	const char *err_msg = "Draw command failed";
	
	/* Check if we can get the actual error message from libged */
	if (s->gedp && s->gedp->ged_result_str && 
	    bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	    err_msg = bu_vls_addr(s->gedp->ged_result_str);
	} else if (!s->gedp) {
	    err_msg = "MGED state not initialized";
	} else if (!s->gedp->dbip) {
	    err_msg = "A database is not open!";
	}
	
	ecl_free_argv(argc, argv);
	cl_va_end(args);
	ecl_signal_mged_error("DRAW", err_msg, ret);
	/* NOTREACHED */
    }
    
    /* Success - return result or NIL */
    if (s->gedp->ged_result_str && bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	const char *result_str = bu_vls_addr(s->gedp->ged_result_str);
	result = ecl_make_simple_base_string((char *)result_str, -1);
    }
    /* else result stays as NIL */
    
    ecl_free_argv(argc, argv);
    cl_va_end(args);
    
    return result;
}


/**
 * ECL wrapper for the 'quit' command.
 *
 * Exits MGED. This is already defined in ecl_interface.c but
 * included here for completeness.
 */
cl_object
ecl_cmd_quit(cl_narg narg, ...)
{
    (void)narg; /* Unused */
    
    /* Exit ECL REPL which will trigger mged exit */
    cl_object quit_fn = ecl_read_from_cstring("SI::QUIT");
    cl_funcall(1, quit_fn);
    
    return ECL_NIL;
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
