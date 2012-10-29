module eval;

extern(C):

int eval_set_var(in char* name, double value);
int eval_get_var(in char* name, double *value);

int eval_def_fn(in char* name, int function(int args, double* argv, double* rv, void* data) fn, void* data, int args);

int eval(in char* expr, double *result);
alias eval eval_exr;

void eval_info(int* ver, int* revision, int* buildno,
	char* authbuf, int authlim, char* copybuf, int copylim,
	char* licebuf, int licelim);

version(D_Version2)
	mixin("const(char)* eval_error(int err);");
else
	char* eval_error(int err);

int eval_set_default_env(); 
