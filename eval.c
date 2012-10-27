/*
** simple expression evaluator library (recursive descent parser)
** Copyright (C) 2006, 2007  Jeffrey S. Dutky
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
**
** 20 Dec 2006 - v1.0.0 - initial release
** 21 Dec 2006 - v1.0.1 - code cleanup (FUNCTION() macro, FunctionPtr typedef)
** 23 Dec 2006 - v1.0.2 - added recursive check to eval(), fixed fact grammer
** 24 Dec 2006 - v1.0.3 - fixed round() and trunc(), added factorial function
** 26 Dec 2006 - v1.0.4 - fixed typos, added sign(), Makefile enhancements
**  3 Jan 2006 - v1.0.5 - fixed typos, fixed tanh() and acosh() functions
**  5 Jan 2007 - v1.0.6 - changed info strings and added build date
**  6 Jan 2007 - v1.0.7 - fixed stuff done in 1.0.6 for info strings and dates
**  6 Jan 2007 - v1.0.8 - fixed bug in var() and version string construction
*/

/* simple recursive descent parser for arithmetic expressions
**
** Copyright (C) 11-17 Dec. 2006, Jeffrey Dutky
**
** supports numeric literals, variables, addition (+), subtraction (-),
** multiplication (*), division (/), modulo division (\), exponentiation (^),
** grouping (()), sign change (-+), percentages (%) and function evaluation (()) 
**
**   expr = term+expr | term-expr | term
**   term = fact*term | fact/term | fact\term | fact
**   fact = item^fact | item
**   item = +item | -item | num | var | fn(args) | item% | (expr)
**   args = expr,args | expr
*/

/*** a comment like this (starting with /***) indicates unimplemented stuff */

static int G_version = VER;
static int G_revision = REV;
static int G_buildno = BLD;
static char *G_copyright = "libeval Copyright (C) 2006, 2007  Jeffrey S. Dutky";
static char *G_author = "libeval author: Jeffrey S. Dutky";
static char *G_license = "libeval license: GNU Lesser General Public License (LGPL) v2.1";
#include "version_str.h"
#include "build_date.h"
#include "package_date.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "hashtable.h"
#include "func.h"
#include "eval.h"

#ifdef EVAL_DEBUG
#define DB(X) X
#else
#define DB(X)
#endif

typedef struct Node_struct Node;
struct Node_struct { /* automatic free node */
	Node *link;
	char data[1];
};

static Node *G_cur_node = NULL; /* head of automatic free list */
static size_t G_cur_data_pos = 0; /* current position in head node */
static size_t G_cur_data_size = 0; /* size of data in head node */
#define MIN_ALLOC_SIZE 5000

/* allocate memory for later automatic clean-up*/
static void *lalloc(size_t size)
{
	size_t asize;
	void *ptr;
	Node *n;
	
	DB(printf("-- lalloc(size=%d)\n", size));
	if(size > G_cur_data_size-G_cur_data_pos)
	{ /* if there is not enough space in the current block */
		if(size < MIN_ALLOC_SIZE)
			asize = MIN_ALLOC_SIZE; /* never alloc less than MIN_ALLOC_SIZE */
		else
			asize = size+MIN_ALLOC_SIZE; /* alloc more than was asked for */
		/* allocate a new block large enough to hold the requested amount */
		DB(printf("-- lalloc new block %d bytes\n", asize));
		n = (Node*)malloc(sizeof(Node)+asize); /* alloc new block */
		if(n == NULL)
			return NULL; /* failed to alloc new block */
		DB(printf("-- lalloc new block at %p (old block at %p)\n",
			n, G_cur_node));
		n->link = G_cur_node; /* link current list head to new block */
		G_cur_node = n; /* new block becomes new list head */
		DB(printf("-- lalloc head block now %p\n", G_cur_node));
		G_cur_data_pos = 0; /* alloc from start of new block */
		G_cur_data_size = asize; /* alloc up to asize bytes from new block */
		DB(printf("-- lalloc new block size %d bytes\n", G_cur_data_size));
	}
	ptr = G_cur_node->data+G_cur_data_pos; /* return pointer to current byte */
	DB(printf("-- lalloc old data pos = %d\n", G_cur_data_pos));
	G_cur_data_pos += size; /* move alloc pos forward by alloced length */
	DB(printf("-- lalloc new data pos = %d\n", G_cur_data_pos));
	DB(printf("-- lalloc ptr=%p\n", ptr));
	
	return ptr;
}

