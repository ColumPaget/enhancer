#include "actions.h"
#include "config.h"
#include "net.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <poll.h>
#include "hooks.h"
#include "socket_hooks.h"
#include "exec_hooks.h"
#include "file_hooks.h"
#include "time_hooks.h"

#ifdef HAVE_X11
#include "x11_hooks.h"
#endif

int (* enhancer_real_uname)(struct utsname *UTS)=NULL;

int (*enhancer_real_setuid)(uid_t uid)=NULL; 
int (*enhancer_real_setreuid)(uid_t ruid, uid_t euid)=NULL; 
int (*enhancer_real_setresuid)(uid_t ruid, uid_t euid, uid_t suid)=NULL; 
int (*enhancer_real_setgid)(gid_t gid)=NULL; 
int (*enhancer_real_setregid)(gid_t rgid, gid_t egid)=NULL; 
int (*enhancer_real_setresgid)(gid_t rgid, gid_t egid, gid_t sgid)=NULL; 

pid_t (*enhancer_real_fork)()=NULL;
pid_t (*enhancer_real_vfork)()=NULL;
int (*enhancer_real_select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)=NULL;
int (*enhancer_real_poll)(struct pollfd *fds, nfds_t nfds, int timeout);

int (*enhancer_real_main)(int (*main) (int, char **, char **), int argc, char **ubp_av, void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (*stack_end))=NULL;



int uname(struct utsname *UTS)
{
int Flags, result;

if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();
Flags=enhancer_checkconfig_default(FUNC_UNAME, "uname", "", "", 0, 0);
if (Flags & FLAG_DENY) return(-1);

result=enhancer_real_uname(UTS);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("uname");
return(result);
}




int setuid (uid_t uid)
{
int result, Flags;

Flags=enhancer_checkconfig_xid_function(FUNC_SETUID, "setuid", uid, 0, 0);

if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setuid(uid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setuid");

return(result);
}





int setreuid (uid_t ruid, uid_t euid)
{
int result, Flags;

Flags=enhancer_checkconfig_xid_function(FUNC_SETUID, "setreuid", ruid, euid, 0);

if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setreuid(ruid, euid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setreuid");

return(result);
}



int setresuid (uid_t ruid, uid_t euid, uid_t suid)
{
int result, Flags;

Flags=enhancer_checkconfig_xid_function(FUNC_SETUID, "setresuid", ruid, euid, suid);

if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setresuid(ruid, euid, suid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setresuid");
return(result);
}



int setgid (gid_t gid)
{
int result, Flags;

Flags=enhancer_checkconfig_xid_function(FUNC_SETGID, "setgid", gid, 0, 0);

if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setgid(gid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setgid");
return(result);
}



int setregid (gid_t rgid, gid_t egid)
{
int result, Flags;

Flags=enhancer_checkconfig_xid_function(FUNC_SETGID, "setregid", rgid, egid, 0);

if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setregid(rgid, egid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setregid");
return(result);
}



int setresgid (gid_t rgid, gid_t egid, gid_t sgid)
{
int result, Flags;


Flags=enhancer_checkconfig_xid_function(FUNC_SETGID, "setresgid", rgid, egid, sgid);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

result=enhancer_real_setresuid(rgid, egid, sgid);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("setresgid");
return(result);
}


pid_t fork()
{
int result, Flags;

Flags=enhancer_checkconfig_chfile_function(FUNC_FORK, "fork", NULL, NULL, 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(getpid());

setenv("LD_PRELOAD", getenv("LD_PRELOAD"), 1);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("fork");
return(enhancer_real_fork());
}

pid_t vfork()
{
int result, Flags;

Flags=enhancer_checkconfig_chfile_function(FUNC_FORK, "fork", NULL, NULL, 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(getpid());

setenv("LD_PRELOAD", getenv("LD_PRELOAD"), 1);

if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("vfork");
return(enhancer_real_vfork());
}



int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
int result, Flags, val;
char *Redirect=NULL;

Flags=enhancer_checkconfig_with_redirect(FUNC_SELECT, "select", NULL, NULL, 0, 0, &Redirect);
if (strvalid(Redirect))
{
	timeout->tv_sec=0;

	val=atoi(Redirect);
	if (val > 1000) 
	{
		timeout->tv_sec=val / 1000;
		val -= (timeout->tv_sec * 1000);
	}

	timeout->tv_usec=val * 1000;
}


result=enhancer_real_select(nfds, readfds, writefds, exceptfds, timeout);
destroy(Redirect);

return(result);
}



int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
int result, Flags;
char *Redirect=NULL;

Flags=enhancer_checkconfig_with_redirect(FUNC_SELECT, "poll", NULL, NULL, 0, 0, &Redirect);
if (strvalid(Redirect)) timeout=atoi(Redirect);
result=enhancer_real_poll(fds, nfds, timeout);

destroy(Redirect);
return(result);
}



/*
int lchown (const char *file, uid_t owner, gid_t group)
{


}

int fchownat (int fd, const char *file, uid_t owner, gid_t group, int Flags)
{


}
*/



int __libc_start_main (int (*main) (int, char **, char **),
		   int argc, char **ubp_av,
		   void (*init) (void),
		   void (*fini) (void),
		   void (*rtld_fini) (void), void (*stack_end))
{
int result=0, i;
char *Args=NULL;

	enhancer_argc=argc;
	enhancer_argv=ubp_av;
	enhancer_init();

	for (i=1; i < argc; i++) 
	{
		if (i < enhancer_argc) enhancer_setvar("argv:curr", enhancer_argv[i]);
		if (i < (enhancer_argc-1) ) enhancer_setvar("argv:next", enhancer_argv[i+1]);
		result=enhancer_checkconfig_program_arg(enhancer_argv[i], NULL);
		Args=enhancer_strncat(Args, enhancer_argv[i], 0);
		Args=enhancer_strncat(Args, " ", 1);
	}
	result = enhancer_checkconfig_default(FUNC_MAIN, "main", Args, "", 0, 0);
	destroy(Args);

  return enhancer_real_main(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}


void enhancer_get_real_functions()
{
enhancer_exec_hooks();
enhancer_time_hooks();
enhancer_file_hooks();
enhancer_socket_hooks();

#ifdef HAVE_X11
enhancer_x11_hooks();
#endif

if (! enhancer_real_main) enhancer_real_main = dlsym (RTLD_NEXT, "__libc_start_main");

if (! enhancer_real_uname) enhancer_real_uname = dlsym(RTLD_NEXT, "uname");

if (! enhancer_real_setuid) enhancer_real_setuid= dlsym(RTLD_NEXT, "setuid");
if (! enhancer_real_setreuid) enhancer_real_setreuid= dlsym(RTLD_NEXT, "setreuid");
if (! enhancer_real_setresuid) enhancer_real_setresuid= dlsym(RTLD_NEXT, "setresuid");
if (! enhancer_real_setgid) enhancer_real_setgid= dlsym(RTLD_NEXT, "setgid");
if (! enhancer_real_setregid) enhancer_real_setregid= dlsym(RTLD_NEXT, "setregid");
if (! enhancer_real_setresgid) enhancer_real_setresgid= dlsym(RTLD_NEXT, "setresgid");


if (! enhancer_real_fork) enhancer_real_fork= dlsym(RTLD_NEXT, "fork");
if (! enhancer_real_vfork) enhancer_real_vfork= dlsym(RTLD_NEXT, "vfork");

if (! enhancer_real_select) enhancer_real_select= dlsym(RTLD_NEXT, "select");
if (! enhancer_real_poll) enhancer_real_poll= dlsym(RTLD_NEXT, "poll");
}
