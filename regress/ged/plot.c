/*                         P L O T . C
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
/** @file plot.c
 *
 * Regression test for the plot command, specifically the -t (text mode) option.
 *
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <bu.h>
#include <ged.h>


static int
test_plot_text_basic(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", NULL};
    const char *result;
    int ret;

    bu_log("TEST: plot -t (basic ASCII output)\n");

    ret = ged_exec(gedp, 2, plot_cmd);
    if (ret != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t' command failed with code %d\n", ret);
	bu_log("Error message: %s\n", bu_vls_cstr(gedp->ged_result_str));
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);
    
    /* Check that we got ASCII output */
    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t' returned empty result\n");
	return -1;
    }

    /* Verify it contains plot command characters (ASCII format) */
    /* Common plot commands: M (move3), N (cont3), O (move3 float), etc. */
    if (!strchr(result, 'M') && !strchr(result, 'N') && 
	!strchr(result, 'O') && !strchr(result, 'Q') &&
	!strchr(result, 'W') && !strchr(result, 'C')) {
	bu_log("FAIL: 'plot -t' output doesn't contain expected plot command characters\n");
	bu_log("Result: %s\n", result);
	return -1;
    }

    /* Verify format looks like ASCII (contains spaces and newlines) */
    if (!strchr(result, ' ') && !strchr(result, '\n')) {
	bu_log("FAIL: 'plot -t' output doesn't look like ASCII text format\n");
	return -1;
    }

    bu_log("PASS: plot -t basic ASCII output\n");
    return 0;
}


static int
test_plot_text_3d(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", "-3", NULL};
    const char *result;

    bu_log("TEST: plot -t -3 (3D ASCII output)\n");

    if (ged_exec(gedp, 3, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t -3' command failed\n");
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);
    
    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t -3' returned empty result\n");
	return -1;
    }

    /* 3D plots should use integer commands like M, N, L, S */
    if (!strchr(result, 'M') && !strchr(result, 'L') && !strchr(result, 'S')) {
	bu_log("FAIL: 'plot -t -3' output doesn't contain expected 3D plot commands\n");
	return -1;
    }

    bu_log("PASS: plot -t -3 3D ASCII output\n");
    return 0;
}


static int
test_plot_text_2d(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", "-2", NULL};
    const char *result;

    bu_log("TEST: plot -t -2 (2D ASCII output)\n");

    if (ged_exec(gedp, 3, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t -2' command failed\n");
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);
    
    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t -2' returned empty result\n");
	return -1;
    }

    /* 2D plots use lowercase commands: m, n, l, s */
    /* Note: space command is 's' for 2D */
    if (!strchr(result, 'm') && !strchr(result, 'n') && 
	!strchr(result, 'l') && !strchr(result, 's')) {
	bu_log("FAIL: 'plot -t -2' output doesn't contain expected 2D plot commands\n");
	return -1;
    }

    bu_log("PASS: plot -t -2 2D ASCII output\n");
    return 0;
}


static int
test_plot_text_float(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", "-f", NULL};
    const char *result;

    bu_log("TEST: plot -t -f (floating point ASCII output)\n");

    if (ged_exec(gedp, 3, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t -f' command failed\n");
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);
    
    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t -f' returned empty result\n");
	return -1;
    }

    /* Floating point 3D plots use: O (move), Q (cont), V (line), W (space) */
    if (!strchr(result, 'O') && !strchr(result, 'Q') && 
	!strchr(result, 'V') && !strchr(result, 'W')) {
	bu_log("FAIL: 'plot -t -f' output doesn't contain expected float plot commands\n");
	return -1;
    }

    /* Floating point output should contain decimal points */
    if (!strchr(result, '.')) {
	bu_log("FAIL: 'plot -t -f' output doesn't contain decimal points (not floating point?)\n");
	return -1;
    }

    bu_log("PASS: plot -t -f floating point ASCII output\n");
    return 0;
}


static int
test_plot_text_zclip(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", "-z", NULL};

    bu_log("TEST: plot -t -z (with Z clipping)\n");

    if (ged_exec(gedp, 3, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t -z' command failed\n");
	return -1;
    }

    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t -z' returned empty result\n");
	return -1;
    }

    /* Just verify it works with -z option */
    bu_log("PASS: plot -t -z Z clipping works\n");
    return 0;
}


