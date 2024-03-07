/*
 * Copyright (C) 2024 Pierre Colin
 * This file is part of Condor.
 * 
 * Condor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, version 3.
 * 
 * Condor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Condor.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "condor.h"
#include "util.h"

static cdor_bool
test_empty (void)
{
	const struct cdor_strategy strat = cdor_optimal_strategy (0, NULL);
	fputs ("test_empty: ", stdout);
	switch (strat.type) {
	case CDOR_MIXED:
		free (strat.val.mixed);
		/* fallthrough */
	case CDOR_PURE:
		puts ("a strategy was found");
		return false;
	case CDOR_ERROR:
		puts ("OK");
	}
	return true;
}

static cdor_bool
expect_pure (const struct cdor_strategy REF(strat), const unsigned expect)
{
	if (strat->type == CDOR_ERROR) {
		printf ("no strategy found (expected %u)\n", expect);
		return false;
	} else if (strat->type == CDOR_MIXED) {
		free (strat->val.mixed);
		printf ("strategy was mixed (expected %u)\n", expect);
		return false;
	} else if (strat->val.pure != expect) {
		printf ("%u won (expected %u)\n", (unsigned) strat->val.pure,
		        expect);
		return false;
	}
	puts ("OK");
	return true;
}

static cdor_bool
test_trivial (void)
{
	const char graph[1] = { 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (1, graph);
	fputs ("test_trivial: ", stdout);
	return expect_pure (&strat, 0);
}

static int
comp_double (const void *x_arg, const void *y_arg)
{
	double x = *(double *) x_arg, y = *(double *) y_arg;
	if (x < y)
		return -1;
	if (x > y)
		return 1;
	return 0;
}

static cdor_bool
vec_compare (const size_t n, const double ARR_PARAM(x, n), const double ARR_PARAM(y, n))
{
	double * const diff = allocate(double, n);
	double sum = 0;
	size_t i;
	if (diff) {
		for (i = 0; i < n; i++)
			diff[i] = fabs (x[i] - y[i]);
		qsort (diff, n, sizeof (double), comp_double);
		for (i = 0; i < n; i++)
			sum += diff[i];
		free (diff);
	} else {
		/* Less accurate but doesn't require heap */
		for (i = 0; i < n; i++)
			sum = fabs (x[i] - y[i]);
	}
	return sum < 1e-6;
}

static cdor_bool
expect_mixed (const struct cdor_strategy REF(strat), const size_t nalt,
              const double ARR_PARAM(expect, nalt))
{
	size_t i;
	if (strat->type == CDOR_ERROR) {
		printf ("no strategy found (expected");
		for (i = 0; i < nalt; i++)
			printf (" %f", expect[i]);
		puts (")");
		return false;
	} else if (strat->type == CDOR_PURE) {
		printf ("strategy was pure (expected");
		for (i = 0; i < nalt; i++)
			printf (" %f", expect[i]);
		puts (")");
		return false;
	} else if (!vec_compare (nalt, strat->val.mixed, expect)) {
		printf ("strategy was");
		for (i = 0; i < nalt; i++)
			printf (" %f", strat->val.mixed[i]);
		printf (" (expected ");
		for (i = 0; i < nalt; i++)
			printf (" %f", expect[i]);
		puts (")");
		free (strat->val.mixed);
		return false;
	}
	free (strat->val.mixed);
	puts ("OK");
	return true;
}

static cdor_bool
test_tie (void)
{
	const char graph[4] = { 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (2, graph);
	const double expected[2] = { 0.5, 0.5 };
	fputs ("test_tie: ", stdout);
	return expect_mixed (&strat, 2, expected);
}

static cdor_bool
test_1v1 (void)
{
	const char graph[4] = { 0, 1, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (2, graph);
	fputs ("test_1v1: ", stdout);
	return expect_pure (&strat, 0);
}

static cdor_bool
test_3way_tie (void)
{
	const char graph[9] = { 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	const double expected[3] = { 1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0 };
	fputs ("test_3way_tie: ", stdout);
	return expect_mixed (&strat, 3, expected);
}

static cdor_bool
test_tie_plus_win (void)
{
	const char graph[9] = { 0, 1, 0, 0, 0, 0, 0, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	const double expected[3] = { 0.5, 0.0, 0.5 };
	fputs ("test_tie_plus_win: ", stdout);
	return expect_mixed (&strat, 3, expected);
}

static cdor_bool
test_win_over_tie (void)
{
	const char graph[9] = { 0, 1, 1, 0, 0, 0, 0, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	fputs ("test_win_over_tie: ", stdout);
	return expect_pure (&strat, 0);
}

static cdor_bool
test_3ladder (void)
{
	const char graph[9] = { 0, 1, 1, 0, 0, 1, 0, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	fputs ("test_3ladder: ", stdout);
	return expect_pure (&strat, 0);
}

static cdor_bool
test_dp (void)
{
	const char graph[9] = { 0, 0, 1, 0, 0, 1, 0, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	const double expected[3] = { 0.5, 0.5, 0.0 };
	fputs ("test_dp: ", stdout);
	return expect_mixed (&strat, 3, expected);
}

static cdor_bool
test_condorcet_paradox (void)
{
	const char graph[9] = { 0, 1, 0, 0, 0, 1, 1, 0, 0 };
	const struct cdor_strategy strat = cdor_optimal_strategy (3, graph);
	const double expected[3] = { 1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0 };
	fputs ("test_condorcet_paradox: ", stdout);
	return expect_mixed (&strat, 3, expected);
}

static cdor_bool
test_paradox_plus_lonely (void)
{
	const char graph[16] = {
		0, 1, 0, 0,
		0, 0, 1, 0,
		1, 0, 0, 0,
		0, 0, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (4, graph);
	fputs("test_paradox_plus_lonely: ", stdout);
	return expect_pure (&strat, 3);
}

static cdor_bool
test_win_over_paradox (void)
{
	const char graph[16] = {
		0, 1, 0, 0,
		0, 0, 1, 0,
		1, 0, 0, 0,
		1, 1, 1, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (4, graph);
	fputs ("test_win_over_paradox: ", stdout);
	return expect_pure (&strat, 3);
}

static cdor_bool
test_paradox_over_lonely (void)
{
	const char graph[16] = {
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 0, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (4, graph);
	const double expected[4] = { 1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 0.0 };
	fputs ("test_paradox_over_lonely: ", stdout);
	return expect_mixed (&strat, 4, expected);
}

static cdor_bool
test_5uniform (void)
{
	const char graph[25] = {
		0, 1, 1, 0, 0,
		0, 0, 1, 1, 0,
		0, 0, 0, 1, 1,
		1, 0, 0, 0, 1,
		1, 1, 0, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (5, graph);
	const double expected[5] = { 0.2, 0.2, 0.2, 0.2, 0.2 };
	fputs ("test_5uniform: ", stdout);
	return expect_mixed (&strat, 5, expected);
}

static cdor_bool
test_5heterogen (void)
{
	const char graph[25] = {
		0, 1, 1, 1, 0,
		0, 0, 1, 0, 1,
		0, 0, 0, 1, 1,
		0, 1, 0, 0, 1,
		1, 0, 0, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (5, graph);
	const double expected[5] = {
		1.0 / 3.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 3.0
	};
	fputs ("test_5heterogen: ", stdout);
	return expect_mixed (&strat, 5, expected);
}

static cdor_bool
test_two_paradox (void)
{
	const char graph[36] = {
		0, 1, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (6, graph);
	const double expected[6] = {
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0
	};
	fputs ("test_two_paradox: ", stdout);
	return expect_mixed (&strat, 6, expected);
}

static cdor_bool
test_paradox_plus_5 (void)
{
	const char graph[64] = {
		0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 0, 1,
		0, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 1, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0, 0
	};
	const struct cdor_strategy strat = cdor_optimal_strategy (8, graph);
	const double expected[8] = {
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 6.0,
		1.0 / 18.0,
		1.0 / 18.0,
		1.0 / 18.0,
		1.0 / 6.0
	};
	fputs ("test_paradox_plus_5: ", stdout);
	return expect_mixed (&strat, 8, expected);
}

int
main (void)
{
	cdor_bool (*test[])(void) = {
		test_empty,
		test_trivial,
		test_tie,
		test_1v1,
		test_3way_tie,
		test_tie_plus_win,
		test_win_over_tie,
		test_3ladder,
		test_dp,
		test_condorcet_paradox,
		test_paradox_plus_lonely,
		test_win_over_paradox,
		test_paradox_over_lonely,
		test_5uniform,
		test_5heterogen,
		test_two_paradox,
		test_paradox_plus_5
	};
	size_t i;
	cdor_bool all_good = true;
	for (i = 0; i < sizeof test / sizeof test[0]; i++)
		all_good &= test[i] ();
	if (all_good) {
		puts ("All tests succeeded!");
		return EXIT_SUCCESS;
	} else {
		puts ("One or more tests failed");
		return EXIT_FAILURE;
	}
}
