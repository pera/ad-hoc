# Ad-hoc programming language

Ad-hoc is an experimental programming language currently supporting the following features: first-class functions, immutability, lambda terms, recursive definitions, closures, and lexical and dynamic scopes.

## Building Instructions

### Dependencies

* GCC 6 (or later)
* GNU Bison 3
* Flex
* GNU Make
* GNU Readline
* *(optionals)* GDB, Valgrind, DejaGnu

### Build and start the REPL

```sh
$ make
$ ./ahci
```

## Syntax

In Ad-hoc everything is an expression: a program is composed as a list of expressions delimited by the `;` token, and this list is itself an expression (the result of evaluating *expr<sub>1</sub> ; expr<sub>2</sub> ; ... ; expr<sub>n</sub>* is the value of *expr<sub>n</sub>*).

The syntax to define a new variable, named *num*, as the number 2600 is `num : 2600`. Variables are immutable, therefore any attempt to redefine a variable would result in an error. Note though that shadowing is possible in Ad-hoc.

Functions have two parts: an argument list where each identifier is delimited by the `,` token, and the body (an expression list). For instance, one could write a function that multiplies a number by 2 as `[ x | x * 2 ]`, or another function that sums two numbers as `[ x, y | x + y ]`.

For function application there are two options available: the *fun@params* syntax (which is right-associative, and where the parameter list has high precedence), and the *fun{params}* syntax (which is left-associative, and where the parameter list has low precedence). So for example, the expression `f @ 1, 2 - 3` is equivalent to `(f @ 1, 2) - 3`. If instead we wanted to do the substraction first we could write `f @ 1, (2 - 3)` or `f {1, 2 - 3}`.

This is how an interactive session with *ahci*, the Ad-hoc interpreter, looks like (the **λ** symbol is the prompt):
```
λ fib : [ n | if n<2 [n] [fib{n-1} + fib{n-2}] ]
==> function [0x1221c20]
λ fib@10
==> 55
```
Since the REPL uses the GNU Readline library to manage the user input, you can use all the standard key bindings, like the Up/Down-arrow keys for instance, to navigate the current session history. To quit use either ctrl+d or ctrl+c.
