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
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lpsolve/lp_lib.h>

#include "condor.h"
#include "util.h"

struct wcc_data {
	size_t num;
	size_t *size;
	size_t *map;
	size_t maxsz;
};

#ifdef __GNUC__
__attribute__((const))
#endif
static double
prep_row_coord (const cdor_bool direct, const cdor_bool indirect,
                const cdor_bool minimax)
{
	if (minimax)
		return direct ? 3.0 : indirect ? 1.0 : 2.0;
	return indirect ? 3.0 : direct ? 1.0 : 2.0;
}

static lprec *
cdor_prepare (const size_t nalt, const cdor_bool ARR_PARAM(graph, nalt * nalt),
              const cdor_bool minimax)
{
	lprec * CDOR_RESTRICT prob = NULL;
	double * CDOR_RESTRICT row;
	unsigned int r;
	if (nalt >= INT_MAX) {
#if _POSIX_C_SOURCE >= 1L
		errno = EINVAL;
#endif
		return NULL;
	}
	if (!(prob = make_lp (0, (int) nalt))) {
#if _POSIX_C_SOURCE >= 1L
		errno = ENOMEM;
#endif
		return NULL;
	}
	set_verbose (prob, NEUTRAL);
	if (minimax)
		set_maxim (prob);
	if (!(row = allocate(double, nalt + 1))) {
		delete_lp (prob);
		return NULL;
	}
	for (r = 1; r <= nalt; r++) {
		size_t c;
		/* lp_solve errors impossible given arguments */
		set_obj (prob, (int) r, 1.0);
		for (c = 1; c <= nalt; c++) {
			row[c] = prep_row_coord (graph[(r - 1) * nalt + c - 1],
			                         graph[(c - 1) * nalt + r - 1],
			                         minimax);
		}
		if (!add_constraint (prob, row, minimax ? LE : GE, 1.0)) {
			free (row);
			delete_lp (prob);
#if _POSIX_C_SOURCE >= 1L
			errno = ENOMEM;
#endif
			return NULL;
		}
	}
	free (row);
	return prob;
}

#ifdef __GNUC__
__attribute__ ((const, nonnull (1, 2)))
#endif
static int
comp_double (const void * CDOR_RESTRICT const x_arg,
             const void * CDOR_RESTRICT const y_arg)
{
	double x = *(double *) x_arg, y = *(double *) y_arg;
	return (x > y) - (x < y);
}

static double
cdor_sum (const size_t n, const double ARR_PARAM(x, n))
{
	double * const a = allocate(double, n);
	double s = 0.0;
	if (a != NULL) {
		size_t i;
		memcpy (a, x, n * sizeof (double));
		qsort (a, n, sizeof (double), comp_double);
		for (i = 0; i < n; i++)
			s += a[i];
		free (a);
		return s;
	} else {
		/* Slower but doesn't need memory */
		double min = 0.0;
		int found = 0;
		for (;;) {
			double new_min = HUGE_VAL;
			size_t i, im = 0;
			for (i = 0; i < n; i++) {
				if (x[i] > min && x[i] < new_min) {
					new_min = x[i];
					im = i;
					found = 1;
				}
			}
			if (!found)
				return s;
			for (i = im; i < n; i++) {
				if (x[i] == new_min)
					s += new_min;
			}
			min = new_min;
			found = 0;
		}
	}
}
 
#ifdef __GNUC__
__attribute__((nonnull (1)))
#endif
static cdor_bool
cdor_solve_finalize (lprec REF(prob), const size_t nalt,
                     double ARR_PARAM(dest, nalt))
{
	double norm;
	size_t c;
	const int result = solve (prob);
	if (result < 0 || result > 1)
		goto fail;
	norm = get_objective (prob);
	if (!get_variables (prob, dest))
		goto fail;
	for (c = 0; c < nalt; c++)
		dest[c] /= norm;
	delete_lp (prob);
	return true;
fail:
	delete_lp (prob);
#if _POSIX_C_SOURCE >= 1L
	errno = EDOM;
#endif
	return false;
}

static cdor_bool
cdor_solve (const size_t nalt, double ARR_PARAM(dest, nalt),
            const cdor_bool ARR_PARAM(graph, nalt * nalt),
            const cdor_bool minimax)
{
	lprec * CDOR_RESTRICT prob;
	if (nalt == 0)
		return true;
	if (!(prob = cdor_prepare (nalt, graph, minimax)))
		return false;
	return cdor_solve_finalize (prob, nalt, dest);
}