/* free all memory previously allocated by lalloc */
static void lfreeall(void)
{
	Node *n, *t;
	
	n = G_cur_node;
	while(n != NULL)
	{
		t = n->link;
		free(n);
		n = t;
	}
	G_cur_node = NULL;
	G_cur_data_pos = 0;
	G_cur_data_size = 0;
	
	return;
}

typedef struct
{
	double value; /* variable value */
	FunctionPtr fn; /* function pointer */
	int nargs; /* function argument count expected */
	void *data; /* used by function call */
	char name[1]; /* name of function, array sized when allocated */
} VarFn;

/* create a new variable structure with the given name and value */
static VarFn *create_var(char *name, double value)
{
	VarFn *vf;
	
	vf = (VarFn*)malloc(sizeof(VarFn)+strlen(name));
	if(vf != NULL)
	{
		vf->value = value;
		vf->fn = NULL;
		vf->nargs = 0;
		vf->data = NULL;
		strcpy(vf->name, name);
	}
	
	return vf;
}

/* create a new function structure with the given name, pointer and args */
static VarFn *create_fn(char *name, FunctionPtr fn, int args, void *data)
{
	VarFn *vf;
	
	vf = (VarFn*)malloc(sizeof(VarFn)+strlen(name));
	if(vf != NULL)
	{
		vf->value = 0.0;
		vf->fn = fn;
		vf->nargs = args;
		vf->data = data;
		strcpy(vf->name, name);
	}
	
	return vf;
}

static hashtable *G_varfn_table = NULL;
static int G_var_count = 0;

/* compute hash code based on up to the first 32 bytes of the key string */
static unsigned int vhash(void *key)
{
	char *k;
	int i, h = 0;
	
	if(key == NULL)
		return 0;
	k = (char*)key;
	for(i = 0; k[i] && i < 32; i++)
		h += k[i]<<i;
	
	return h;
}

/* compare two key strings, allowing NULLs */
static int vcomp(void *key1, void *key2)
{
	if(key1 == NULL && key2 == NULL)
		return 0;
	if(key1 == NULL)
		return -1;
	if(key2 == NULL)
		return 1;
	return strcmp((char*)key1, (char*)key2);
}

/* delete the variable value */
static void vdel(void *val)
{
	if(val != NULL)
		free(val);
	
	return;
}

/* public: variable access (set) function */
int eval_set_var(char *name, double value)
{
	VarFn *var;
	
	set_funcs();
	
	if(G_varfn_table == NULL)
	{ /* allocate the var table */
		G_varfn_table = ht_create(500, vhash, vcomp, NULL, vdel);
		if(G_varfn_table == NULL)
			return 1;
	}
	/* find named var, update value or insert var/value */
	if(ht_lookup(G_varfn_table, name, (void*)&var))
	{ /* not found, insert new variable */
		var = create_var(name, value);
		if(var == NULL)
			return 2;
		if(ht_insert(G_varfn_table, (void*)(var->name), (void*)var))
			return 3;
		G_var_count++;
	}else if(var->fn != NULL)
		return 4;
	else
		var->value = value;
	
	return 0;
}

/* public: variable access (get) function */
int eval_get_var(char *name, double *value)
{
	VarFn *var;
	
	set_funcs();
	
	if(G_varfn_table == NULL)
	{ /* allocate the var table */
		G_varfn_table = ht_create(500, vhash, vcomp, NULL, vdel);
		if(G_varfn_table == NULL)
			return 1;
	}
	/* find named var, return value or error if not found */
	if(ht_lookup(G_varfn_table, name, (void*)&var))
		return 2; /* not found */
	if(var->fn != NULL)
		return 3; /* this is a funciton, NOT a variable */
	if(value != NULL)
		*value = var->value;
	
	return 0;
}

