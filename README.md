# Ad-hoc programming language

Ad-hoc is an experimental programming language currently supporting the following features: first-class functions, immutability, lambda terms, recursion, closures, strict and non-strict evaluation, lexical and dynamic scopes, and deep binding.

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

## Language introduction

In Ad-hoc almost everything is an expression: a program is composed as a list of expressions delimited by the `;` token, and this list is itself an expression (the result of evaluating *expr<sub>1</sub> ; expr<sub>2</sub> ; ... ; expr<sub>n</sub>* is the value of *expr<sub>n</sub>*).

The syntax to define a new variable, named *num*, as the number 2600 is `num := 2600`. Variables are immutable, therefore any attempt to redefine a variable would result in an error (note though that variables can be shadowed on an inner scope). The use of free variables is supported in Ad-hoc.

Functions have two parts: an argument list where each identifier is delimited by the `,` token, and the body (an expression list). For instance, one could write a function that multiplies a number by 2 as `[ x | x * 2 ]`, or another function that sums two numbers as `[ x, y | x + y ]`.

For function application there are two options available: the *fun@params* syntax (which is right-associative, and where the parameter list has high precedence), and the *fun{params}* syntax (which is left-associative, and where the parameter list has low precedence). So for example, the expression `f @ 1, 2 - 3` is equivalent to `(f @ 1, 2) - 3`. If instead we wanted to do the subtraction first we could write `f @ 1, (2 - 3)` or `f {1, 2 - 3}`.

Shown below is an example of a factorial function defined with a strict fixed point combinator:
```
Y := [f | [x | x@x] @ [g | f @ [a | (g@g)@a]]];
fac := Y @ [f | [x | if x<2 [1] [x*f{x-1}]]];
```

As we have already shown, mathematical expressions are written using infix notation. Aside from the well-known symbols for addition (`+`), subtraction (`-`), multiplication (`*`) and division (`/`), Ad-hoc also provides `**` for exponentiation, `//` for the *n*th root, `mod` for modulo and `log` for logarithm.

Binary relations (i.e. `=`, `<`, `<=`, `>`, `>=`) are also expressed as usual, and they evaluate to a boolean value. The keywords for boolean literals are `true` and `false`, and the logical operators are `and`, `or` and `not`.

List literals are expressed with curly brackets (e.g. `{1, 2, 3}`) and a few built-in functions are available to operate over lists, these are: `append`, `head`, `tail`, `reverse` and `length`. Some common higher-order functions are also available, such as: `map`, `foldr`, `foldl`, `scanr`, `scanl` and `filter`.

A *thunk* in Ad-hoc is a delayed expression, in some way similar to a function in the sense that it's a list of expressions that doesn't evaluate immediately and that it have its own environment. The main difference is that with a thunk the evaluation will only happen either when the value is needed or when explicitly forced. The syntactic form is *[expr]*, and for forcing, the postfix `!` operator is provided. For instance, the ω combinator (i.e. *λx.xx*) can be written as `[x|[x@x]]` so when applied to itself, instead of eagerly diverge, it will return a thunk. This is useful to write lambda terms almost in the same way one would do it in lambda calculus, so knowing this we could rewrite the previous fixed point combinator like the usual call-by-name Y combinator: `Y := [f | [x | [f{x@x}]] @ [x | [f{x@x}]]]`. Another interesting thing that we can accomplish with thunks (but not necessarily a good practice ^.^) is for example, accessing variables in inner scopes as we do here: `[x | a:=2; a*x] @ [a+1]` (which, since *a* is a free variable in the context of the thunk, evaluates to 6).

## The REPL

This is how an interactive session with *ahci*, the Ad-hoc interpreter, looks like (the **λ** symbol is the prompt):
```
λ fib := [n | if n<2 [n] [fib{n-1} + fib{n-2}]]
==> function [0x1221c20]
λ fib@10
==> 55
```

Since the REPL uses the GNU Readline library to manage the user input, you can use all the standard key bindings, like the Up/Down-arrow keys for instance, to navigate the current session history. To quit use either ctrl+d or ctrl+c.
