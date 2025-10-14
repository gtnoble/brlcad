/*            G U I L E _ C O M M A N D S . C
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
/** @file guile_commands.c
 *
 * Register BRL-CAD commands with Guile
 *
 */

#include "common.h"

#ifdef HAVE_GUILE

#include <libguile.h>
#include <string.h>

#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/vls.h"
#include "bn.h"
#include "ged.h"
#include "guilecad/guilecad.h"


/* Forward declaration */
extern struct ged *guilecad_get_ged(void);


/**
 * Convert Scheme list to C argv array
 */
static const char **
scm_list_to_argv(SCM list, int *argc_out)
{
    const char **argv;
    int argc;
    int i;
    SCM item;
    
    argc = scm_to_int(scm_length(list));
    argv = (const char **)bu_calloc(argc + 1, sizeof(char *), "argv");
    
    for (i = 0; i < argc; i++) {
        item = scm_list_ref(list, scm_from_int(i));
        if (scm_is_string(item)) {
            argv[i] = scm_to_locale_string(item);
        } else {
            /* Convert non-strings to strings */
            SCM str = scm_object_to_string(item, SCM_UNDEFINED);
            argv[i] = scm_to_locale_string(str);
        }
    }
    argv[argc] = NULL;
    
    *argc_out = argc;
    return argv;
}


/**
 * Free argv array created by scm_list_to_argv
 */
static void
free_argv(const char **argv, int argc)
{
    int i;
    
    if (!argv) {
        return;
    }
    
    for (i = 0; i < argc; i++) {
        if (argv[i]) {
            free((void *)argv[i]);
        }
    }
    bu_free((void *)argv, "argv");
}


/**
 * Generic GED command wrapper
 */
static SCM
scm_ged_command(const char *cmd_name, SCM args)
{
    struct ged *gedp = guilecad_get_ged();
    const char **argv;
    const char **full_argv;
    int argc;
    int ret;
    SCM result;
    const char *err_msg;
    
    /* Convert Scheme args to C argv */
    argv = scm_list_to_argv(args, &argc);
    
    /* Prepend command name */
    full_argv = (const char **)bu_calloc(argc + 2, sizeof(char *), "full_argv");
    full_argv[0] = cmd_name;
    for (int i = 0; i < argc; i++) {
        full_argv[i + 1] = argv[i];
    }
    full_argv[argc + 1] = NULL;
    
    /* Execute GED command - let it handle NULL gedp */
    ret = ged_exec(gedp, argc + 1, full_argv);
    
    /* Get result string */
    if (gedp && bu_vls_strlen(gedp->ged_result_str) > 0) {
        result = scm_from_locale_string(bu_vls_cstr(gedp->ged_result_str));
    } else {
        result = (ret == BRLCAD_OK) ? SCM_BOOL_T : SCM_BOOL_F;
    }
    
    /* Cleanup */
    free_argv(argv, argc);
    bu_free(full_argv, "full_argv");
    
    /* Handle errors - get the actual error message */
    if (ret != BRLCAD_OK && ret != GED_HELP) {
        if (gedp && bu_vls_strlen(gedp->ged_result_str) > 0) {
            err_msg = bu_vls_cstr(gedp->ged_result_str);
        } else if (!gedp) {
            err_msg = "No database open";
        } else {
            err_msg = "Command failed";
        }
        scm_misc_error(cmd_name, err_msg, SCM_EOL);
    }
    
    return result;
}


/* Individual command wrappers */

static SCM
scm_ged_draw(SCM args)
{
    return scm_ged_command("draw", args);
}


static SCM
scm_ged_erase(SCM args)
{
    return scm_ged_command("erase", args);
}


static SCM
scm_ged_blast(SCM args)
{
    return scm_ged_command("blast", args);
}


static SCM
scm_ged_kill(SCM args)
{
    return scm_ged_command("kill", args);
}


static SCM
scm_ged_ls(SCM args)
{
    return scm_ged_command("ls", args);
}