static size_t
cdor_find_sources (const size_t nalt, cdor_bool ARR_PARAM(source, nalt),
                   const char ARR_PARAM(graph, nalt * nalt))
{
	size_t nsources = 0, i;
	for (i = 0; i < nalt; i++) {
		size_t j;
		source[i] = true;
		for (j = 0; j < nalt; j++) {
			if (graph[j * nalt + i]) {
				source[i] = false;
				break;
			}
		}
		if (source[i])
			nsources++;
	}
	return nsources;
}

#ifdef __GNUC__
__attribute__((nonnull (1)))
#endif
static void
cdor_fill_wcc (struct wcc_data REF(wcc), const size_t nalt,
               cdor_bool ARR_PARAM(assigned, nalt),
               const char ARR_PARAM(graph, nalt * nalt), const size_t start)
{
	size_t v;
	wcc->size[wcc->num]++;
	wcc->map[start] = wcc->num;
	assigned[start] = true;
	for (v = 0; v < nalt; v++) {
		if (v != start && !assigned[v]
		    && (graph[start * nalt + v] || graph[v * nalt + start])) {
			cdor_fill_wcc (wcc, nalt, assigned, graph, v);
		}
	}
}

#ifdef __GNUC__
__attribute__((nonnull (1)))
#endif
static cdor_bool
construct_wcc (struct wcc_data REF(wcc), const size_t nalt,
               const char ARR_PARAM(graph, nalt * nalt))
{
	size_t *r;
	cdor_bool *assigned;
	size_t v;
	if (!(wcc->size = zero_allocate(size_t, nalt)))
		return false;
	if (!(wcc->map = allocate(size_t, nalt))) {
		free (wcc->size);
		return false;
	}
	wcc->num = wcc->maxsz = 0;
	if (!(assigned = zero_allocate(cdor_bool, nalt))) {
		free (wcc->map);
		free (wcc->size);
		return false;
	}
	for (v = 0; v < nalt; v++) {
		if (assigned[v])
			continue;
		cdor_fill_wcc (wcc, nalt, assigned, graph, v);
		if (wcc->size[wcc->num] > wcc->maxsz)
			wcc->maxsz = wcc->size[wcc->num];
		wcc->num++;
	}
	free (assigned);
	if ((r = (size_t *) realloc (wcc->size, wcc->num * sizeof (size_t))))
		wcc->size = r;
	return true;
}

#ifdef __GNUC__
__attribute__((nonnull (2, 5)))
#endif
/* Passing wcc by value is necessary for g_wcc size specification */
static void
cdor_extract_wcc (const size_t nalt, const char ARR_PARAM(graph, nalt * nalt),
                  const struct wcc_data wcc, const size_t cur_wcc,
                  cdor_bool ARR_PARAM(g_wcc,
                                      wcc.size[cur_wcc] * wcc.size[cur_wcc]))
{
	size_t i = 0, wcc_i;
	for (wcc_i = 0; wcc_i < wcc.size[cur_wcc]; wcc_i++, i++) {
		size_t j = 0, wcc_j;
		while (wcc.map[i] != cur_wcc)
			i++;
		for (wcc_j = 0; wcc_j < wcc.size[cur_wcc]; wcc_j++, j++) {
			while (wcc.map[j] != cur_wcc)
				j++;
			g_wcc[wcc_i * wcc.size[cur_wcc] + wcc_j] = graph[i * nalt + j];
		}
	}
}

static cdor_bool
cdor_comp_strats (const size_t nalt, const double ARR_PARAM(left, nalt),
                  const cdor_bool ARR_PARAM(graph, nalt * nalt),
                  const double ARR_PARAM(right, nalt))
{
	double acc = 0.0;
	size_t i;
	for (i = 0; i < nalt; i++) {
		double b = 0.0;
		size_t j;
		for (j = 0; j < nalt; j++) {
			if (graph[i * nalt + j])
				b += right[j];
			else if (graph[j * nalt + i])
				b -= right[j];
		}
		acc += left[i] * b;
	}
	return acc <= 0.0;
}

#ifdef __GNUC__
__attribute__((nonnull (2, 3)))
#endif
static cdor_bool
cdor_optimal (const size_t nalt, double ARR_PARAM(dest, nalt),
              const cdor_bool ARR_PARAM(graph, nalt * nalt))
{
	assert(nalt >= 2);
	if (cdor_solve (nalt, dest, graph, true)) {
		double * const right = allocate(double, nalt);
		if (right && cdor_solve (nalt, right, graph, false)) {
			if (!cdor_comp_strats (nalt, dest, graph, right))
				memcpy (dest, right, nalt * sizeof (double));
		}
		free (right);
		return true;
	} else {
		return cdor_solve (nalt, dest, graph, false);
	}
}

