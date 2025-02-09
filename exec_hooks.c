#include "common.h"
#include "actions.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>

int (*enhancer_real_execve)(const char *path, char *const args[], char *const env[])=NULL;
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


//pack a list of varadic args into an array
int enhancer_exec_varg_pack(char ***packed, const char *first_arg, va_list args)
{
    const char *arg;
    int i=1;


			//first arg is a special case, as it's not considered part
			//of the varadic list, as it's the last 'normal' arg that
			//must be supplied to mark the start of the varadic list
			//thus we must add it outside of the loop
     (*packed)=realloc((*packed), sizeof(char *) * (i+10));
     (*packed)[i]=strdup(first_arg);
     i++;

    arg=va_arg(args, const char *);
    while (arg != NULL)
    {
        (*packed)=realloc((*packed), sizeof(char *) * (i+10));
        (*packed)[i]=strdup(arg);
        i++;
        arg=va_arg(args, const char *);
    }
    (*packed)[i]=NULL;

    return(i);
}


//destro a null-terminated array
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

    printf("system: [%s] [%s] %d\n", command, p_command, Flags);
    fflush(NULL);
    if (! (Flags & FLAG_DENY)) result=enhancer_real_system(p_command);

    destroy(Sanitised);

    if ((result < 1 ) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("system");

    return(result);
}


int execve(const char *command, char *const args[], char *const env[])
{
    int Flags, result=-1, i;
    char *Sanitised=NULL;
    const char *ptr;
    char *const *p_args;
    char **sani_args=NULL;


    Flags=enhancer_checkconfig_default(FUNC_EXEC, "exec", command, "", 0, 0);
    Flags |= enhancer_checkconfig_default(FUNC_SYSTEM, "sysrun", command, "", 0, 0);

    p_args=args;
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

		//we can only get here fi the execve failed
    if ((result < 1 ) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("execve");
    enhancer_exec_pack_destroy(sani_args);

    return(result);
}




int execl(const char *path, const char *first_arg, ...)
{
    va_list args;
    char **packed;
    int result;

		packed=calloc(10, sizeof(char **));
		packed[0]=strdup(path);
    va_start(args, first_arg);
    enhancer_exec_varg_pack(&packed, first_arg, args);
    va_end(args);

		//args will get sanitized in here
    result=execve(path, packed, environ);
			
		//we can only get here if the execve failed
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
