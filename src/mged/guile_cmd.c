/*                   G U I L E _ C M D . C
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
/** @file mged/guile_cmd.c
 *
 * Guile command processing for MGED
 *
 */

#include "common.h"

#ifdef HAVE_GUILE

#include <libguile.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_EDITLINE
#  include <histedit.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "bu/log.h"
#include "bu/time.h"
#include "bu/vls.h"
#include "ged.h"
#include "guilecad/guilecad.h"

#include "./mged.h"
#include "./mged_dm.h"


/**
 * Evaluate a Guile expression
 *
 * Returns:
 * CMD_OK - success
 * CMD_BAD - error (shouldn't happen as exceptions propagate)
 * CMD_MORE - incomplete expression
 */
int
cmdline_guile(struct mged_state *s, struct bu_vls *vp)
{
    SCM result;
    SCM result_str;
    char *output;
    int64_t start, finish;
    
    (void)s; /* Unused parameter */
    
    if (!vp || bu_vls_strlen(vp) <= 0) {
        return CMD_OK;
    }
    
    /* Check if expression is complete */
    if (!guilecad_expression_complete(bu_vls_cstr(vp))) {
        return CMD_MORE;
    }
    
    start = bu_gettime();
    
    /* Evaluate - let exceptions propagate to Guile's REPL */
    result = scm_c_eval_string(bu_vls_cstr(vp));
    
    finish = bu_gettime();
    
    /* Convert result to string and display */
    if (!scm_is_false(result)) {
        result_str = scm_object_to_string(result, SCM_UNDEFINED);
        output = scm_to_locale_string(result_str);
        
        if (output && strlen(output) > 0 && strcmp(output, "#<unspecified>") != 0) {
            bu_log("%s\n", output);
        }
        
        free(output);
    }
    
    /* Update frame time for display refresh */
    if (finish > start) {
        extern double frametime;
        frametime = 0.9 * frametime + 0.1 * (finish - start) / 1000000.0;
    }
    
    return CMD_OK;
}


#ifdef HAVE_EDITLINE

/* Static state for libedit */
static EditLine *el = NULL;
static History *hist = NULL;
static HistEvent ev;
static char prompt_buffer[32] = "guile> ";

/**
 * Prompt callback for libedit
 */
static char *
guile_prompt_callback(EditLine *UNUSED(e))
{
    return prompt_buffer;
}

/**
 * Initialize libedit for Guile REPL
 */
static void
guile_editline_init(void)
{
    if (el) return;  /* Already initialized */
    
    /* Initialize EditLine */
    el = el_init("mged", stdin, stdout, stderr);
    el_set(el, EL_PROMPT, guile_prompt_callback);
    el_set(el, EL_EDITOR, "emacs");
    
    /* Initialize History */
    hist = history_init();
    if (hist) {
        history(hist, &ev, H_SETSIZE, 800);
        el_set(el, EL_HIST, history, hist);
    }
}

/**
 * Clean up libedit
 */
static void
guile_editline_cleanup(void)
{
    if (el) {
        el_end(el);
        el = NULL;
    }
    if (hist) {
        history_end(hist);
        hist = NULL;
    }
}

/**
 * Interactive REPL using libedit
 * 
 * This function handles expression-oriented input, accumulating lines
 * until a complete Scheme expression is entered.
 */
void
guile_repl_libedit(struct mged_state *s)
{
    struct bu_vls expr_buffer = BU_VLS_INIT_ZERO;
    int line_count = 0;
    
    guile_editline_init();
    
    while (1) {
        int char_count;
        const char *line;
        
        /* Set appropriate prompt */
        if (line_count == 0) {
            strncpy(prompt_buffer, "guile> ", sizeof(prompt_buffer)-1);
        } else {
            strncpy(prompt_buffer, "...  ", sizeof(prompt_buffer)-1);
        }
        prompt_buffer[sizeof(prompt_buffer)-1] = '\0';
        
        /* Read a line with full editing support */
        line = el_gets(el, &char_count);
        
        /* Handle EOF (CTRL-D) */
        if (!line) {
            if (bu_vls_strlen(&expr_buffer) == 0) {
                /* Empty buffer - exit cleanly */
                bu_log("\n");
                bu_vls_free(&expr_buffer);
                guile_editline_cleanup();
                quit(s);
                /* NOTREACHED */
            }
            /* Partial expression - ignore CTRL-D and continue */
            continue;
        }
        
        /* Empty line with no accumulated input - just show prompt again */
        if (char_count <= 0 || (char_count == 1 && line[0] == '\n')) {
            if (bu_vls_strlen(&expr_buffer) == 0) {
                continue;
            }
        }
        
        /* Accumulate this line */
        bu_vls_strcat(&expr_buffer, line);
        line_count++;
        
        /* Check if we have a complete expression */
        if (guilecad_expression_complete(bu_vls_cstr(&expr_buffer))) {
            int status;
            
            /* Add to history as a single entry */
            if (hist) {
                history(hist, &ev, H_ENTER, bu_vls_cstr(&expr_buffer));
            }
            
            /* Evaluate the complete expression */
            (void)signal(SIGINT, SIG_IGN);
            status = cmdline_guile(s, &expr_buffer);
            (void)signal(SIGINT, SIG_DFL);
            
            if (status == CMD_MORE) {
                /* Shouldn't happen if guilecad_expression_complete() works correctly */
                bu_log("Warning: Expression marked complete but evaluation returned CMD_MORE\n");
            }
            
            /* Reset for next expression */
            bu_vls_trunc(&expr_buffer, 0);
            line_count = 0;
        }
        /* else: incomplete expression, continue accumulating on next iteration */
    }
    
    /* Cleanup (not normally reached) */
    bu_vls_free(&expr_buffer);
    guile_editline_cleanup();
}