static SCM
scm_ged_view(SCM args)
{
    return scm_ged_command("view", args);
}


static SCM
scm_ged_size(SCM args)
{
    return scm_ged_command("size", args);
}


static SCM
scm_ged_center(SCM args)
{
    return scm_ged_command("center", args);
}


static SCM
scm_ged_autoview(SCM args)
{
    return scm_ged_command("autoview", args);
}


static SCM
scm_ged_ae(SCM args)
{
    return scm_ged_command("ae", args);
}


static SCM
scm_ged_search(SCM args)
{
    return scm_ged_command("search", args);
}


static SCM
scm_ged_make(SCM args)
{
    return scm_ged_command("make", args);
}


static SCM
scm_ged_in(SCM args)
{
    return scm_ged_command("in", args);
}


static SCM
scm_ged_cp(SCM args)
{
    return scm_ged_command("cp", args);
}


static SCM
scm_ged_mv(SCM args)
{
    return scm_ged_command("mv", args);
}


static SCM
scm_ged_g(SCM args)
{
    return scm_ged_command("g", args);
}


static SCM
scm_ged_r(SCM args)
{
    return scm_ged_command("r", args);
}


static SCM
scm_ged_comb(SCM args)
{
    return scm_ged_command("comb", args);
}


static SCM
scm_ged_region(SCM args)
{
    return scm_ged_command("region", args);
}


static SCM
scm_ged_group(SCM args)
{
    return scm_ged_command("group", args);
}


static SCM
scm_ged_rm(SCM args)
{
    return scm_ged_command("rm", args);
}


static SCM
scm_ged_killall(SCM args)
{
    return scm_ged_command("killall", args);
}


static SCM
scm_ged_killtree(SCM args)
{
    return scm_ged_command("killtree", args);
}


static SCM
scm_ged_zap(SCM args)
{
    return scm_ged_command("zap", args);
}


static SCM
scm_ged_tree(SCM args)
{
    return scm_ged_command("tree", args);
}


static SCM
scm_ged_tops(SCM args)
{
    return scm_ged_command("tops", args);
}


static SCM
scm_ged_who(SCM args)
{
    return scm_ged_command("who", args);
}


static SCM
scm_ged_get(SCM args)
{
    return scm_ged_command("get", args);
}


static SCM
scm_ged_put(SCM args)
{
    return scm_ged_command("put", args);
}


static SCM
scm_ged_attr(SCM args)
{
    return scm_ged_command("attr", args);
}


static SCM
scm_ged_title(SCM args)
{
    return scm_ged_command("title", args);
}


static SCM
scm_ged_units(SCM args)
{
    return scm_ged_command("units", args);
}


static SCM
scm_ged_summary(SCM args)
{
    return scm_ged_command("summary", args);
}


static SCM
scm_ged_B(SCM args)
{
    return scm_ged_command("B", args);
}


static SCM
scm_ged_bb(SCM args)
{
    return scm_ged_command("bb", args);
}


static SCM
scm_ged_analyze(SCM args)
{
    return scm_ged_command("analyze", args);
}


static SCM
scm_ged_check(SCM args)
{
    return scm_ged_command("check", args);
}


static SCM
scm_ged_heal(SCM args)
{
    return scm_ged_command("heal", args);
}


static SCM
scm_ged_tra(SCM args)
{
    return scm_ged_command("tra", args);
}


static SCM
scm_ged_rot(SCM args)
{
    return scm_ged_command("rot", args);
}


static SCM
scm_ged_scale(SCM args)
{
    return scm_ged_command("scale", args);
}


static SCM
scm_ged_mirror(SCM args)
{
    return scm_ged_command("mirror", args);
}


static SCM
scm_ged_rotate(SCM args)
{
    return scm_ged_command("rotate", args);
}


static SCM
scm_ged_translate(SCM args)
{
    return scm_ged_command("translate", args);
}


static SCM
scm_ged_orotate(SCM args)
{
    return scm_ged_command("orotate", args);
}


static SCM
scm_ged_oscale(SCM args)
{
    return scm_ged_command("oscale", args);
}


