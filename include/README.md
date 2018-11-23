#### Project structure

Here is what each folder contains:

* `cli` is where the library interface and the simple REPL are defined
* `compiler` parses a code string and compiles it into bytecode
* `util` a few helper things like resizeable arrays and hash tables
* `vm` the bytecode runner

There are also a few files in this directory:

 + `lit.h` the must-include header to work with lit
 + `lit_common.h` some useful includes like bools and int types
 + `lit_debug.h` provides some pretty abstract syntax tree (AST) and bytecode outputs
 + `lit_mem_manager.h` provides a simple interface for controlling memory allocation (and leaks)