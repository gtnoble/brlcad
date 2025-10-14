/*                    G U I L E C A D . H
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
/** @file guilecad.h
 *
 * Header for BRL-CAD's Guile interface
 *
 */

#ifndef GUILECAD_H
#define GUILECAD_H

#include "common.h"
#include "tclcad/defines.h"

__BEGIN_DECLS

/* Forward declarations */
struct ged;
struct bu_vls;

/**
 * Initialize Guile interpreter and register BRL-CAD commands
 *
 * @param gedp GED pointer for command context
 * @param tlog Optional log output (can be NULL)
 * @return 0 on success, non-zero on error
 */
TCLCAD_EXPORT extern int guilecad_init(struct ged *gedp, struct bu_vls *tlog);

/**
 * Evaluate a Scheme expression string
 *
 * @param gedp GED context
 * @param expr Scheme expression to evaluate
 * @param result Output buffer for result (can be NULL)
 * @return 0 on success, non-zero on error
 */
TCLCAD_EXPORT extern int guilecad_eval(struct ged *gedp, const char *expr, struct bu_vls *result);

/**
 * Register BRL-CAD library commands with Guile
 */
TCLCAD_EXPORT extern void guilecad_register_bu_commands(void);
TCLCAD_EXPORT extern void guilecad_register_bn_commands(void);
TCLCAD_EXPORT extern void guilecad_register_rt_commands(void);
TCLCAD_EXPORT extern void guilecad_register_ged_commands(struct ged *gedp);
TCLCAD_EXPORT extern void guilecad_register_dm_commands(void);

/**
 * Check if a Scheme expression is complete (balanced parentheses)
 *
 * @param expr Expression string to check
 * @return 1 if complete, 0 if incomplete
 */
TCLCAD_EXPORT extern int guilecad_expression_complete(const char *expr);

__END_DECLS

#endif /* GUILECAD_H */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