static SCM
scm_ged_otranslate(SCM args)
{
    return scm_ged_command("otranslate", args);
}


static SCM
scm_ged_push(SCM args)
{
    return scm_ged_command("push", args);
}


static SCM
scm_ged_xpush(SCM args)
{
    return scm_ged_command("xpush", args);
}


static SCM
scm_ged_pull(SCM args)
{
    return scm_ged_command("pull", args);
}


static SCM
scm_ged_color(SCM args)
{
    return scm_ged_command("color", args);
}


static SCM
scm_ged_shader(SCM args)
{
    return scm_ged_command("shader", args);
}


static SCM
scm_ged_mater(SCM args)
{
    return scm_ged_command("mater", args);
}


static SCM
scm_ged_edcomb(SCM args)
{
    return scm_ged_command("edcomb", args);
}


static SCM
scm_ged_arot(SCM args)
{
    return scm_ged_command("arot", args);
}


static SCM
scm_ged_vrot(SCM args)
{
    return scm_ged_command("vrot", args);
}


static SCM
scm_ged_zoom(SCM args)
{
    return scm_ged_command("zoom", args);
}


static SCM
scm_ged_slew(SCM args)
{
    return scm_ged_command("slew", args);
}


static SCM
scm_ged_perspective(SCM args)
{
    return scm_ged_command("perspective", args);
}


static SCM
scm_ged_eye_pos(SCM args)
{
    return scm_ged_command("eye_pos", args);
}


static SCM
scm_ged_lookat(SCM args)
{
    return scm_ged_command("lookat", args);
}


static SCM
scm_ged_orientation(SCM args)
{
    return scm_ged_command("orientation", args);
}


static SCM
scm_ged_ypr(SCM args)
{
    return scm_ged_command("ypr", args);
}


static SCM
scm_ged_rt(SCM args)
{
    return scm_ged_command("rt", args);
}


static SCM
scm_ged_rtcheck(SCM args)
{
    return scm_ged_command("rtcheck", args);
}


static SCM
scm_ged_rtabort(SCM args)
{
    return scm_ged_command("rtabort", args);
}


static SCM
scm_ged_overlay(SCM args)
{
    return scm_ged_command("overlay", args);
}


static SCM
scm_ged_plot(SCM args)
{
    return scm_ged_command("plot", args);
}


static SCM
scm_ged_png(SCM args)
{
    return scm_ged_command("png", args);
}


static SCM
scm_ged_saveview(SCM args)
{
    return scm_ged_command("saveview", args);
}


static SCM
scm_ged_loadview(SCM args)
{
    return scm_ged_command("loadview", args);
}


static SCM
scm_ged_dbconcat(SCM args)
{
    return scm_ged_command("dbconcat", args);
}


static SCM
scm_ged_keep(SCM args)
{
    return scm_ged_command("keep", args);
}


static SCM
scm_ged_dump(SCM args)
{
    return scm_ged_command("dump", args);
}


static SCM
scm_ged_cat(SCM args)
{
    return scm_ged_command("cat", args);
}


static SCM
scm_ged_find(SCM args)
{
    return scm_ged_command("find", args);
}


static SCM
scm_ged_which(SCM args)
{
    return scm_ged_command("which", args);
}


static SCM
scm_ged_pathlist(SCM args)
{
    return scm_ged_command("pathlist", args);
}


static SCM
scm_ged_bot(SCM args)
{
    return scm_ged_command("bot", args);
}


static SCM
scm_ged_brep(SCM args)
{
    return scm_ged_command("brep", args);
}


static SCM
scm_ged_nmg(SCM args)
{
    return scm_ged_command("nmg", args);
}


static SCM
scm_ged_dsp(SCM args)
{
    return scm_ged_command("dsp", args);
}


static SCM
scm_ged_pipe(SCM args)
{
    return scm_ged_command("pipe", args);
}


static SCM
scm_ged_arb(SCM args)
{
    return scm_ged_command("arb", args);
}