#endif /* HAVE_EDITLINE */


/**
 * Process a single character of Guile input (fallback for non-libedit systems)
 *
 * This handles REPL-style input with parenthesis balancing
 */
void
guile_process_char(struct mged_state *s, char ch)
{
    static int paren_depth = 0;
    static int in_string = 0;
    static int escaped = 0;
    struct bu_vls temp = BU_VLS_INIT_ZERO;

#define CTRL_D      4

    /* Handle CTRL-D (EOF/delete character) */
    if (ch == CTRL_D) {
        if (s->input_str_index == 0 && bu_vls_strlen(&s->input_str) == 0) {
            /* Empty input buffer - exit like a normal shell */
            bu_log("exit\n");
            quit(s);
            /* NOTREACHED */
        }
        /* Not at beginning or buffer not empty - delete character at cursor */
        if (s->input_str_index < bu_vls_strlen(&s->input_str)) {
            bu_vls_strcpy(&temp, bu_vls_addr(&s->input_str)+s->input_str_index+1);
            bu_vls_trunc(&s->input_str, s->input_str_index);
            bu_log("%s ", bu_vls_addr(&temp));
            bu_vls_strcat(&s->mged_prompt, bu_vls_addr(&s->input_str));
            bu_log("\r");
            pr_prompt(s);
            bu_log("%s", bu_vls_addr(&s->input_str));
            bu_vls_vlscat(&s->input_str, &temp);
            bu_vls_free(&temp);
        }
        return;
    }
    
    /* Track parenthesis depth and string state */
    if (!escaped && !in_string) {
        if (ch == '(') {
            paren_depth++;
        } else if (ch == ')') {
            paren_depth--;
        }
    }
    
    if (!escaped && ch == '"') {
        in_string = !in_string;
    }
    
    if (ch == '\\') {
        escaped = !escaped;
    } else {
        escaped = 0;
    }
    
    /* Add character to input buffer */
    if (ch == '\n' || ch == '\r') {
        bu_log("\n");
        
        /* Check if expression is complete */
        if (paren_depth == 0 && !in_string) {
            int status;
            
            /* Execute the expression */
            (void)signal(SIGINT, SIG_IGN);
            status = cmdline_guile(s, &s->input_str);
            
            if (status == CMD_MORE) {
                /* Incomplete expression - continue on next line */
                bu_vls_strcat(&s->input_str, "\n");
                bu_vls_strcpy(&s->mged_prompt, "... ");
            } else {
                /* Complete - reset for next input */
                bu_vls_trunc(&s->input_str, 0);
                bu_vls_strcpy(&s->mged_prompt, "guile> ");
                paren_depth = 0;
                in_string = 0;
            }
            
            pr_prompt(s);
            s->input_str_index = 0;
        } else {
            /* Multi-line expression - continue */
            bu_vls_strcat(&s->input_str, "\n");
            bu_vls_strcpy(&s->mged_prompt, "... ");
            pr_prompt(s);
        }
    } else if (ch == '\b' || ch == 127) {
        /* Backspace */
        if (s->input_str_index > 0) {
            if (s->input_str_index == bu_vls_strlen(&s->input_str)) {
                bu_log("\b \b");
                bu_vls_trunc(&s->input_str, bu_vls_strlen(&s->input_str) - 1);
            } else {
                bu_vls_strcpy(&temp, bu_vls_addr(&s->input_str)+s->input_str_index);
                bu_vls_trunc(&s->input_str, s->input_str_index-1);
                bu_log("\b%s ", bu_vls_addr(&temp));
                bu_log("\r");
                pr_prompt(s);
                bu_log("%s", bu_vls_addr(&s->input_str));
                bu_vls_vlscat(&s->input_str, &temp);
                bu_vls_free(&temp);
            }
            s->input_str_index--;
        }
    } else if (ch >= ' ' && ch < 127) {
        /* Printable character */
        bu_log("%c", ch);
        bu_vls_putc(&s->input_str, ch);
        s->input_str_index++;
    }
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
