/*                 E C L _ I N T E R F A C E . H
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
/** @file mged/ecl_interface.h
 *
 * ECL (Embeddable Common Lisp) integration for MGED.
 *
 * Provides an alternative extension language to Tcl, allowing users
 * to interact with MGED using Common Lisp via ECL's native REPL.
 */

#ifndef MGED_ECL_INTERFACE_H
#define MGED_ECL_INTERFACE_H

#include "common.h"

#ifdef HAVE_ECL

#include "mged.h"

__BEGIN_DECLS

/**
 * Start the ECL REPL for MGED.
 *
 * This function initializes ECL, registers all MGED commands as ECL
 * functions, and sets up the ECL REPL state. Unlike the original blocking
 * implementation, this function returns to allow integration with MGED's
 * event loop. Call ecl_repl_step() periodically to process ECL input.
 *
 * @param s MGED state structure containing database and view information
 */
void start_ecl_repl(struct mged_state *s);

/**
 * Process one ECL REPL iteration if input is available.
 *
 * This function should be called periodically from MGED's main event loop.
 * It checks if stdin has input available and processes one read-eval-print
 * cycle if so. This keeps the ECL REPL responsive without blocking the
 * display event processing.
 *
 * @param s MGED state structure
 * @return 1 if a command was processed, 0 if no input was available
 */
int ecl_repl_step(struct mged_state *s);

/**
 * Register all MGED commands as ECL functions.
 *
 * Called internally by start_ecl_repl() to expose MGED commands
 * to the ECL environment.
 *
 * @param s MGED state structure
 */
void ecl_register_commands(struct mged_state *s);

__END_DECLS

#endif /* HAVE_ECL */

#endif /* MGED_ECL_INTERFACE_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
