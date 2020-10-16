#include "common.h"
#include "actions.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>

int (*enhancer_real_execve)(const char *path, const char *args[], const char *env[])=NULL;
int (*enhancer_real_execl)(const char *path, const char *arg, ...)=NULL;
int (*enhancer_real_system)(const char *command)=NULL;


#define SANITISE_FLAGS (FLAG_SANITISE | FLAG_DIE_ON_TAINT | FLAG_DENY_ON_TAINT)

static int enhancer_sanitise_exec(int Flags, const char *command, char **Sanitised)
{
const char *ptr;
const char *BadChars="`$&;|";
int len=0;;

*Sanitised=realloc(*Sanitised, strlen(command) +10);

for (ptr=command; *ptr !='\0'; ptr++)
{
	if (! strchr(BadChars, *ptr)) 
	{
		(*Sanitised)[len]=*ptr;
		len++;
	}
	else 
	{
	if (Flags & FLAG_DIE_ON_TAINT)
	{
		syslog(LOG_WARNING, "tainted exec: %s", command);
		_exit(1);
	}
	if (Flags & FLAG_DENY_ON_TAINT) Flags |= FLAG_DENY;
	}
}

(*Sanitised)[len]='\0';

return(Flags);
}


static char **enhancer_exec_varg_pack(const char *first_arg, va_list args)
{
const char *arg;
char **packed=NULL;
int i=1;

packed=realloc(packed, sizeof(char *) * (i+1));
packed[i]=strdup(first_arg);

arg=va_arg(args, const char *);
while (arg != NULL)
{
packed=realloc(packed, sizeof(char *) * (i+1));
packed[i]=strdup(arg);
i++;
arg=va_arg(args, const char *);
}
packed[i]=NULL;

return(packed);
}


static void enhancer_exec_pack_destroy(char **pack)
{
int i;

for (i=0; pack[i] != NULL; i++)
{
	free(pack[i]);
}
free(pack);
}


int system(const char *command)
{
int Flags, result=-1;
char *Sanitised=NULL;
const char *p_command;

Flags=enhancer_checkconfig_default(FUNC_SYSTEM, "system", command, "", 0, 0);
Flags |= enhancer_checkconfig_default(FUNC_SYSTEM, "sysrun", command, "", 0, 0);

p_command=command;
if (Flags & SANITISE_FLAGS) 
{
	Flags=enhancer_sanitise_exec(Flags, command, &Sanitised);
	p_command=Sanitised;
}

printf("system: [%s] [%s] %d\n", command, p_command, Flags); fflush(NULL);
if (! (Flags & FLAG_DENY)) result=enhancer_real_system(p_command);

destroy(Sanitised);

if ((result < 1 ) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("system");

return(result);
}


int execve(const char *command, char *const args[], char *const env[])
{
int Flags, result=-1, i;
char *Sanitised=NULL;
const char **p_args, *ptr;
char **sani_args=NULL;


Flags=enhancer_checkconfig_default(FUNC_EXEC, "exec", command, "", 0, 0);
Flags |= enhancer_checkconfig_default(FUNC_SYSTEM, "sysrun", command, "", 0, 0);

p_args=(const char **) args;
if (Flags & SANITISE_FLAGS) 
{
	for (i=0; args[i] != NULL; i++)
	{
			sani_args=realloc(sani_args, sizeof(char *) * (i+1));
			sani_args[i]=NULL;
			Flags |= enhancer_sanitise_exec(Flags, args[i], &(sani_args[i]));
	}
	sani_args[i]=NULL;
	p_args=sani_args;
}


if (Flags & FLAG_DENY) return(-1);

result=enhancer_real_execve(command, p_args, env);

if ((result < 1 ) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("execve");

return(result);
}




int execl(const char *path, const char *arg, ...)
{
va_list args;
char **packed;
int result;

va_start(args, arg);
packed=enhancer_exec_varg_pack(arg, args);
va_end(args);

result=execve(path, packed, environ);
enhancer_exec_pack_destroy(packed);

return(result);
}


/*
int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg,
..., char * const envp[]);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]); 
*/

void enhancer_exec_hooks()
{
if (! enhancer_real_system) enhancer_real_system = dlsym(RTLD_NEXT,"system");
if (! enhancer_real_execve) enhancer_real_execve = dlsym(RTLD_NEXT,"execve");
if (! enhancer_real_execl) enhancer_real_execl = dlsym(RTLD_NEXT,"execl");
}
