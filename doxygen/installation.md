# Installation and compilation

[TOC]

## Prerequisites

A prerequisite to be able to compile and use the code is the C++ Fusion Mosek API (both the header C++ files, the binaries, and a valid license).
Please refer to https://docs.mosek.com/latest/cxxfusion/install-interface.html for details on how to get there.
Alternatively, the code could be used without Mosek: this would require modifying a little bit the makefile and the source code to remove the very efficient inf::FullyCorrectiveFW, and only use inf::PairwiseFW (or a variant thereof) which does not require Mosek.

## Compilation

The compilation of the code is automated with a makefile which requires [GNU make](https://www.gnu.org/software/make/manual/html_node/index.html).
The makefile automatically recompiles files when needed, and forward its command-line arguments to the project executable (either `./debug_inf` or `./release_inf`). 
The following is a summary of the commands available from the root of the project directory.

**Preparing the `bin` folder:** `make prepare`

This creates the subdirectories in the `bin` folder that are necessary for the purpose of the automatic makefile.
This command should be ran once after cloning the directory, and again everytime after running `make clean`.

**Compiling:** `make {d[ebug]|r[elease]} [parameters]`

Compiles (or rather, re-compiles as needed) in debug or release mode, thus creating the executable `./debuf_inf` or `./release_inf`.
The corresponding executable is then called with the optional `parameters`.
If the parameters are left empty, this simply prints out the available application names.
See [running applications](doxygen/running_applications.md) for further details on running applications.
See \ref debug-vs-release-modes "debug vs release" for further details on the difference between debug and release modes.

*Examples:* Either `make release` or `make debug`, or just `make r` or `make d` for short.

**Miscellaneous:**

- Create the documentation with `make doc` (prerequisite : `doxygen` (from [Doxygen](https://www.doxygen.nl/manual/index.html)))
- Clean the `./bin/` directory as well as the executables `./debug_inf` and `./release_inf` with `make clean`.

\subsection debug-vs-release-modes Debug vs release modes

The `debug` mode uses `ASSERT_TRUE` statements, bound checking of 
`std::vector::operator[]`, etc., whenever sensible (in particular, with no concern for performance).
These additional safety mechanisms are ignored in `release` mode, because a correct use of the code ought to not trip
any of these assertions.
However, hard assertions such as `HARD_ASSERT_TRUE` are actively checked in both `debug` and `release` mode.
Conceptually, a hard assertion is one that ought to be tripped by high-level design errors.
In particular, the tests ran when invoking `make {d[ebug]|r[elease]} test` feature such hard assertions.

Furthermore, during development, re-compiling in `debug` mode is recommended, since it is
typically faster then in `release` mode. Indeed, `debug` mode compilation involves dynamically linking
the different modules together, while `release` mode statically re-links all of the modules together every time,
which is slower at link time, but potentially enables some minor performance improvements.
In addition, `release` mode features the `-O3` [optimization flag of GCC](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html).

To summarize:
|                      | `debug`                             | `release`                           |
|----------------------|-------------------------------------|-------------------------------------|
| Soft assertions      | Active                              | Inactive                          |
| Hard assertions      | Active                              | Active                              |
| Compilation stategy  | Dynamic linking of multiple modules | Static linking of the whole project |
| Re-compilation speed | Faster                           | Slower                            |
| Runtime speed        | Slower                           | Faster                            |