int eval_def_fn(char *name, FunctionPtr fn, void *data, int args)
{
	VarFn *f;
	
	set_funcs();
	
	if(G_varfn_table == NULL)
	{ /* allocate new fn table */
		G_varfn_table = ht_create(500, vhash, vcomp, NULL, vdel);
		if(G_varfn_table == NULL)
			return 1; /* failed to create table */
	}
	if(ht_lookup(G_varfn_table, name, (void*)(&f)))
	{
		f = create_fn(name, fn, args, data);
		if(f == NULL)
			return 2; /* failed to create new entry */
		if(ht_insert(G_varfn_table, (void*)(f->name), (void*)f))
			return 3; /* insert failed */
	}else if(f->fn == NULL)
		return 4; /* this is a variable, NOT a function */
	else
	{
		f->fn = fn;
		f->data = data;
		f->nargs = args;
	}
	
	return 0;
}

/* make a malloc()'d copy of a string, up to lim chars, full strlen if lim<1 */
static char *copy_str(char *str, int lim)
{
	int len;
	char *s;
	struct node *n;
	
	DB(printf("-- copy_str(\"%s\", lim=%d)\n", str, lim));
	len = strlen(str);
	DB(printf("-- copy len(\"%s\")=%d\n", str, len));
	if(lim <= 0)
		lim = len;
	if(len > lim)
		len = lim;
	DB(printf("-- copy len=%d lim=%d\n", len, lim));
	s = lalloc(len+1);
	DB(printf("-- copy s=%p\n", s));
	if(s == NULL)
		return NULL;
	DB(printf("-- copy %d bytes from \"%s\" to s (%p)\n", len, str, s));
	strncpy(s, str, len);
	s[len] = '\0';
	DB(printf("-- copy return \"%s\" (%p)\n", s, s));
	return s;
}

typedef struct
{
	char type; /* v f n + - * / % ^ ( ) , or null char ('\0') */
	char *str; /* actual token string, lalloc()'d */
	double value; /* value of token, if 'v' or 'i' */
	int args; /* number of arguments to function, if 'f' */
	FunctionPtr fn; /* function pointer, if 'f' */
	void *data; /* custom data block for function, if 'f' */
	char buf[2]; /* buffer for short token strings */
} Token;

Token G_pb_token = {'\0', NULL, 0.0, 0, NULL, NULL, {'\0','\0'}}; /* push back token */

#define EVAL_SYNTAX_ERROR 1
#define EVAL_DIVIDE_BY_ZERO 2
#define EVAL_UNKNOWN_NAME 3
#define EVAL_BAD_LITERAL 4
#define EVAL_MEM_ERROR 5
#define EVAL_CONVERT_ERROR 6
#define EVAL_NESTED_PARENS 7
#define EVAL_NULL_EXPRESSION 8
#define EVAL_FUNCTION_ERROR 9
#define EVAL_ARGS_ERROR 10
static int G_eval_error = 0;

#define MIN_ERR_VALUE 0
#define MAX_ERR_VALUE 10
static char *G_eval_err_str[11] = {
	"No Error", "Syntax Error", "Divide By Zero", "Unkown Name",
	"Bad Literal Value", "Error Allocating Memory", "Integer Convert Error",
	"Missing Close Parentheses", "NULL Expression String",
	"Error in Function Evaluation", "Invalid Argument Count"
};