static SCM
scm_ged_tol(SCM args)
{
    return scm_ged_command("tol", args);
}


static SCM
scm_ged_prefix(SCM args)
{
    return scm_ged_command("prefix", args);
}


static SCM
scm_ged_clone(SCM args)
{
    return scm_ged_command("clone", args);
}


static SCM
scm_ged_dup(SCM args)
{
    return scm_ged_command("dup", args);
}


static SCM
scm_ged_instance(SCM args)
{
    return scm_ged_command("instance", args);
}


static SCM
scm_quit(void)
{
    struct ged *gedp = guilecad_get_ged();
    
    /* Close database if open */
    if (gedp) {
        ged_close(gedp);
    }
    
    exit(0);
    /* NOTREACHED */
}


static SCM
scm_exit(void)
{
    struct ged *gedp = guilecad_get_ged();
    
    /* Close database if open */
    if (gedp) {
        ged_close(gedp);
    }
    
    exit(0);
    /* NOTREACHED */
}


void
guilecad_register_ged_commands(struct ged *UNUSED(gedp))
{
    /* Display Commands */
    scm_c_define_gsubr("draw", 0, 0, 1, (scm_t_subr)scm_ged_draw);
    scm_c_define_gsubr("erase", 0, 0, 1, (scm_t_subr)scm_ged_erase);
    scm_c_define_gsubr("blast", 0, 0, 1, (scm_t_subr)scm_ged_blast);
    scm_c_define_gsubr("zap", 0, 0, 1, (scm_t_subr)scm_ged_zap);
    scm_c_define_gsubr("B", 0, 0, 1, (scm_t_subr)scm_ged_B);
    scm_c_define_gsubr("who", 0, 0, 1, (scm_t_subr)scm_ged_who);
    
    /* Object Manipulation */
    scm_c_define_gsubr("kill", 0, 0, 1, (scm_t_subr)scm_ged_kill);
    scm_c_define_gsubr("killall", 0, 0, 1, (scm_t_subr)scm_ged_killall);
    scm_c_define_gsubr("killtree", 0, 0, 1, (scm_t_subr)scm_ged_killtree);
    scm_c_define_gsubr("rm", 0, 0, 1, (scm_t_subr)scm_ged_rm);
    scm_c_define_gsubr("cp", 0, 0, 1, (scm_t_subr)scm_ged_cp);
    scm_c_define_gsubr("mv", 0, 0, 1, (scm_t_subr)scm_ged_mv);
    scm_c_define_gsubr("clone", 0, 0, 1, (scm_t_subr)scm_ged_clone);
    scm_c_define_gsubr("dup", 0, 0, 1, (scm_t_subr)scm_ged_dup);
    scm_c_define_gsubr("mirror", 0, 0, 1, (scm_t_subr)scm_ged_mirror);
    scm_c_define_gsubr("instance", 0, 0, 1, (scm_t_subr)scm_ged_instance);
    
    /* Geometry Creation */
    scm_c_define_gsubr("make", 0, 0, 1, (scm_t_subr)scm_ged_make);
    scm_c_define_gsubr("in", 0, 0, 1, (scm_t_subr)scm_ged_in);
    scm_c_define_gsubr("g", 0, 0, 1, (scm_t_subr)scm_ged_g);
    scm_c_define_gsubr("r", 0, 0, 1, (scm_t_subr)scm_ged_r);
    scm_c_define_gsubr("comb", 0, 0, 1, (scm_t_subr)scm_ged_comb);
    scm_c_define_gsubr("region", 0, 0, 1, (scm_t_subr)scm_ged_region);
    scm_c_define_gsubr("group", 0, 0, 1, (scm_t_subr)scm_ged_group);
    
    /* Transformations */
    scm_c_define_gsubr("tra", 0, 0, 1, (scm_t_subr)scm_ged_tra);
    scm_c_define_gsubr("rot", 0, 0, 1, (scm_t_subr)scm_ged_rot);
    scm_c_define_gsubr("scale", 0, 0, 1, (scm_t_subr)scm_ged_scale);
    scm_c_define_gsubr("rotate", 0, 0, 1, (scm_t_subr)scm_ged_rotate);
    scm_c_define_gsubr("translate", 0, 0, 1, (scm_t_subr)scm_ged_translate);
    scm_c_define_gsubr("orotate", 0, 0, 1, (scm_t_subr)scm_ged_orotate);
    scm_c_define_gsubr("oscale", 0, 0, 1, (scm_t_subr)scm_ged_oscale);
    scm_c_define_gsubr("otranslate", 0, 0, 1, (scm_t_subr)scm_ged_otranslate);
    scm_c_define_gsubr("push", 0, 0, 1, (scm_t_subr)scm_ged_push);
    scm_c_define_gsubr("xpush", 0, 0, 1, (scm_t_subr)scm_ged_xpush);
    scm_c_define_gsubr("pull", 0, 0, 1, (scm_t_subr)scm_ged_pull);
    
    /* View Commands */
    scm_c_define_gsubr("view", 0, 0, 1, (scm_t_subr)scm_ged_view);
    scm_c_define_gsubr("size", 0, 0, 1, (scm_t_subr)scm_ged_size);
    scm_c_define_gsubr("center", 0, 0, 1, (scm_t_subr)scm_ged_center);
    scm_c_define_gsubr("autoview", 0, 0, 1, (scm_t_subr)scm_ged_autoview);
    scm_c_define_gsubr("ae", 0, 0, 1, (scm_t_subr)scm_ged_ae);
    scm_c_define_gsubr("arot", 0, 0, 1, (scm_t_subr)scm_ged_arot);
    scm_c_define_gsubr("vrot", 0, 0, 1, (scm_t_subr)scm_ged_vrot);
    scm_c_define_gsubr("zoom", 0, 0, 1, (scm_t_subr)scm_ged_zoom);
    scm_c_define_gsubr("slew", 0, 0, 1, (scm_t_subr)scm_ged_slew);
    scm_c_define_gsubr("perspective", 0, 0, 1, (scm_t_subr)scm_ged_perspective);
    scm_c_define_gsubr("eye_pos", 0, 0, 1, (scm_t_subr)scm_ged_eye_pos);
    scm_c_define_gsubr("lookat", 0, 0, 1, (scm_t_subr)scm_ged_lookat);
    scm_c_define_gsubr("orientation", 0, 0, 1, (scm_t_subr)scm_ged_orientation);
    scm_c_define_gsubr("ypr", 0, 0, 1, (scm_t_subr)scm_ged_ypr);
    scm_c_define_gsubr("saveview", 0, 0, 1, (scm_t_subr)scm_ged_saveview);
    scm_c_define_gsubr("loadview", 0, 0, 1, (scm_t_subr)scm_ged_loadview);
    
    /* Information & Queries */
    scm_c_define_gsubr("ls", 0, 0, 1, (scm_t_subr)scm_ged_ls);
    scm_c_define_gsubr("search", 0, 0, 1, (scm_t_subr)scm_ged_search);
    scm_c_define_gsubr("tree", 0, 0, 1, (scm_t_subr)scm_ged_tree);
    scm_c_define_gsubr("tops", 0, 0, 1, (scm_t_subr)scm_ged_tops);
    scm_c_define_gsubr("get", 0, 0, 1, (scm_t_subr)scm_ged_get);
    scm_c_define_gsubr("put", 0, 0, 1, (scm_t_subr)scm_ged_put);
    scm_c_define_gsubr("attr", 0, 0, 1, (scm_t_subr)scm_ged_attr);
    scm_c_define_gsubr("title", 0, 0, 1, (scm_t_subr)scm_ged_title);
    scm_c_define_gsubr("units", 0, 0, 1, (scm_t_subr)scm_ged_units);
    scm_c_define_gsubr("summary", 0, 0, 1, (scm_t_subr)scm_ged_summary);
    scm_c_define_gsubr("bb", 0, 0, 1, (scm_t_subr)scm_ged_bb);
    scm_c_define_gsubr("cat", 0, 0, 1, (scm_t_subr)scm_ged_cat);
    scm_c_define_gsubr("find", 0, 0, 1, (scm_t_subr)scm_ged_find);
    scm_c_define_gsubr("which", 0, 0, 1, (scm_t_subr)scm_ged_which);
    scm_c_define_gsubr("pathlist", 0, 0, 1, (scm_t_subr)scm_ged_pathlist);
    
    /* Materials & Appearance */
    scm_c_define_gsubr("color", 0, 0, 1, (scm_t_subr)scm_ged_color);
    scm_c_define_gsubr("shader", 0, 0, 1, (scm_t_subr)scm_ged_shader);
    scm_c_define_gsubr("mater", 0, 0, 1, (scm_t_subr)scm_ged_mater);
    scm_c_define_gsubr("edcomb", 0, 0, 1, (scm_t_subr)scm_ged_edcomb);
    
    /* Analysis */
    scm_c_define_gsubr("analyze", 0, 0, 1, (scm_t_subr)scm_ged_analyze);
    scm_c_define_gsubr("check", 0, 0, 1, (scm_t_subr)scm_ged_check);
    scm_c_define_gsubr("heal", 0, 0, 1, (scm_t_subr)scm_ged_heal);
    
    /* Raytracing */
    scm_c_define_gsubr("rt", 0, 0, 1, (scm_t_subr)scm_ged_rt);
    scm_c_define_gsubr("rtcheck", 0, 0, 1, (scm_t_subr)scm_ged_rtcheck);
    scm_c_define_gsubr("rtabort", 0, 0, 1, (scm_t_subr)scm_ged_rtabort);
    
    /* Output */
    scm_c_define_gsubr("overlay", 0, 0, 1, (scm_t_subr)scm_ged_overlay);
    scm_c_define_gsubr("plot", 0, 0, 1, (scm_t_subr)scm_ged_plot);
    scm_c_define_gsubr("png", 0, 0, 1, (scm_t_subr)scm_ged_png);
    
    /* File Operations */
    scm_c_define_gsubr("dbconcat", 0, 0, 1, (scm_t_subr)scm_ged_dbconcat);
    scm_c_define_gsubr("keep", 0, 0, 1, (scm_t_subr)scm_ged_keep);
    scm_c_define_gsubr("dump", 0, 0, 1, (scm_t_subr)scm_ged_dump);
    
    /* Primitive-Specific */
    scm_c_define_gsubr("bot", 0, 0, 1, (scm_t_subr)scm_ged_bot);
    scm_c_define_gsubr("brep", 0, 0, 1, (scm_t_subr)scm_ged_brep);
    scm_c_define_gsubr("nmg", 0, 0, 1, (scm_t_subr)scm_ged_nmg);
    scm_c_define_gsubr("dsp", 0, 0, 1, (scm_t_subr)scm_ged_dsp);
    scm_c_define_gsubr("pipe", 0, 0, 1, (scm_t_subr)scm_ged_pipe);
    scm_c_define_gsubr("arb", 0, 0, 1, (scm_t_subr)scm_ged_arb);
    
    /* Utilities */
    scm_c_define_gsubr("tol", 0, 0, 1, (scm_t_subr)scm_ged_tol);
    scm_c_define_gsubr("prefix", 0, 0, 1, (scm_t_subr)scm_ged_prefix);
    
    /* REPL Control */
    scm_c_define_gsubr("quit", 0, 0, 0, (scm_t_subr)scm_quit);
    scm_c_define_gsubr("exit", 0, 0, 0, (scm_t_subr)scm_exit);
}


void
guilecad_register_bu_commands(void)
{
    /* Placeholder for BU library commands */
}


void
guilecad_register_bn_commands(void)
{
    /* Placeholder for BN library commands */
}


void
guilecad_register_rt_commands(void)
{
    /* Placeholder for RT library commands */
}


void
guilecad_register_dm_commands(void)
{
    /* Placeholder for DM library commands */
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
