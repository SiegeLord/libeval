#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include "eval.h"
#include "func.h"

/* compare two doubles, return -1 if v1 < v2, 1 if v1 > v2, 0 if v1 = v2 */
static int dcomp(const void *v1, const void *v2)
{
	double d1, d2;
	
	d1 = *(double*)v1;
	d2 = *(double*)v2;
	
	if(d1 < 0.0)
		d1 = -d1; /* absolute value of d1 */
	if(d2 < 0.0)
		d2 = -d2; /* absolute value of d2 */
	
	if(d1 < d2)
		return -1;
	if(d2 > d2)
		return 1;
	return 0;
}

/* compare two doubles, inverted, return -1 if v1 > v2, 1 if v1 < v2 */
static int dcompi(const void *v1, const void *v2)
{
	return -dcomp(v1, v2);
}

static FUNCTION(func_abs,args,arg,rv,data)
{
	*rv = fabs(arg[0]);
	return 0;
}

static FUNCTION(func_int,args,arg,rv,data)
{
	*rv = (long)arg[0];
	return 0;
}

/* 24 Dec. 2006
** for some stupid reason the round() and trunc() functions don't work on
** Linux (you get REALLY crazy results, NaN and wildly large integer values.
** I've tried the same thing on Mac OS X and it works just fine.)
**
** I've reimplemented round() and trunc() using floor() and ceil(), which
** seems to work just fine (although I won't vouch for the statistical
** accuracy of the rounding method). Of course, this is ONLY necessary on
** a system with a broken C stanard library (such as, aparantly, Linux).
*/

static FUNCTION(func_round,args,arg,rv,data)
{
	double x;
	
	x = arg[0];
	if(x <= -0.5)
		*rv = ceil(x-0.5);
	else if(x >= 0.5)
		*rv = floor(x+0.5);
	else
		*rv = 0.0;
	/* *rv = round(arg[0]); */
	return 0;
}

static FUNCTION(func_trunc,args,arg,rv,data)
{
	double x;
	
	x = arg[0];
	if(x <= 0.0)
		*rv = ceil(x);
	else
		*rv = floor(x);
	/* *rv = trunc(arg[0]); */
	return 0;
}

static FUNCTION(func_floor,args,arg,rv,data)
{
	*rv = floor(arg[0]);
	return 0;
}

static FUNCTION(func_ceil,args,arg,rv,data)
{
	*rv = ceil(arg[0]);
	return 0;
}


static FUNCTION(func_sin,args,arg,rv,data)
{
	*rv = sin(arg[0]);
	return 0;
}

static FUNCTION(func_cos,args,arg,rv,data)
{
	*rv = cos(arg[0]);
	return 0;
}

static FUNCTION(func_tan,args,arg,rv,data)
{
	*rv = tan(arg[0]);
	return 0;
}


static FUNCTION(func_asin,args,arg,rv,data)
{
	if(arg[0] < -1.0 || arg[0] > 1.0)
		return 1;
	*rv = asin(arg[0]);
	return 0;
}

static FUNCTION(func_acos,args,arg,rv,data)
{
	if(arg[0] < -1.0 || arg[0] > 1.0)
		return 1;
	*rv = acos(arg[0]);
	return 0;
}

static FUNCTION(func_atan,args,arg,rv,data)
{
	*rv = atan(arg[0]);
	return 0;
}


static FUNCTION(func_sinh,args,arg,rv,data)
{
	*rv = sinh(arg[0]);
	return 0;
}

static FUNCTION(func_cosh,args,arg,rv,data)
{
	*rv = cosh(arg[0]);
	return 0;
}

static FUNCTION(func_tanh,args,arg,rv,data)
{
	*rv = tanh(arg[0]);
	return 0;
}

static FUNCTION(func_asinh,args,arg,rv,data)
{
	*rv = asinh(arg[0]);
	return 0;
}

static FUNCTION(func_acosh,args,arg,rv,data)
{
	*rv = acosh(arg[0]);
	return 0;
}

static FUNCTION(func_atanh,args,arg,rv,data)
{
	*rv = atanh(arg[0]);
	return 0;
}


static FUNCTION(func_ln,args,arg,rv,data)
{
	*rv = log(arg[0]);
	return 0;
}

static FUNCTION(func_exp,args,arg,rv,data)
{
	*rv = exp(arg[0]);
	return 0;
}

static FUNCTION(func_log,args,arg,rv,data)
{
	*rv = log10(arg[0]);
	return 0;
}

static FUNCTION(func_sqrt,args,arg,rv,data)
{
	*rv = sqrt(arg[0]);
	return 0;
}


static FUNCTION(func_rand,args,arg,rv,data)
{
	*rv = (double)rand()/(double)RAND_MAX;
	
	return 0;
}

