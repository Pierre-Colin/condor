# Copyright (C) 2024 Pierre Colin
# This file is part of Condor.
# 
# Condor is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, version 3.
# 
# Condor is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
# more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with Condor.  If not, see <https://www.gnu.org/licenses/>.
SHELL = /bin/sh
CFLAGS = -pedantic -Wall -Wextra -Wconversion -Wshadow -fanalyzer -Og -g -fpic
OBJ = cast_ballot.o make_duel_graph.o optimal_strategy.o
TEXI = manual/condor.texi manual/cast_ballot.texi manual/custom-build.texi \
	manual/fdl-1.3.texi manual/make_duel_graph.texi \
	manual/optimal_strategy.texi manual/simple-build.texi manual/types.texi
TEXI2HTML = makeinfo --html --no-split
TEXI2PS = makeinfo --ps
TEXI2PDF = makeinfo --pdf

.SUFFIXES:
.SUFFIXES: .c .o

all: libcondor.a libcondor.so test

test: test.o libcondor.so
	$(CC) -L$(shell pwd) -flto $(CFLAGS) -o $@ $< -lcondor -llpsolve55

libcondor.a: $(OBJ)
	$(AR) -crs $@ $(OBJ)

libcondor.so: $(OBJ)
	$(CC) -shared $(CFLAGS) -o $@ $(OBJ)

cast_ballot.o: cast_ballot.c condor.h util.h
make_duel_graph.o: make_duel_graph.c condor.h util.h
optimal_strategy.o: optimal_strategy.c condor.h util.h
test.o: test.c condor.h util.h

info: condor.info
dvi: condor.dvi
html: condor.html
pdf: condor.pdf
ps: condor.ps

condor.info: $(TEXI)
	$(MAKEINFO) manual/condor.texi

condor.dvi: $(TEXI)
	$(TEXI2DVI) manual/condor.texi

condor.html: $(TEXI)
	$(TEXI2HTML) manual/condor.texi

condor.pdf: $(TEXI)
	$(TEXI2PDF) manual/condor.texi

condor.ps: $(TEXI)
	$(TEXI2PS) manual/condor.texi

clean:
	rm -f libcondor.a libcondor.so $(OBJ) test.o test \
		condor.{aux,cp,cps,dvi,fn,fns,info,log,pdf,ps,toc,tp,tps}

dist: clean
	mkdir condor-0.1
	mkdir condor-0.1/manual
	cp Makefile $(OBJ:.o=.c) test.c util.h condor.h condor.h.3 \
		COPYING{,.LESSER} condor-0.1/
	cp $(TEXI) condor-0.1/manual/
	tar -czf condor-0.1.tar.gz condor-0.1
	rm -fr condor-0.1

.PHONY: all clean dist dvi html info pdf ps
