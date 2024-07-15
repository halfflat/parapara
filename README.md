# Parapara

Parapara is a C++ header library designed to read, parse, and write sets of parameters corresponding to
fieleds within some struct. It is a proof of concept — it might become a useful standalone library in
the fullness of time, but for now it is a testbed for use in a different application.

Targets C++17 or maybe C++20 if the lack of concepts becomes too annoying.

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

In the first instance, only a simple TOML-like subset that is compatible with the target application's
conventions.

