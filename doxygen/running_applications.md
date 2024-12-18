# Running applications

[TOC]

## Application system

The idea of the application system that we use is that the main function always stay the same: it merely parses the command-line arguments to decide which application (which can be thought of as the effective main function) should be ran.

**Running an application:** `./{debug|release}_inf app [parameters]`

This requires having compiled at least once.

*Examples:* 
- `./bin/debug.exe srb` (debug mode, application `srb`)
- `./bin/release.exe ejm` (release mode, application `ejm`)

**Obtaining the existing application names:**

Use `./{debug|release}_inf` without any parameter to display the registered application names.

## Timers

The applications will typically displayed three times: 
- the "opt" time corresponds to the time recorded by inf::Optimizer::total_optimization_chrono.
- the "fw" time corresponds to the time recorded by inf::FrankWolfe::total_fw_chrono.
- the "tot" time corresponds to the time recorded by util::Chrono::process_clock.

## Test system

Some applications can be flagged as being a test.
Ideally, such applications should be short to execute and test basic functions of the code.
For this, they should use hard assertions (see \ref debug-vs-release-modes "debug vs release").
Then, all tests can be ran directly by calling the special application name `test`, e.g., with `./debug_inf test`.
This will execute all applications registered as a test but bypassing their output and merely showing whether or not the test is passed, i.e., whether or not any assertion failed.

\section CLIparams Command-line parameters

Currently, the optional command-line parameters, apart from the application name `app`, are as follows:
- `--verb` : controls the level of output produced. See the \ref printing "printing module" for more information.
- `--threads` : sets the number of threads used by some parts of the program, see inf::FullyCorrectiveFW and inf::TreeOpt for more information about how multithreading is used.
- `--symtree-io {none|read|write}` : sets whether the inflation event tree should be read/written to disk, see inf::EventTree::IO for more information. 

\warning Note that since this parameter is `read` by default, a fresh installation of the codebase may produce errors because the event tree have not been written to the `data/` folder yet. In this case, it is necessary to re-run the failed application passing `--symtree-io write` in the command-line parameters to compute the event tree and write it to disk (thus incurring a longer execution time). The next runs can then omit this since the event tree will be successfully read from disk. 

- `--vis` : this parameter is used by some applications to pick a visibility, i.e., to select a target distribution from a one-parameter family of distributions. See  the application user::ejm.

## Creating new applications

Please refer to the \ref user "application interface" module.

