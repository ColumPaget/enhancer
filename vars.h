
#ifndef ENHANCER_VARS_H
#define ENHANCER_VARS_H

#include "common.h"


typedef struct
{
char *name;
char *value;
} TVar;

typedef struct
{
int max;
TVar *vars;
} TVarList;

TVarList *enhancer_varlist_create();
void enhancer_setvarlist(TVarList *list, const char *name, const char *value);
const char *enhancer_getvarlist(TVarList *list, const char *name);
void enhancer_setvar(const char *name, const char *value);
const char *enhancer_getvar(const char *name);
void enhancer_func_setvar(const char *Arg, const char *FuncName, const char *Str1, const char *Str2);


#endif