/* pull the next token from the buffer, starting at the indicated position
** and update the position afterwards, returning a token structure
**
** the types of tokens that can be returned are:
**
**    'v' = variable name
**    'f' = function name
**    'n' = numeric value
**    '+' = plus sign (add or positive)
**    '-' = minus sign (subtract or negative)
**    '*' = asterisk (multiply)
**    '/' = slash (divide)
**    '\' = percent sign (modulo divide)
**    '^' = carat (exponent)
**    '%' = precent sign (percentages)
**    '(' = open parenthesis (start grouping or function call)
**    ')' = close parenthesis (end grouping or function call)
**    ',' = comma (argument delimiter)
*/
static Token pull_token(char *buf, int *pos)
{
	char tbuf[101];
	Token tok;
	int i, j;
	
	tok.type = '\0';
	tok.str = tok.buf;
	tok.value = 0.0;
	tok.args = 0;
	tok.fn = NULL;
	tok.data = NULL;
	tok.buf[0] = '\0';
	tok.buf[1] = '\0';
	
	if(buf == NULL)
		return tok;
	
	if(G_pb_token.type)
	{
		tok = G_pb_token;
		G_pb_token.type = '\0';
		G_pb_token.str = NULL;
		G_pb_token.value = 0.0;
		G_pb_token.args = 0;
		G_pb_token.fn = NULL;
		G_pb_token.data = NULL;
		G_pb_token.buf[0] = '\0';
		G_pb_token.buf[1] = '\0';
		return tok;
	}
	
	if(pos != NULL)
		i = (*pos);
	else
		i = 0;
	while(isspace(buf[i])) i++;
	switch(buf[i])
	{
	case '+':
		tok.type = '+';
		tok.buf[0] = '+';
		i++;
		break;
	case '-':
		tok.type = '-';
		tok.buf[0] = '-';
		i++;
		break;
	case '*':
		tok.type = '*';
		tok.buf[0] = '*';
		i++;
		break;
	case '/':
		tok.type = '/';
		tok.buf[0] = '/';
		i++;
		break;
	case '\\':
		tok.type = '\\';
		tok.buf[0] = '\\';
		i++;
		break;
	case '%':
		tok.type = '%';
		tok.buf[0] = '%';
		i++;
		break;
	case '^':
		tok.type = '^';
		tok.buf[0] = '^';
		i++;
		break;
	case '(':
		tok.type = '(';
		tok.buf[0] = '(';
		i++;
		break;
	case ')':
		tok.type = ')';
		tok.buf[0] = ')';
		i++;
		break;
	case ',':
		tok.type = ',';
		tok.buf[0] = ',';
		i++;
		break;
	default: /* numeric literal, variable name or invalid stuff */
		if(isdigit(buf[i]) || buf[i] == '.') /* numeric literal */
		{
			/* convert simple decimal number to fp value */
			j = 0;
			while(isdigit(buf[i]))
			{
				if(j < 100) tbuf[j++] = buf[i];
				i++;
			}
			if(buf[i] == '.')
			{
				if(j < 100) tbuf[j++] = '.';
				i++;
				while(isdigit(buf[i]))
				{
					if(j < 100) tbuf[j++] = buf[i];
					i++;
				}
			}
			tbuf[j] = '\0';
			if(j == 100)
				G_eval_error = EVAL_BAD_LITERAL;
			else
			{
				if(tok.str == NULL)
					G_eval_error = EVAL_MEM_ERROR;
				else
				{
					tok.str = copy_str(tbuf, 0);
					tok.value = strtod(tbuf, NULL);
					tok.type = 'n';
				}
			}
		}else if(isalpha(buf[i]) || buf[i] == '_') /* variable name */
		{
			VarFn *vf;
			
			/* get variable name and lookup value in var table */
			j = 0;
			while(isalpha(buf[i]) || isdigit(buf[i]) || buf[i] == '_')
			{
				if(j < 100) tbuf[j++] = buf[i];
				i++;
			}
			tbuf[j] = '\0';
			/* lookup variable name in var/fn table */
			if(ht_lookup(G_varfn_table, tbuf, (void*)(&vf)))
				G_eval_error = EVAL_UNKNOWN_NAME;
			else
			{
				tok.str = copy_str(tbuf, 0);
				tok.args = vf->nargs;
				tok.fn = vf->fn;
				tok.data = vf->data;
				tok.value = vf->value;
				if(vf->fn == NULL)
					tok.type = 'v'; /* name is a variable */
				else
					tok.type = 'f'; /* name is a function */
			}
		}else if(buf[i] == '\0')
		{
			DB(printf("-- token end of buffer\n"));
			tok.type = '\0';
			tok.str = tok.buf;
			tok.buf[0] = '\0';
			tok.value = 0.0;
		}else /* invalid stuff */
		{
			DB(printf("-- token invalid char '%c'\n", buf[i]));
			G_eval_error = EVAL_SYNTAX_ERROR;
		}
	}
	if(pos != NULL)
		*pos = i;
	
	return tok;
}