static cdor_bool
cdor_solve_components (const size_t nalt, double ARR_PARAM(dest, nalt),
                       const char ARR_PARAM(graph, nalt * nalt))
{
	struct wcc_data wcc;
	double *strats, *strat_wcc = NULL, *strat, *term = NULL;
	cdor_bool *graph_wcc = NULL;
	size_t i, j;
	assert(nalt >= 2);
	if (!construct_wcc (&wcc, nalt, graph))
		return false;
	if (!(strats = allocate(double, wcc.num * nalt)))
		goto fail;
	if (!(graph_wcc = allocate(cdor_bool, wcc.maxsz * wcc.maxsz)))
		goto fail;
	if (!(strat_wcc = allocate(double, wcc.maxsz)))
		goto fail;
	strat = strats;
	for (i = 0; i < wcc.num; i++) {
		size_t v_wcc = 0;
		cdor_extract_wcc (nalt, graph, wcc, i, graph_wcc);
		if (!cdor_optimal (wcc.size[i], strat_wcc, graph_wcc))
			goto fail;
		for (j = 0; j < nalt; j++)
			strat[j] = wcc.map[j] == i ? strat_wcc[v_wcc++] : 0.0;
		strat += nalt;
	}
	free (graph_wcc);
	graph_wcc = NULL;
	if (!(term = (double *) realloc (strat_wcc, wcc.num * sizeof (double))))
		goto fail;
	for (i = 0; i < nalt; i++) {
		strat = strats;
		for (j = 0; j < wcc.num; j++) {
			term[j] = strat[i];
			strat += nalt;
		}
		dest[i] = cdor_sum (wcc.num, term) / (double) wcc.num;
	}
	free (term);
	free (strats);
	free (wcc.size);
	free (wcc.map);
	return true;
fail:
	free (term);
	free (strat_wcc);
	free (graph_wcc);
	free (strats);
	free (wcc.size);
	free (wcc.map);
	return false;
}

#ifdef __GNUC__
__attribute__((const, nonnull (1)))
#endif
static struct cdor_strategy
cdor_one_source (const cdor_bool * CDOR_RESTRICT const sources)
{
	struct cdor_strategy r = { CDOR_PURE, { 0 } };
	while (!sources[r.val.pure])
		r.val.pure++;
	return r;
}

static struct cdor_strategy
cdor_mixed_sources (const size_t nalt, const size_t nsources,
                    const cdor_bool ARR_PARAM(sources, nalt))
{
	struct cdor_strategy r = { CDOR_MIXED, { 0 } };
	const double coef = 1.0 / (double) nsources;
	size_t v;
	if (!(r.val.mixed = allocate(double, nalt))) {
		r.type = CDOR_ERROR;
		return r;
	}
	for (v = 0; v < nalt; v++)
		r.val.mixed[v] = sources[v] ? coef : 0.0;
	return r;
}

#ifdef __GNUC__
__attribute__((const))
#endif
static size_t
max_election_size()
{
	/*
	 * TODO: implement using integer squareroot
	 * Correct result is the smallest of:
	 * INT_MAX
	 * sqrt(SIZE_MAX / sizeof (cdor_bool))
	 * sqrt(SIZE_AMX / sizeof (double))
	 * SIZE_MAX / sizeof (size_t)
	 */
	return 127;
}

struct cdor_strategy
cdor_optimal_strategy (const size_t nalt, const char * CDOR_RESTRICT graph)
{
	struct cdor_strategy r = { CDOR_ERROR, { 0 } };
	if (nalt == 0 || nalt > max_election_size() || graph == NULL) {
#if _POSIX_C_SOURCE >= 1L
		errno = EINVAL;
#endif
		return r;
	}
	/* First strategy: sources */
	{
		cdor_bool * const sources = allocate(cdor_bool, nalt);
		size_t nsources;
		if (!sources)
			return r;
		nsources = cdor_find_sources (nalt, sources, graph);
		if (nsources > 0) {
			r = nsources == 1 ? cdor_one_source (sources) :
				cdor_mixed_sources (nalt, nsources, sources);
			free (sources);
			return r;
		}
		free (sources);
	}
	/* Second strategy: weakly-connected components */
	if (!(r.val.mixed = allocate(double, nalt)))
		return r;
	if (cdor_solve_components (nalt, r.val.mixed, graph))
		r.type = CDOR_MIXED;
	else
		free (r.val.mixed);
	return r;
}

/*
 * TODO:
 * factorize to write maximin more easily
 */
