Condor
======

Version **0.1.0**

Description
-----------

**Condor** is a C library implementing the Randomized Condorcet Voting System,
an non-deterministic electoral system with game-theoretic properties getting
very close to incentive compatibility and independence of irrelevant
alternatives. It relies on
[lpsolve 5.5](https://sourceforge.net/projects/lpsolve/) to perform the
optimization needed to compute the strategy.

Installing
----------

Running the `make` command should be enough to build the library as well as
the unit tests. The `Makefile` contains easily-customizable parameters if
tinkering is needed. To run the tests:
```bash
LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH ./test
```

To build the manual, use any of the following depending on what format you want
it in. You will need [GNU Texinfo](https://www.gnu.org/software/texinfo/).
```bash
make info
make dvi
make html
make ps
make pdf
```

Usage
-----

The whole C API is documented in the manual page `condor.h.3` as well as in the
manual.

Current state
-------------

The core library is likely finished. It passes the tests for its only complex
function, the other ones being relatively trivial. The C++ header appears to
be functional, but more tests are needed. In the future, it will be merged with
the C header. A Python 3 package is in development for an upcoming demo.

The Condor manual contains basic install instructions as well as the reference.
However, it lacks a long chapter setting forth the motivations and the
underlying theory.

License
-------

Condor and its dependency lpsolve 5.5 are both released under the GNU
[LGPLv3](https://www.gnu.org/licenses/licenses.html#LGPL).

The Condor manual is released under the GNU
[FDLv1.3+](https://www.gnu.org/licenses/licenses.html#FDL).
