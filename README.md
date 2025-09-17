# Parapara

Parapara is a C++17-compatible header library designed to read, parse, and write sets of parameters corresponding to
fieleds within some struct. It is very much still under development.

## Design goals

* Individual parameter values are parsed from a string where the post-parsing validation and
  failure text are specifiable.

* Parameter values are assigned to members of a struct; the particular member is provided by a pointer
  to member value.

* Sets of parameter values can be used to parse and assign value from a file or from command line
  arguments.

* Error behaviour can be specified at run-time: a parsing or validation failure can throw or the
  failure information returned to the caller.

* New types and representations can be supported through extending a set of readers and (for saving
  parameter set values) writers.


## Supported formats

In the first instance, only an INI-style file format is supported with a specialized importer.

## Building tests and examples

(To be expanded)

Basic approach:
```
    mkdir b
    cd b
    ln -s ../Makefile .
    make
```

The Makefile uses `CXX` for the C++ compiler, `CP` for the `cp` command, as well as other variables that
can be used to customize the build (`CXXSTD`, `OPTFLAGS` — refer to Makefile).

For a MacPorts environment with a modern clang++, GNU Make, and GNU Coreutils installed use:
```
    gmake CXX=clang++ CP=gcp
```

