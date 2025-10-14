/*                   G U I L E _ C M D . H
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
/** @file mged/guile_cmd.h
 *
 * Header for Guile command processing in MGED
 *
 */

#ifndef MGED_GUILE_CMD_H
#define MGED_GUILE_CMD_H

#include "common.h"

__BEGIN_DECLS

/* Forward declarations */
struct mged_state;
struct bu_vls;

#ifdef HAVE_GUILE

/**
 * Evaluate a Guile expression
 *
 * @param s MGED state
 * @param vp Expression string
 * @return CMD_OK, CMD_BAD, or CMD_MORE
 */
extern int cmdline_guile(struct mged_state *s, struct bu_vls *vp);

/**
 * Process a single character of Guile input
 *
 * @param s MGED state
 * @param ch Character to process
 */
extern void guile_process_char(struct mged_state *s, char ch);

#ifdef HAVE_EDITLINE
/**
 * Interactive REPL using libedit
 *
 * @param s MGED state
 */
extern void guile_repl_libedit(struct mged_state *s);
#endif /* HAVE_EDITLINE */

#endif /* HAVE_GUILE */

__END_DECLS

#endif /* MGED_GUILE_CMD_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
