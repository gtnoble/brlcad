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
	cl_object base_str;
	
	/* Convert ECL object to string */
	if (ecl_stringp(arg)) {
	    /* Coerce to base-string, then extract C pointer */
	    base_str = si_coerce_to_base_string(arg);
	    (*argv)[i] = bu_strdup(ecl_base_string_pointer_safe(base_str));
	} else {
	    /* For non-string objects, convert to string representation */
	    cl_object str_obj = cl_princ_to_string(arg);
	    base_str = si_coerce_to_base_string(str_obj);
	    (*argv)[i] = bu_strdup(ecl_base_string_pointer_safe(base_str));
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
 * Generic wrapper for executing MGED commands from ECL.
 *
 * This function handles the common pattern of:
 * - Converting ECL arguments to C argv
 * - Prepending the command name
 * - Calling ged_exec
 * - Handling errors and returning results
 *
 * @param command_name The MGED command to execute (e.g., "ls", "draw")
 * @param narg Number of ECL arguments
 * @param args ECL variadic argument list
 * @return ECL object containing result string or NIL
 */
static cl_object
ecl_exec_mged_command(const char *command_name, cl_narg narg, cl_va_list args)
{
    struct mged_state *s;
    int argc;
    char **argv = NULL;
    char **full_argv = NULL;
    int ret;
    cl_object result = ECL_NIL;
    int i;
    
    /* Get MGED state */
    s = ecl_get_mged_state();
    if (!s || !s->gedp) {
	ecl_signal_mged_error(command_name, "MGED state not initialized", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Convert arguments */
    if (ecl_args_to_argv(narg, args, &argc, &argv) != BRLCAD_OK) {
	ecl_signal_mged_error(command_name, "Failed to convert arguments", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Prepend command name to argv for ged_exec */
    full_argv = (char **)bu_calloc(argc + 2, sizeof(char *), "full_argv");
    full_argv[0] = bu_strdup(command_name);
    for (i = 0; i < argc; i++) {
	full_argv[i + 1] = argv[i];
	argv[i] = NULL; /* Transfer ownership */
    }
    full_argv[argc + 1] = NULL;
    
    /* Free the old argv array (strings transferred to full_argv) */
    bu_free(argv, "argv");
    argv = NULL;
    
    /* Call the libged function through ged_exec */
    ret = ged_exec(s->gedp, argc + 1, (const char **)full_argv);
    
    if (ret != BRLCAD_OK) {
	/* Extract error message from ged_result_str */
	const char *err_msg = "Command failed";
	
	if (s->gedp->ged_result_str && bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	    err_msg = bu_vls_addr(s->gedp->ged_result_str);
	}
	
	ecl_free_argv(argc + 1, full_argv);
	ecl_signal_mged_error(command_name, err_msg, ret);
	/* NOTREACHED */
    }
    
    /* Success - return result or NIL */
    if (s->gedp->ged_result_str && bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	const char *result_str = bu_vls_addr(s->gedp->ged_result_str);
	result = ecl_make_simple_base_string((char *)result_str, -1);
    }
    /* else result stays as NIL */
    
    ecl_free_argv(argc + 1, full_argv);
    
    return result;
}


/**
 * Generic ECL wrapper that dispatches any MGED command.
 * 
 * The command name is passed as the first ECL argument, followed by
 * the actual command arguments. This single dispatcher handles all
 * MGED commands, eliminating the need for per-command wrapper functions.
 *
 * @param narg Number of arguments (including command name)
 * @param ... Variadic ECL arguments
 * @return Result from command or signals error
 */
cl_object
ecl_generic_mged_dispatcher(cl_narg narg, ...)
{
    cl_va_list args;
    cl_object cmd_name_obj;
    const char *command_name;
    cl_object result;
    cl_narg actual_narg;
    
    if (narg < 1) {
	ecl_signal_mged_error("dispatcher", "No command name provided", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    cl_va_start(args, narg, narg, 0);
    
    /* First argument is the command name */
    cmd_name_obj = cl_va_arg(args);
    
    if (!ecl_stringp(cmd_name_obj)) {
	cl_va_end(args);
	ecl_signal_mged_error("dispatcher", "Command name must be a string", BRLCAD_ERROR);
	/* NOTREACHED */
    }
    
    /* Coerce to base-string, then extract C pointer */
    {
	cl_object base_str = si_coerce_to_base_string(cmd_name_obj);
	command_name = ecl_base_string_pointer_safe(base_str);
    }
    
    /* Remaining arguments are the actual command arguments */
    actual_narg = narg - 1;
    
    /* Call the generic executor with the remaining args */
    result = ecl_exec_mged_command(command_name, actual_narg, args);
    
    cl_va_end(args);
    
    return result;
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
