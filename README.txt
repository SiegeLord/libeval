libeval: simple arithmetic expression evaluation library

OVERVIEW

  Libeval is a (very) simple expression evaluator based on a recursive
  descent parser (the sort that you might write in a third year Concepts
  of Programming Lanuages class, or as an early project in a Compiler
  Design class before they showed you how this aught to be done).

  Libeval accepts the basic arithmetic operators: add (+), subtract (-),
  multiply (*), divide (/), modulo divide (\), exponent (^), grouping (()),
  function evaluation (()), sign change (+-), percentages (%), numeric
  literal values and scaler variables.

  You can evaluate an expression by calling the eval() function. eval()
  takes two parameters, the expression to evaluate (as a simple C string)
  and a reference to a double precision float in which to put the result.
  If eval() encounters an error it returns a non-zero value, otherwise,
  if everything went well, it returns zero.

  The error code returned by eval() can be converted into a human readable
  string by the eval_error() function. eval_error() takes one parameter,
  the error code returned by eval(),and returns a constant string describing
  the error.

  Variables can be manipulated with the eval_set_var() and eval_get_var()
  functions.

  eval_set_var() sets the named variable to the specified double precision
  value. eval_set_var() takes two parameters, the name of the variable to
  set as a simple C string, and the double precision float value to set the
  variable to. It returns 0 (zero) on success, non-zero on failure.

  eval_get_var() gets the value of the named variable. eval_get_var() takes
  two parameters, the name of the variable as a simple C string and a
  reference to a double precision float in which to store the variables
  value. It returns 0 (zero) on success, non-zero on failure.
  
  Functions can be defined with the eval_def_fn() function, which takes
  the name of the function as a simple C string, a pointer to a C function
  implementing the function, a pointer to a block of custom storage for use
  by the function and the number of arguments taken by the function. The
  prototype for the implementation function is:
  
    int fn(int args, double *arg, double *rv, void *data);
  
  The first two parameters (args and arg) are similar to the standard
  parameters to the main() function in C, the args parameter indicates
  how many elements are the argument list, and arg is the argument list
  itself. The third parameter (rv) is the return value from the function.
  The last parameter (data) is the custom storage block passed in when
  the function was defined.
  
  If you specify a positive value (including zero) as the number of arguments
  for a function, libeval will only all the function to be called with exactly
  that number of parameters. If you specify a -1 (negative one) for the number
  of arguments, the function can be called with any number of parameters.
  
  The following functions and constants can are predefined when 
  eval_set_default_env() is called:
  
    abs(x)      absolute value of x
    sign(x)     sign of x (1.0 or -1.0)
    int(x)      integer part of x
    round(x)    round x to nearest integer
    trunc(x)    truncate x (same as int(x))
    floor(x)    round x to nearest lesser integer
    ceil(x)     round x to nearest greater integer
	
    sin(x)      sine of x (radians)
    cos(x)      cosine of x (radians)
    tan(x)      tangent of x (radians)
	
    asin(x)     arc sine of x (radians)
    acos(x)     arc cosine of x (radians)
    atan(x)     arc tangent of x (radians)
	
    sinh(x)     hyperbolic sine of x (radians)
    cosh(x)     hyperbolic cosine of x (radians)
    tanh(x)     hyperbolic tangent of x (radians)
	
    asinh(x)    hyperbolic arc sine of x (radians)
    acosh(x)    hyperbolic arc cosine of x (radians)
    atanh(x)    hyperbolic arc tangent of x (radians)
	
    deg(x)      convert radians to degrees
    rad(x)      convert degrees to radians
	
    ln(x)       natural logarithm of x
    log(x)      base 10 logarithm of x
    sqrt(x)     square root of x
    exp(x)      e to x power
	
    rand()      random number between 0.0 and 1.0
    fact(x)     factorial of x (or gamma(x) if x is non-integer)
	
    sum(...)    sum of the arguments
    min(...)    minimum value in arguments
    max(...)    maximum value in arguments
    avg(...)    average of arguments
    med(...)    median of arguments
    var(...)    variance of arguments
    std(...)    standard deviation of arguments
    
    pi          3.1415...
    e           2.7182...

  Finally, you can get a set of bookkeepping information about the libeval
  libray with the eval_info() function. eval_info() takes nine parmaeters:
  three references to integer values for the version, revision and build
  numbers of the current libeval library, and three pairs of buffer address
  and buffer size limit for author's name, copyright info and license info.

  You can use libeval by including the libeval header in your program source
  
    #include <eval.h>

  and then by linking your program against the libeval library
  
    gcc -o myprog myprog.c -leval

BUILD & INSTALL

  you can build the libraries by simply typing

    make libs

  at the command line. This will build both the static library and the shard
  library.

  The libraries and the header file can be installed to the default location
  (/usr/local/lib and /usr/local/incude) by typing

    make install

  at the command line. If you want a different install location, you will have
  to edit the make file (Makefile) and change the value of INSTALLDIR to your
  preferred location (sorry).

  Other possible make targets include:

    clean       delete build products (*.o, binaries, libs, etc.)
    test        build the test shell (lets you play with eval() at the CLI)
    backup      make a backup of the source
    dist        make a distribution package of the source
    remove      remove the current version of the library from the install dir
    remove-all  remove all revision of the current version of libeval

  I've only really tested this on Linux, but it ought to work on just about
  anything that supports an ANSI C compiler. You WILL have to make signifcant
  modifications to the Makefile, however, in order to compile for anything
  other than Linux (I know that the command line for building shared libs on
  MacOS X or a DLL on Cygwin is quite different from the Linux version).

DOCUMENTATION

  You're looking at it. I haven't had time to put together a proper manual
  page for libeval just yet. Sorry.

LICENSING & COPYRIGHT

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or (at
  your option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation,
  Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The libeval library is copyright (C) 2006, Jeffrey S. Dutky.

CONTACT INFOMATION

  You can contact me by e-mail at dutky@bellatlantic.net to ask questions or
  report bugs.