static int
test_plot_text_combined_options(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", "-f", "-z", NULL};
    const char *result;

    bu_log("TEST: plot -t -f -z (combined options)\n");

    if (ged_exec(gedp, 4, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t -f -z' command failed\n");
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);
    
    if (bu_vls_strlen(gedp->ged_result_str) == 0) {
	bu_log("FAIL: 'plot -t -f -z' returned empty result\n");
	return -1;
    }

    /* Should still have floating point output */
    if (!strchr(result, '.')) {
	bu_log("FAIL: 'plot -t -f -z' output doesn't contain decimal points\n");
	return -1;
    }

    bu_log("PASS: plot -t -f -z combined options work\n");
    return 0;
}


static int
test_plot_traditional_file_output(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "test_plot.pl", NULL};
    const char *result;

    bu_log("TEST: plot test_plot.pl (traditional file output still works)\n");

    if (ged_exec(gedp, 2, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot test_plot.pl' command failed\n");
	return -1;
    }

    result = bu_vls_cstr(gedp->ged_result_str);

    /* Should report where plot was stored */
    if (!strstr(result, "plot stored in")) {
	bu_log("FAIL: 'plot test_plot.pl' doesn't report file storage\n");
	return -1;
    }

    /* Verify file was created */
    if (!bu_file_exists("test_plot.pl", NULL)) {
	bu_log("FAIL: 'plot test_plot.pl' didn't create output file\n");
	return -1;
    }

    bu_log("PASS: plot test_plot.pl traditional file output works\n");
    return 0;
}


static int
test_plot_no_geometry(struct ged *gedp)
{
    const char *plot_cmd[] = {"plot", "-t", NULL};

    bu_log("TEST: plot -t with no geometry displayed\n");

    /* This test uses a database with no objects drawn */
    if (ged_exec(gedp, 2, plot_cmd) != BRLCAD_OK) {
	bu_log("FAIL: 'plot -t' failed with no geometry\n");
	return -1;
    }

    /* Should still succeed, just with minimal output (maybe just space command) */
    bu_log("PASS: plot -t with no geometry doesn't crash\n");
    return 0;
}


int
main(int argc, char *argv[])
{
    struct ged *gedp;
    const char *gname = "ged_plot_test.g";
    const char *make_cmd[] = {"make", "sph1.s", "sph", NULL};
    const char *draw_cmd[] = {"draw", "sph1.s", NULL};
    int test_failures = 0;
    struct bview *temp_view = NULL;

    bu_setprogname(argv[0]);

    if (argc != 2) {
	printf("Usage: %s test_name\n", argv[0]);
	printf("Available tests:\n");
	printf("  all               - Run all tests\n");
	printf("  text_basic        - Test basic ASCII output\n");
	printf("  text_3d           - Test 3D ASCII output\n");
	printf("  text_2d           - Test 2D ASCII output\n");
	printf("  text_float        - Test floating point ASCII output\n");
	printf("  text_zclip        - Test Z clipping with ASCII output\n");
	printf("  text_combined     - Test combined options\n");
	printf("  file_output       - Test traditional file output still works\n");
	printf("  no_geometry       - Test with no geometry displayed\n");
	return 1;
    }

    if (bu_file_exists(gname, NULL)) {
	printf("Error: %s already exists\n", gname);
	return 1;
    }

    /* Create test database */
    gedp = ged_open("db", gname, 0);
    if (!gedp) {
	bu_log("Error: failed to create database %s\n", gname);
	return 1;
    }

    /* Initialize a view - required for plot command */
    if (!gedp->ged_gvp) {
	/* Create a temporary view for testing */
	BU_ALLOC(temp_view, struct bview);
	bv_init(temp_view, &gedp->ged_views);
	gedp->ged_gvp = temp_view;
    }

    /* Run test based on argument */
    if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "no_geometry")) {
	if (test_plot_no_geometry(gedp) != 0)
	    test_failures++;
    }

    /* For other tests, we need geometry */
    if (!BU_STR_EQUAL(argv[1], "no_geometry")) {
	/* Create and display some geometry */
	if (ged_exec(gedp, 3, make_cmd) != BRLCAD_OK) {
	    bu_log("Error: failed to create test geometry\n");
	    goto cleanup;
	}

	if (ged_exec(gedp, 2, draw_cmd) != BRLCAD_OK) {
	    bu_log("Error: failed to draw test geometry\n");
	    goto cleanup;
	}

	/* Run the requested test(s) */
	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_basic")) {
	    if (test_plot_text_basic(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_3d")) {
	    if (test_plot_text_3d(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_2d")) {
	    if (test_plot_text_2d(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_float")) {
	    if (test_plot_text_float(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_zclip")) {
	    if (test_plot_text_zclip(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "text_combined")) {
	    if (test_plot_text_combined_options(gedp) != 0)
		test_failures++;
	}

	if (BU_STR_EQUAL(argv[1], "all") || BU_STR_EQUAL(argv[1], "file_output")) {
	    if (test_plot_traditional_file_output(gedp) != 0)
		test_failures++;
	}
    }

cleanup:
    /* Clean up temporary view if we created one */
    if (temp_view) {
	bu_free(temp_view, "temp_view");
    }
    
    ged_close(gedp);

    /* Clean up test files */
    if (bu_file_exists(gname, NULL)) {
	bu_file_delete(gname);
    }
    if (bu_file_exists("test_plot.pl", NULL)) {
	bu_file_delete("test_plot.pl");
    }

    if (test_failures > 0) {
	bu_log("\n%d test(s) FAILED\n", test_failures);
	return 1;
    }

    bu_log("\nAll tests PASSED\n");
    return 0;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
