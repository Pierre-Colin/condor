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
#ifndef CONDOR_H_INCLUDED
#define CONDOR_H_INCLUDED

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

struct cdor_strategy
{
	enum { CDOR_ERROR, CDOR_PURE, CDOR_MIXED } type;
	union { size_t pure; double *mixed; } val;
};

#if __cplusplus >= 201103L || __STDC_VERSION__ >= 199901L || defined __GNUC__
typedef unsigned long long cdor_adv;
#else
typedef unsigned long cdor_adv;
#endif

extern void cdor_cast_ballot (size_t, cdor_adv *, int (*) (size_t, size_t));
extern void cdor_make_duel_graph (size_t, char *, const cdor_adv *);
extern struct cdor_strategy cdor_optimal_strategy (size_t, const char *);

#ifdef __cplusplus
}
#endif

#endif /* CONDOR_H_INCLUDED */