static FUNCTION(func_sum,args,arg,rv,data)
{
	int i;
	double sum = 0.0;
	
	qsort(arg, args, sizeof(double), dcompi);
	for(i = 0; i < args; i++)
		sum += arg[i];
	*rv = sum;
	
	return 0;
}

static FUNCTION(func_min,args,arg,rv,data)
{
	int i;
	double min;
	
	min = arg[0];
	for(i = 1; i < args; i++)
		if(arg[i] < min)
			min = arg[i];
	*rv = min;
	
	return 0;
}

static FUNCTION(func_max,args,arg,rv,data)
{
	int i;
	double max;
	
	max = arg[0];
	for(i = 1; i < args; i++)
		if(arg[i] > max)
			max = arg[i];
	*rv = max;
	
	return 0;
}

static FUNCTION(func_avg,args,arg,rv,data)
{
	int n;
	double sum = 0.0;
	
	qsort(arg, args, sizeof(double), dcompi);
	for(n = 0; n < args; n++)
		sum += arg[n];
	*rv = sum/(double)n;
	
	return 0;
}

/* return the median value */
static FUNCTION(func_med,args,arg,rv,data)
{
	qsort(arg, args, sizeof(double), dcomp);
	if(args%2 == 0) /* even number, return average of middle values */
		*rv = (arg[args/2-1]+arg[args/2])/2.0;
	else /* odd number of elements, return middle value */
		*rv = arg[args/2];
	
	return 0;
}

/* return the variance */
static FUNCTION(func_var,args,arg,rv,data)
{
	int n;
	double sum = 0.0, sumsq = 0.0, avg = 0.0;
	
	qsort(arg, args, sizeof(double), dcompi);
	for(n = 0; n < args; n++)
		sum += arg[n];
	avg = sum/(double)n;
	for(n = 0; n < args; n++)
		sumsq += (arg[n]-avg)*(arg[n]-avg);
	
	if(n > 1)
		*rv = sumsq/(double)(n-1);
	else
		*rv = 0.0;
	
	return 0;
}

/* return the standard deviation */
static FUNCTION(func_std,args,arg,rv,data)
{
	double var;
	
	func_var(args, arg, &var, data);
	*rv = sqrt(var);
	
	return 0;
}

#define PI  3.14159265358979323
#define DEGREES_PER_RADIAN (360.0/(2.0*PI))
/* convert radians to degrees */
static FUNCTION(func_deg,args,arg,rv,data)
{
	*rv = arg[0]*DEGREES_PER_RADIAN;
	return 0;
}

#define RADIANS_PER_DEGREE ((2.0*PI)/360.0)
/* convert degrees to radians */
static FUNCTION(func_rad,args,arg,rv,data)
{
	*rv = arg[0]*RADIANS_PER_DEGREE;
	return 0;
}

/* factorial approximation */
static FUNCTION(func_fact,args,arg,rv,data)
{
	double x, f = 1.0;
	int n;
	
	x = arg[0];
	if(x < 0.0)
		return 1;
	if(fabs(floor(x)-x) > DBL_EPSILON)
	{
		f = sqrt(x*6.283185308)*pow(x/2.71828183, x); /* stirling's approx. */
	}else
	{
		for(n = 1; n <= x; n++)
		{ /* simple iterative factorial, with early exit on overflow */
			if(isinf(f))
				break;
			f *= (double)n;
		}
	}
	*rv = f;
	
	return 0;
}

/* sign of x */
static FUNCTION(func_sign,args,arg,rv,data)
{
	if(arg[0] < 0.0)
		*rv = -1.0;
	else
		*rv = 1.0;
	return 0;
}

static char *fnname[] = {
	"abs", "int", "round", "trunc", "floor", "ceil",
	"sin", "cos", "tan", "asin", "acos", "atan",
	"sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
	"ln", "exp", "log", "sqrt", "rand", "sum",
	"min", "max", "avg", "med", "var", "std",
	"deg", "rad", "fact", "sign", NULL
};

static FUNCTION((*fn[]),args,arg,rv,data) =
{
	func_abs, func_int, func_round, func_trunc, func_floor, func_ceil,
	func_sin, func_cos, func_tan, func_asin, func_acos, func_atan,
	func_sinh, func_cosh, func_tanh, func_asinh, func_acosh, func_atanh,
	func_ln, func_exp, func_log, func_sqrt, func_rand, func_sum,
	func_min, func_max, func_avg, func_med, func_var, func_std,
	func_deg, func_rad, func_fact, func_sign, NULL
};

/* positive numbers (including zero) indicate fixed number of arguments,
** negative one (-1) indicates variable number of arguments */
static int fnargs[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	-1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 0
};

int set_funcs(void)
{
	int i;
	static int done = 0;
	
	if(done)
		return 0;
	done = 1;
	
	for(i = 0; fnname[i] != NULL; i++)
		if(eval_def_fn(fnname[i], fn[i], NULL, fnargs[i]))
			return 1;
	
	return 0;
}
