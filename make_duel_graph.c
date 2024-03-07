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
#include <stddef.h>

#include "condor.h"
#include "util.h"

void
cdor_make_duel_graph (const size_t nalt, char * CDOR_RESTRICT graph,
                      const cdor_adv * CDOR_RESTRICT duels)
{
	size_t i, j;
	if (nalt == 0)
		return;
	graph[0] = 0;
	for (i = 1; i < nalt; i++) {
		graph[i * nalt + i] = 0;
		for (j = 0; j < i; j++) {
			const size_t l = i * nalt + j, r = j * nalt + i;
			graph[l] = duels[l] > duels[r];
			graph[r] = duels[r] > duels[l];
		}
	}
}
