#include "vars.h"


TVarList *enhancer_varlist=NULL;

TVarList *enhancer_varlist_create()
{
TVarList *varl;

varl=(TVarList *) calloc(1, sizeof(TVarList));
varl->max=0;
return(varl);
}

void enhancer_setvarlist(TVarList *list, const char *name, const char *value)
{
char *ptr, *end;
TVar *Var;
int i;


if (name==NULL) return;
if (value==NULL) return;

for (i=0; i < list->max; i++)
{
	Var=&(list->vars[i]);
	if (Var && (strcmp(Var->name,name) ==0)) 
	{
		Var->value=enhancer_strcpy(Var->value, value);
		return;
	}

}


/*if we get to here, then we didn't find a variable in the list */
list->vars=realloc(list->vars, (list->max+10) * sizeof(TVar));
Var=&(list->vars[list->max]);
//these variables won't be allocated, nor set to NULL, so use enhancer_strcpy
//in light of that
Var->name=enhancer_strcpy(NULL, name);
Var->value=enhancer_strcpy(NULL, value);

list->max++;
}

void enhancer_setvar(const char *name, const char *value)
{
if (! enhancer_varlist) enhancer_varlist=enhancer_varlist_create();
enhancer_setvarlist(enhancer_varlist, name, value);
}

void enhancer_func_setvar(const char *Arg, const char *FuncName, const char *Str1, const char *Str2)
{
char *Name=NULL, *Value=NULL, *Tempstr=NULL;
const char *ptr;

ptr=enhancer_strtok(Arg, "=", &Tempstr);
Name=enhancer_format_str(Name, Tempstr, FuncName, Str1, Str2);
ptr=enhancer_strtok(ptr, "=", &Tempstr);
Value=enhancer_format_str(Value, Tempstr, FuncName, Str1, Str2);
enhancer_setvar(Name, Value);

destroy(Name);
destroy(Value);
destroy(Tempstr);
}


const char *enhancer_getvarlist(TVarList *list, const char *name)
{
TVar *Var;
int i;

for (i=0; i < list->max; i++)
{
	Var=&(list->vars[i]);
	if (Var && (strcmp(Var->name,name)==0) ) return(Var->value);
}

return("");
}

const char *enhancer_varlist_find_value(TVarList *list, const char *value)
{
TVar *Var;
int i;

for (i=0; i < list->max; i++)
{
	Var=&(list->vars[i]);
	if (Var && (strcmp(Var->value,value)==0) ) return(Var->name);
}

return("");
}



const char *enhancer_getvar(const char *name)
{
if (! enhancer_varlist) return("");
return(enhancer_getvarlist(enhancer_varlist, name));
}


