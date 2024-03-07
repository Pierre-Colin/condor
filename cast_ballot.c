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
cdor_cast_ballot (const size_t nalt, cdor_adv * CDOR_RESTRICT duels,
                  int (*ballot) (size_t, size_t))
{
	size_t i, j;
	for (i = 1; i < nalt; i++) {
		for (j = 0; j < i; j++) {
			const int cmp = ballot (i, j);
			if (cmp < 0)
				duels[j * nalt + i]++;
			else if (cmp > 0)
				duels[i * nalt + j]++;
		}
	}
}
