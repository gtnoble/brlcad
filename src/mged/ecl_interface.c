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

#include "bio.h"
#include "bu/app.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/str.h"
#include "ged.h"

#include "./mged.h"
#include "./cmd.h"

/* Forward declarations for command wrappers from ecl_cmds.c */
extern cl_object ecl_cmd_ls(cl_narg narg, ...);
extern cl_object ecl_cmd_draw(cl_narg narg, ...);
extern cl_object ecl_cmd_quit(cl_narg narg, ...);


/**
 * Register all MGED commands as ECL functions.
 *
 * This function iterates through the mged_cmdtab and registers each
 * command as an ECL function that can be called from the REPL.
 */
void
ecl_register_commands(struct mged_state *s)
{
    struct cmdtab *ctp;
    struct bu_vls ecl_name = BU_VLS_INIT_ZERO;
    
    if (!s) {
	bu_log("ERROR: NULL mged_state passed to ecl_register_commands\n");
	return;
    }

    /* Store MGED state in ECL global variable for access by command wrappers */
    cl_object state_sym = ecl_read_from_cstring("*MGED-STATE*");
    cl_object state_ptr = ecl_make_unsigned_integer((uintptr_t)s);
    cl_set(state_sym, state_ptr);

    /* Register initial set of commands - more will be added in Phase 5 */
    
    /* Register 'ls' command */
    cl_def_c_function(
	ecl_read_from_cstring("LS"),
	(cl_objectfn)ecl_cmd_ls,
	0,  /* min args */
	ECL_CALL_ARGUMENTS_LIMIT  /* max args (variadic) */
    );
    
    /* Register 'draw' command */
    cl_def_c_function(
	ecl_read_from_cstring("DRAW"),
	(cl_objectfn)ecl_cmd_draw,
	0,  /* min args */
	ECL_CALL_ARGUMENTS_LIMIT  /* max args (variadic) */
    );
    
    /* Register 'quit' command */
    cl_def_c_function(
	ecl_read_from_cstring("QUIT"),
	(cl_objectfn)ecl_cmd_quit,
	0,  /* min args */
	0   /* max args */
    );
    
    bu_log("Registered %d ECL commands\n", 3);
    bu_vls_free(&ecl_name);
}


/**
 * Start the ECL REPL for MGED.
 *
 * This function initializes ECL, registers all MGED commands, and
 * starts ECL's native REPL (si::tpl). When the user quits the REPL,
 * this function exits the entire mged application.
 */
void
start_ecl_repl(struct mged_state *s)
{
    char *argv[] = {"mged", NULL};
    cl_object tpl_fn;
    
    if (!s) {
	bu_log("ERROR: NULL mged_state passed to start_ecl_repl\n");
	exit(1);
    }

    bu_log("Starting ECL REPL...\n");
    bu_log("ECL REPL - Type (quit) or (exit) to exit\n");

    /* Initialize ECL runtime */
    cl_boot(1, argv);

    /* Register all MGED commands as ECL functions */
    ecl_register_commands(s);

    /* Set a custom prompt for the REPL */
    cl_object prompt_hook = ecl_read_from_cstring("*TPL-PROMPT-HOOK*");
    cl_object prompt_str = ecl_make_simple_base_string("mged> ", -1);
    cl_set(prompt_hook, prompt_str);

    /* Define quit and exit functions to cleanly exit */
    ecl_eval_from_cstring(
	"(defun quit () "
	"  \"Exit MGED\" "
	"  (si::quit))"
    );
    
    ecl_eval_from_cstring(
	"(defun exit () "
	"  \"Exit MGED\" "
	"  (si::quit))"
    );

    /* Start ECL's native REPL - this will block until user quits */
    tpl_fn = ecl_read_from_cstring("SI::TPL");
    ECL_CATCH_ALL_BEGIN(ecl_process_env()) {
	cl_funcall(1, tpl_fn);
    } ECL_CATCH_ALL_IF_CAUGHT {
	bu_log("ECL REPL exited with an error\n");
    } ECL_CATCH_ALL_END;

    /* Shutdown ECL */
    bu_log("Shutting down ECL...\n");
    cl_shutdown();

    /* Exit mged */
    exit(0);
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
