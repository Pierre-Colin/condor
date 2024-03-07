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

Usage
-----

The whole C API is documented in the manual page `condor.h.3`.

Current state
-------------

The core library is likely finished. It passes the tests for its only complex
function, the other ones being relatively trivial. The C++ header appears to
be functional, but more tests are needed. In the future, it will be merged with
the C header. A Python 3 package is in development for an upcoming demo.

License
-------

Condor and its dependency lpsolve 5.5 are both released under the GNU
[LGPLv3](https://www.gnu.org/licenses/licenses.html#LGPL).
