#include "common.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include "hooks.h"


void *(*enhancer_real_dlopen)(const char *filename, int flag)=NULL;
int (*enhancer_real_dlclose)(void *handle)=NULL;

void *dlopen(const char *path, int flags)
{
char *Redirect=NULL;
int Flags;
void *handle;

if (! enhancer_real_dlclose) enhancer_get_real_functions();

Flags=enhancer_checkconfig_with_redirect(FUNC_DLOPEN, "dlopen", path, "", flags, 0, &Redirect);

if (Flags & FLAG_DENY) 
{
	destroy(Redirect);
	return(NULL);
}

handle=enhancer_real_dlopen(Redirect, flags);
destroy(Redirect);

return(handle);
}




int dlclose(void *handle)
{
int Flags;

if (! enhancer_real_dlclose) enhancer_get_real_functions();

Flags=enhancer_checkconfig_default(FUNC_DLCLOSE, "dlclose", "", "", 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

return(enhancer_real_dlclose(handle));
}




void enhancer_dl_hooks()
{
if (! enhancer_real_dlopen) enhancer_real_dlopen = dlsym(RTLD_NEXT, "dlopen");
if (! enhancer_real_dlclose) enhancer_real_dlclose = dlsym(RTLD_NEXT, "dlclose");
}
