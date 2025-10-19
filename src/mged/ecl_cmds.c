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
 * Minimal C FFI layer for MGED ECL integration.
 *
 * This file provides thin C wrappers that allow Lisp code to call
 * ged_exec and tcl_func. All dispatcher logic is implemented in Lisp.
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

/* Forward declaration from setup.c */
extern struct cmdtab mged_cmdtab[];


/**
 * FFI wrapper to call ged_exec from Lisp.
 *
 * Takes a command name and list of argument strings, calls ged_exec,
 * and returns a list of (return-code result-string).
 *
 * @param cmd_name ECL string - command name
 * @param args_list ECL list of strings - command arguments
 * @return ECL list: (return-code result-string)
 */
cl_object
ecl_call_ged_exec(cl_object cmd_name, cl_object args_list)
{
    struct mged_state *s;
    cl_object state_sym, state_val;
    char **argv = NULL;
    int argc, i;
    cl_object arg, result_list;
    cl_object base_str;
    int ret;
    
    /* Get MGED state from global variable */
    state_sym = ecl_read_from_cstring("MGED::*MGED-STATE*");
    state_val = ecl_symbol_value(state_sym);
    
    if (state_val == ECL_NIL) {
	return cl_list(2,
	    ecl_make_integer(BRLCAD_ERROR),
	    ecl_make_simple_base_string("MGED state not initialized", -1));
    }
    
    s = (struct mged_state *)(uintptr_t)ecl_to_unsigned_integer(state_val);
    if (!s || !s->gedp) {
	return cl_list(2,
	    ecl_make_integer(BRLCAD_ERROR),
	    ecl_make_simple_base_string("MGED gedp not initialized", -1));
    }
    
    /* Count arguments and build argv */
    argc = 1 + ecl_to_int(cl_length(args_list));  /* +1 for command name */
    argv = (char **)bu_calloc(argc + 1, sizeof(char *), "argv");
    
    /* First arg is command name */
    base_str = si_coerce_to_base_string(cmd_name);
    argv[0] = bu_strdup(ecl_base_string_pointer_safe(base_str));
    
    /* Remaining args from list */
    i = 1;
    while (args_list != ECL_NIL && i < argc) {
	arg = ecl_car(args_list);
	base_str = si_coerce_to_base_string(arg);
	argv[i++] = bu_strdup(ecl_base_string_pointer_safe(base_str));
	args_list = ecl_cdr(args_list);
    }
    argv[argc] = NULL;
    
    /* Call ged_exec */
    ret = ged_exec(s->gedp, argc, (const char **)argv);
    
    /* Get result string */
    const char *result_str = "";
    if (s->gedp->ged_result_str && bu_vls_strlen(s->gedp->ged_result_str) > 0) {
	result_str = bu_vls_addr(s->gedp->ged_result_str);
    }
    
    /* Build result list */
    result_list = cl_list(2,
	ecl_make_integer(ret),
	ecl_make_simple_base_string((char *)result_str, -1));
    
    /* Free argv */
    for (i = 0; i < argc; i++) {
	if (argv[i])
	    bu_free(argv[i], "argv element");
    }
    bu_free(argv, "argv");
    
    return result_list;
}


/**
 * FFI wrapper to call tcl_func from Lisp.
 *
 * Takes a command name and list of argument strings, looks up the
 * command in mged_cmdtab, calls its tcl_func, and returns a list
 * of (return-code result-string).
 *
 * @param cmd_name ECL string - command name
 * @param args_list ECL list of strings - command arguments  
 * @return ECL list: (return-code result-string)
 */
cl_object
ecl_call_tcl_func(cl_object cmd_name, cl_object args_list)
{
    struct mged_state *s;
    cl_object state_sym, state_val;
    struct cmdtab *ctp;
    char **argv = NULL;
    char **saved_argv = NULL;
    int argc, i, ret;
    cl_object arg, result_list, base_str;
    const char *cmd_name_str;
    int found = 0;
    
    /* Get MGED state */
    state_sym = ecl_read_from_cstring("MGED::*MGED-STATE*");
    state_val = ecl_symbol_value(state_sym);
    
    if (state_val == ECL_NIL) {
	return cl_list(2,
	    ecl_make_integer(TCL_ERROR),
	    ecl_make_simple_base_string("MGED state not initialized", -1));
    }
    
    s = (struct mged_state *)(uintptr_t)ecl_to_unsigned_integer(state_val);
    if (!s || !s->interp) {
	return cl_list(2,
	    ecl_make_integer(TCL_ERROR),
	    ecl_make_simple_base_string("MGED Tcl interpreter not initialized", -1));
    }
    
    /* Extract command name */
    base_str = si_coerce_to_base_string(cmd_name);
    cmd_name_str = ecl_base_string_pointer_safe(base_str);
    
    /* Look up command in mged_cmdtab */
    for (ctp = mged_cmdtab; ctp->name != NULL; ctp++) {
	if (BU_STR_EQUAL(ctp->name, cmd_name_str)) {
	    found = 1;
	    break;
	}
    }
    
    if (!found) {
	char err_msg[256];
	snprintf(err_msg, sizeof(err_msg), "Command '%s' not found in mged_cmdtab", cmd_name_str);
	return cl_list(2,
	    ecl_make_integer(TCL_ERROR),
	    ecl_make_simple_base_string(err_msg, -1));
    }
    
    /* Build argv */
    argc = 1 + ecl_to_int(cl_length(args_list));
    argv = (char **)bu_calloc(argc + 1, sizeof(char *), "argv");
    
    /* First arg is command name */
    argv[0] = bu_strdup(cmd_name_str);
    
    /* Remaining args from list */
    i = 1;
    while (args_list != ECL_NIL && i < argc) {
	arg = ecl_car(args_list);
	base_str = si_coerce_to_base_string(arg);
	argv[i++] = bu_strdup(ecl_base_string_pointer_safe(base_str));
	args_list = ecl_cdr(args_list);
    }
    argv[argc] = NULL;
    
    /* Save argv pointers (tcl_func may modify them) */
    saved_argv = (char **)bu_calloc(argc + 1, sizeof(char *), "saved_argv");
    for (i = 0; i < argc; i++) {
	saved_argv[i] = argv[i];
    }
    saved_argv[argc] = NULL;
    
    /* Set up cmdtab with current state */
    ctp->s = s;
    
    /* Call tcl_func */
    ret = ctp->tcl_func((ClientData)ctp, s->interp, argc, (const char **)argv);
    
    /* Get result from Tcl interpreter */
    const char *result = Tcl_GetStringResult(s->interp);
    const char *result_str = result ? result : "";
    
    /* Build result list */
    result_list = cl_list(2,
	ecl_make_integer(ret),
	ecl_make_simple_base_string((char *)result_str, -1));
    
    /* Free saved argv (original allocations) */
    for (i = 0; i < argc; i++) {
	if (saved_argv[i])
	    bu_free(saved_argv[i], "argv element");
    }
    bu_free(saved_argv, "saved_argv");
    bu_free(argv, "argv");
    
    /* Reset Tcl result */
    Tcl_ResetResult(s->interp);
    
    return result_list;
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
