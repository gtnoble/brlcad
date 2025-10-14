/*                 G U I L E _ I N I T . C
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
/** @file guile_init.c
 *
 * Guile interpreter initialization for BRL-CAD
 *
 */

#include "common.h"

#ifdef HAVE_GUILE

#include <libguile.h>
#include <string.h>

#include "bu/app.h"
#include "bu/file.h"
#include "bu/log.h"
#include "bu/path.h"
#include "bu/vls.h"
#include "ged.h"
#include "guilecad/guilecad.h"

/* Private headers */
#include "brlcad_version.h"


/* Global pointer to current GED context (for Scheme command access) */
static struct ged *current_ged = NULL;


/**
 * Get the current GED context from Guile
 */
struct ged *
guilecad_get_ged(void)
{
    return current_ged;
}


/**
 * Inner initialization function that runs within Guile context
 */
static void *
guilecad_init_inner(void *data)
{
    struct ged *gedp = (struct ged *)data;
    const char *brlcad_data;
    struct bu_vls load_path = BU_VLS_INIT_ZERO;
    
    /* Store GED pointer for command access */
    current_ged = gedp;
    
    /* Set up load paths for BRL-CAD Scheme scripts */
    brlcad_data = bu_dir(NULL, 0, BU_DIR_DATA, "guile", NULL);
    if (brlcad_data && bu_file_exists(brlcad_data, NULL)) {
        bu_vls_sprintf(&load_path, "(add-to-load-path \"%s\")", brlcad_data);
        scm_c_eval_string(bu_vls_cstr(&load_path));
    }
    bu_vls_free(&load_path);
    
    /* Register BRL-CAD command modules */
    guilecad_register_bu_commands();
    guilecad_register_bn_commands();
    guilecad_register_rt_commands();
    guilecad_register_ged_commands(gedp);
    guilecad_register_dm_commands();
    
    /* Try to load Scheme API wrapper if it exists */
    brlcad_data = bu_dir(NULL, 0, BU_DIR_DATA, "guile", "brlcad-api.scm", NULL);
    if (brlcad_data && bu_file_exists(brlcad_data, NULL)) {
        scm_c_primitive_load(brlcad_data);
    }
    
    /* Define version information */
    scm_c_define("*brlcad-version*", scm_from_locale_string(brlcad_version()));
    
    return NULL;
}


int
guilecad_init(struct ged *gedp, struct bu_vls *tlog)
{
    static int initialized = 0;
    
    /* Initialize Guile if not already initialized */
    if (!initialized) {
        scm_init_guile();
        
        /* Run initialization in Guile context */
        scm_with_guile(guilecad_init_inner, gedp);
        
        if (tlog) {
            bu_vls_printf(tlog, "Guile %s initialized for BRL-CAD\n", 
                         scm_to_locale_string(scm_version()));
        }
        
        initialized = 1;
    }
    
    return 0;
}


int
guilecad_eval(struct ged *gedp, const char *expr, struct bu_vls *result)
{
    SCM scm_result;
    SCM scm_str;
    char *result_str;
    
    if (!expr) {
        return -1;
    }
    
    /* Update current GED context */
    current_ged = gedp;
    
    /* Evaluate the expression */
    scm_result = scm_c_eval_string(expr);
    
    /* Convert result to string if output requested */
    if (result) {
        scm_str = scm_object_to_string(scm_result, SCM_UNDEFINED);
        result_str = scm_to_locale_string(scm_str);
        bu_vls_strcpy(result, result_str);
        free(result_str);
    }
    
    return 0;
}


int
guilecad_expression_complete(const char *expr)
{
    int paren_depth = 0;
    int in_string = 0;
    int escaped = 0;
    const char *p;
    
    if (!expr) {
        return 1;
    }
    
    /* Simple parenthesis balancing check */
    for (p = expr; *p; p++) {
        if (escaped) {
            escaped = 0;
            continue;
        }
        
        if (*p == '\\') {
            escaped = 1;
            continue;
        }
        
        if (*p == '"') {
            in_string = !in_string;
            continue;
        }
        
        if (in_string) {
            continue;
        }
        
        if (*p == '(') {
            paren_depth++;
        } else if (*p == ')') {
            paren_depth--;
        }
    }
    
    return (paren_depth == 0 && !in_string) ? 1 : 0;
}

#endif /* HAVE_GUILE */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