/* push the token back into the token stream */
static int push_token(Token tok)
{
	if(G_pb_token.type)
		return 1; /* push back token is already full */
	G_pb_token = tok;
	return 0;
}

static double eval_expr(char *buf, int *pos); /* expr = term+expr | term-expr | term */
static double eval_term(char *buf, int *pos); /* term = fact*term | fact/term | fact%term | fact */
static double eval_fact(char *buf, int *pos); /* fact = item^fact | item */
static double eval_item(char *buf, int *pos); /* item = -item | +item | int | var | fn(args) | (expr) */
static int eval_args(char *buf, int *pos, int *args, double **arg,
	int *arglen, int argpos); /* args = expr | expr,args */

static double eval_expr(char *buf, int *pos) /* expr = term+expr | term-expr | term */
{
	double rv = 0.0, lhs, rhs;
	Token tok;
	
	DB(printf("-- eval_expr(\"%s\", &pos=%p pos=%d)\n", buf+*pos, pos, *pos));
	lhs = eval_term(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- expr lhs=%f\n", lhs));
	tok = pull_token(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- expr token type '%c' = ", tok.type));
	switch(tok.type)
	{
	case '\0':
		DB(printf("END OF BUFFER\n"));
		rv = lhs;
		break;
	case '+': /* addition */
		rhs = eval_expr(buf, pos);
		DB(printf("%f+%f", lhs, rhs));
		rv = lhs+rhs;
		DB(printf("=%f\n", rv));
		break;
	case '-': /* subtraction */
		rhs = eval_expr(buf, pos);
		DB(printf("%f-%f", lhs, rhs));
		rv = lhs-rhs;
		DB(printf("=%f\n", rv));
		break;
	case ')': /* end of group */
		DB(printf("end group"));
		push_token(tok);
		rv = lhs;
		DB(printf(" (rv=%f)\n", rv));
		break;
	case ',': /* argument delimiter */
		DB(printf("delimiter"));
		push_token(tok);
		rv = lhs;
		DB(printf(" (rv=%f)\n", rv));
		break;
	default:
		DB(printf("invalid expr token\n"));
		G_eval_error = EVAL_SYNTAX_ERROR;
		rv = lhs;
	}
	
	DB(printf("-- expr rv=%f\n", rv));
	return rv;
}

static double eval_term(char *buf, int *pos) /* term = fact*term | fact/term | fact */
{
	double rv = 0.0, lhs, rhs;
	Token tok;
	
	DB(printf("-- eval_term(\"%s\", &pos=%p pos=%d)\n", buf+*pos, pos, *pos));
	lhs = eval_fact(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- term lhs=%f\n", lhs));
	tok = pull_token(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- term token type '%c' = ", tok.type));
	switch(tok.type)
	{
	case '\0':
		DB(printf("END OF BUFFER\n"));
		rv = lhs;
		break;
	case '*': /* multiplication */
		rhs = eval_term(buf, pos);
		DB(printf("%f*%f", lhs, rhs));
		rv = lhs*rhs;
		DB(printf("=%f\n", rv));
		break;
	case '/': /* division */
		rhs = eval_term(buf, pos);
		DB(printf("%f/%f", lhs, rhs));
		if(rhs == 0.0)
		{
			DB(printf(" DIVISION BY ZERO\n"));
			G_eval_error = EVAL_DIVIDE_BY_ZERO;
		}else
		{
			rv = lhs/rhs;
			DB(printf("=%f\n", rv));
		}
		break;
	case '\\': /* modulo division */
		rhs = eval_term(buf, pos);
		DB(printf("%f%%%f", lhs, rhs));
		if(rhs == 0.0)
		{
			DB(printf(" DIVISION BY ZERO\n"));
			G_eval_error = EVAL_DIVIDE_BY_ZERO;
		}else
		{
			rv = fmod(lhs, rhs);
			DB(printf("=%f\n", rv));
		}
		break;
	default:
		DB(printf("PUSHBACK\n"));
		push_token(tok);
		rv = lhs;
	}
	
	DB(printf("-- term rv=%f\n", rv));
	return rv;
}

static double eval_fact(char *buf, int *pos) /* fact = item^fact | item */
{
	double rv = 0.0, lhs, rhs;
	Token tok;
	
	DB(printf("-- eval_fact(\"%s\", &pos=%p pos=%d)\n", buf+*pos, pos, *pos));
	lhs = eval_item(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- fact lhs=%f\n", lhs));
	tok = pull_token(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- fact token type '%c' = ", tok.type));
	switch(tok.type)
	{
	case '\0':
		DB(printf("END OF BUFFER\n"));
		rv = lhs;
		break;
	case '^': /* exponentiation */
		rhs = eval_fact(buf, pos);
		DB(printf("%f^%f", lhs, rhs));
		if(G_eval_error)
			return rv;
		rv = pow(lhs, rhs);
		DB(printf("=%f\n", rv));
		break;
	default:
		DB(printf("PUSHBACK\n"));
		push_token(tok);
		rv = lhs;
	}
	
	DB(printf("-- fact rv=%f\n", rv));
	return rv;
}

static double eval_item(char *buf, int *pos) /* item = -item | +item | int | var | (expr) */
{
	double rv = 0.0, lhs, rhs;
	void *data;
	int tlen, nargs, xargs, i;
	FunctionPtr fn;
	char *fname;
	Token tok;
	
	DB(printf("-- eval_item(\"%s\", &pos=%p pos=%d)\n", buf+*pos, pos, *pos));
	tok = pull_token(buf, pos);
	if(G_eval_error)
		return rv;
	DB(printf("-- item token type '%c' = ", tok.type));
	switch(tok.type)
	{
	case '+': /* positive */
		DB(printf("positive\n"));
		rv = eval_fact(buf, pos);
		break;
	case '-': /* negative */
		DB(printf("negative\n"));
		rv = -eval_fact(buf, pos);
		break;
	case 'v': /* variable */
		DB(printf("variable name '%s'=%f\n", tok.str, tok.value));
		rv = tok.value;
		break;
	case 'f': /* function */
		DB(printf("function name '%s'=%p(%d)\n", tok.str, tok.fn, tok.args));
		tlen = tok.args;
		nargs = tok.args;
		xargs = nargs;
		data = tok.data;
		fn = tok.fn;
		fname = tok.str;
		tok = pull_token(buf, pos);
		if(G_eval_error == 0)
		{
			if(tok.type != '(')
				G_eval_error = EVAL_SYNTAX_ERROR;
			else
			{
				double *targ = NULL;
				
				if(tlen > 0)
				{
					targ = (double*)lalloc(sizeof(double)*tlen);
					if(targ == NULL)
					{
						G_eval_error = EVAL_MEM_ERROR;
						break;
					}
				}else
					tlen = 0;
				if(eval_args(buf, pos, &nargs, &targ, &tlen, 0))
					break;
				DB(printf("-- item %d arguments\n", nargs));
				if(xargs < 0)
				{
					if(nargs < 1)
					{
						DB(printf("-- item too few arguments\n"));
						G_eval_error = EVAL_ARGS_ERROR;
						break;
					}
				}else if(nargs != xargs)
				{
					DB(printf("-- item bad argument count (%d) need %d\n",
						nargs, xargs));
					G_eval_error = EVAL_ARGS_ERROR;
					break;
				}
				tok = pull_token(buf, pos);
				if(tok.type != ')')
					G_eval_error = EVAL_SYNTAX_ERROR;
				else if(G_eval_error == 0)
				{
					DB(printf("-- call function [%p] with %d args [%p]\n",
						fn, nargs, targ));
					DB(printf("-- %s(%f", fname, targ[0]));
					DB(for(i=1;i<nargs;i++)printf(",%f",targ[i]));
					DB(printf(")\n"));
					if(fn(nargs, targ, &rv, data) != 0)
						G_eval_error = EVAL_FUNCTION_ERROR;
					DB(printf("-- rv = %f\n", rv));
				}
			}
		}
		break;
	case 'n': /* number */
		DB(printf("number value '%s'=%f\n", tok.str, tok.value));
		rv = tok.value;
		break;
	case '(':
		DB(printf("start grouping\n"));
		rv = eval_expr(buf, pos);
		tok = pull_token(buf, pos);
		if(tok.type != ')')
			G_eval_error = EVAL_SYNTAX_ERROR;
		break;
	default:
		DB(printf("PUSHBACK\n"));
		push_token(tok);
	}
	
	tok = pull_token(buf, pos);
	while(tok.type == '%')
	{
		rv = rv/100.0;
		tok = pull_token(buf, pos);
	}
	if(tok.type != '\0')
		push_token(tok);
	
	DB(printf("-- item rv=%f\n", rv));
	return rv;
}

static int eval_args(char *buf, int *pos, int *args, double **arg,
	int *arglen, int argpos) /* args = expr,args | expr | */
{
	double *tmp;
	Token tok;
	int i;
	
	DB(printf("-- eval_args(\"%s\", &pos=%p pos=%d, args=%d ,arg=%p, alen=%d apos=%d)\n",
		buf+*pos, pos, *pos, args, arg, *arglen, argpos));
	tok = pull_token(buf, pos);
	push_token(tok);
	if(tok.type == ')')
	{
		push_token(tok);
		return 0; /* allow empty argument lists */
	}
	*args = argpos+1;
	
	if(argpos >= *arglen) /* if arg array is full, grow it */
	{
		DB(printf("-- realloc arglist (%d elements)\n", (*arglen)+8));
		tmp = (double*)lalloc(sizeof(double)*((*arglen)+8));
		if(tmp == NULL)
		{
			G_eval_error = EVAL_MEM_ERROR;
			DB(printf("-- args memory alloc error\n"));
			return 1;
		}
		DB(if(*arglen>0)printf("-- copy %d arguments to new list\n", *arglen));
		for(i = 0; i < *arglen; i++)
		{
			DB(printf("-- %d: %f -> ", i+1, (*arg)[i]));
			tmp[i] = (*arg)[i]; /* copy arg array to tmp array */
			DB(printf("%f\n", tmp[i]));
		}
		*arg = tmp; /* replace arg with tmp (memory will get auto-freed later */
		*arglen = (*arglen)+8; /* update arglen to length of tmp */
	}
	(*arg)[argpos] = eval_expr(buf, pos);
	DB(printf("-- arg #%d = %f\n", argpos+1, (*arg)[argpos]));
	if(G_eval_error)
	{
		DB(printf("-- args eval_expr error\n"));
		return 2;
	}
	tok = pull_token(buf, pos);
	DB(printf("-- args token '%c' = ", tok.type));
	switch(tok.type)
	{
	case ',':
		DB(printf("comma\n"));
		if(eval_args(buf, pos, args, arg, arglen, argpos+1))
			return 3;
		break;
	case ')':
		DB(printf("end args (pushback)\n"));
		push_token(tok);
		break;
	default:
		DB(printf("PUSHBACK/SYNTAX ERROR\n"));
		G_eval_error = EVAL_SYNTAX_ERROR;
		return 4;
	}
	
	DB(printf("-- args return NO ERROR\n"));
	return 0;
}

/* public: expression evaluation function */
int eval(char *expr, double *result)
{
	double rv = 0.0;
	int pos = 0;
	static int recurse = 0;
	
	recurse++;
	set_funcs();
	
	if(expr == NULL)
		return EVAL_NULL_EXPRESSION;
	G_eval_error = 0;
	G_pb_token.type = '\0';
	G_pb_token.str = NULL;
	G_pb_token.value = 0.0;
	G_pb_token.args = 0;
	G_pb_token.fn = NULL;
	G_pb_token.data = NULL;
	G_pb_token.buf[0] = '\0';
	G_pb_token.buf[1] = '\0';
	rv = eval_expr(expr, &pos);
	recurse--;
	if(recurse == 0)
		lfreeall();
	if(G_eval_error)
		return G_eval_error;
	if(result != NULL)
		*result = rv;
	return 0;
}

/* return information about the expression evaluator, copyright, auther, etc. */
void eval_info(int *version, int *revision, int *buildno,
	char *authbuf, int authlim, char *copybuf, int copylim,
	char *licebuf, int licelim)
{
	if(version)
		*version = G_version;
	if(revision)
		*revision = G_revision;
	if(buildno)
		*buildno = G_buildno;
	if(authbuf)
		strncpy(authbuf, G_author+16, authlim);
	if(copybuf)
		strncpy(copybuf, G_copyright+8, copylim);
	if(licebuf)
		strncpy(licebuf, G_license+17, licelim);
	return;
}

/* public: return a string describing the specified error code */
const char *eval_error(int err)
{
	if(err < MIN_ERR_VALUE || err > MAX_ERR_VALUE)
		return "Unknown Error Value";
	return G_eval_err_str[err];
}

#ifdef EVAL_TEST

#include <stdio.h>

static int iter(unsigned long slot, void *key, void *val)
{
	VarFn *vf;
	
	vf = (VarFn*)val;
	
	if(vf == NULL)
		printf("\tNULL pointer!\n");
	else if(vf->fn == NULL)
		printf("\t%s = %f\n", vf->name, vf->value);
	
	return 0;
}

static int cust_func(int args, const double *arg, double *rv, void *data)
{
	if(eval((char*)data, rv))
		return 1;
	
	return 0;
}

int main(int args, char *arg[])
{
	char buf[1001], *p;
	int err, i;
	double rv;
	
	do
	{
		printf("\n> ");
		fflush(stdout);
		buf[0] = '\0';
		fgets(buf, 1000, stdin);
		if(feof(stdin) || ferror(stdin))
			break;
		if(buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		if(strcasecmp(buf, "QUIT") == 0
		|| strcasecmp(buf, "EXIT") == 0
		|| strcasecmp(buf, "DONE") == 0)
			break;
		if(strcasecmp(buf, "HELP") == 0)
		{ /* show help text */
			printf("\texpr            eval expr and print result\n");
			printf("\tname=expr       eval expr and assign to named var\n");
			printf("\tname?           print value of named var\n");
			printf("\t?name           same as 'name?'\n");
			printf("\t?               list all named vars and their values\n");
			printf("\tQUIT/EXIT/DONE  end the program\n");
			printf("\n\toperators: + - * / % ^ ()\n");
		}else if(p = strchr(buf, '='))
		{ /* assign a value to a variable */
			char *name, *expr;
			
			name = buf;
			*p = '\0';
			expr = p+1;
			err = eval(expr, &rv);
			if(err)
				printf("eval error #%d: %s\n", err, eval_error(err));
			else
			{
				printf("%s = %f", name, rv);
				if(eval_set_var(name, rv))
					printf(" - failed to set variable");
				printf("\n");
			}
		}else if(p = strchr(buf, '?'))
		{ /* print variable values */
			char *name;
			
			if(buf[0] == '?')
				name = buf+1;
			else
				name = buf;
			*p = '\0';
			if(G_var_count == 0)
				printf("no variables defined\n");
			else
			{
				if(name[0] == '\0')
				{ /* print all variables */
					if(ht_iterate(G_varfn_table, iter, NULL))
						printf("error while iterating over var table\n");
				}else
				{ /* print named variable */
					printf("%s = ", name);
					if(eval_get_var(name, &rv))
						printf(" - failed to get variable '%s'", name);
					else
						printf("%f", rv);
					printf("\n");
				}
			}
		}else
		{ /* evaluate an expression and print result */
			err = eval(buf, &rv);
			if(err)
				printf("eval error #%d: %s\n", err, eval_error(err));
			else
				printf("= %f\n", rv);
		}
	}while(!feof(stdin) && !ferror(stdin));
	
	printf("\nprogram done.\n");
	
	return 0;
}

#endif
