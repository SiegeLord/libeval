/*
** simple expression evaluator library (recursive descent parser)
** Copyright (C) 2006  Jeffrey S. Dutky
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef EVAL_EXPR_H
#define EVAL_EXPR_H

/* set a named variable used by the eval() function */
int eval_set_var(char *name, double value);

/* get the value of a named variable as used by eval() */
int eval_get_var(char *name, double *value);

/* the FUNCTION() macro is used to declare user-defined functions that can
** be passed to eval_def_fn() for inclusion in the evaluation environment.
**
** the FuncitonPtr typedef defines pointers to user-defined fuctions declared
** using the FUNCTION() macro. */
#define FUNCTION(NAME,ARGS,ARG,RV,DATA) int NAME(int ARGS, double *ARG, double *RV, void *DATA)
typedef FUNCTION((*FunctionPtr),args,arg,rv,data);

/* define a function for use by eval() */
int eval_def_fn(char *name, FunctionPtr fn, void *data, int args);

/* evaluate an arithmetic expression consisting of numeric literals,
** named variables, addition (+), subtraction (-), multiplication (*),
** division (/), modulo division (\), exponentiation (^), sign change (+-)
** function evaluation (f()) and grouping (()) operators. The expression
** is passed as a standard C string in the expr parameter. The evaluated
** value is saved into the result parameter. The function returns 0 (zero)
** on success, non-zero on error */
int eval(char *expr, double *result);

/* return information about the expression evaluator, copyright, auther, etc. */
void eval_info(int *version, int *revision, int *buildno,
	char *authbuf, int authlim, char *copybuf, int copylim,
	char *licebuf, int licelim);

/* interpret an error code returned by eval_expr as a human readable string */
const char *eval_error(int err);

/* setup the default functions and variables */
int eval_set_default_env(void);

#endif
