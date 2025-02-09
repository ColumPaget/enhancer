#ifndef ENHANCER_CONFIG_H
#define ENHANCER_CONFIG_H

#include "sockinfo.h"


#define FLAG_DENY 1
#define FLAG_ALLOW 2
#define FLAG_PRETEND 4
#define FLAG_DIE   8
#define FLAG_ABORT 16
#define FLAG_FAILDIE 32
#define FLAG_REDIRECT 1024
#define FLAG_COLLECT 16384


#define FLAG_FSYNC      4096        //only in fdatasync
#define FLAG_SANITISE   4096	    //only in exec and system
#define FLAG_CACHE_FD  8192         //only file open
#define FLAG_NET_KEEPALIVE 8192     //only in network
#define FLAG_FDATASYNC 8192         //only for fsync
#define FLAG_DIE_ON_TAINT 8192	    //only in exec and system
#define FLAG_LOCK 16384             //only in fopt
#define FLAG_NET_DONTROUTE 16384    //only in bind/connect
#define FLAG_X_STAY_ABOVE 16384     //only in Xwindows
#define FLAG_DENY_ON_TAINT 16384    //only in exec and system
#define FLAG_CREATE 32768           //only in open
#define FLAG_SHRED 32768            //only in unlink
#define FLAG_NET_REUSEPORT 32768    //only in bind
#define FLAG_X_STAY_BELOW 32768     //only in Xwindows
#define FLAG_X_ICONIZED 65536
#define FLAG_TCP_NODELAY 65536
#define FLAG_BACKUP      65536
#define FLAG_X_FULLSCREEN 131072
#define FLAG_NOSYNC 131072
#define FLAG_TCP_QACK 131072
#define FLAG_X_NORMAL 262144
#define FLAG_DENY_SYMLINKS 262144
#define FLAG_NET_FREEBIND 262144
#define FLAG_X_UNMANAGED 524288
#define FLAG_DENY_LINKS 524288
#define FLAG_X_TRANSPARENT 1048576
#define FLAG_ALLOW_XSENDEVENT 2097152 //only in XNextEvent
#define FLAG_FADVISE_SEQU 2097152
#define FLAG_FADVISE_RAND 4194304
#define FLAG_FADVISE_NOCACHE 8388608


typedef enum {ACT_SETVAR, ACT_SETBASENAME, ACT_LOG, ACT_SEND, ACT_SYSLOG, ACT_SYSLOG_CRIT, ACT_ECHO, ACT_DEBUG, ACT_SLEEP, ACT_USLEEP, ACT_REDIRECT, ACT_FALLBACK, ACT_PIDFILE, ACT_LOCKFILE, ACT_EXEC, ACT_SEARCHPATH, ACT_XTERM_TITLE, ACT_CMOD, ACT_TTL, ACT_WRITEJAIL, ACT_UNSHARE, ACT_SETENV, ACT_GETHOSTIP, ACT_CHDIR, ACT_CHROOT, ACT_COPY_CLONE, ACT_LINK_CLONE, ACT_IPMAP, ACT_NO_DESCEND, ACT_MLOCKALL, ACT_MLOCKCURR} EEnhancerConfigActions;



typedef struct 
{
    int memcpy_max;
    int strcpy_max;
} enhancer_settings_struct;

extern enhancer_settings_struct enhancer_settings;

typedef enum {FUNC_ALL, FUNC_MAIN, FUNC_ONEXIT, FUNC_PROGRAM_ARG, FUNC_OPEN, FUNC_CLOSE, FUNC_READ, FUNC_WRITE, FUNC_UNAME, FUNC_DLOPEN, FUNC_DLCLOSE, FUNC_SOCKET, FUNC_CONNECT, FUNC_BIND, FUNC_LISTEN, FUNC_ACCEPT, FUNC_GETHOSTIP, FUNC_SPRINTF, FUNC_FORK, FUNC_EXEC, FUNC_SYSTEM, FUNC_SYSEXEC, FUNC_UNLINK, FUNC_RENAME, FUNC_SETUID, FUNC_SETGID, FUNC_CHOWN, FUNC_CHMOD, FUNC_CHDIR, FUNC_CHROOT, FUNC_TIME, FUNC_SETTIME, FUNC_MPROTECT, FUNC_FSYNC, FUNC_FDATASYNC, FUNC_SELECT, FUNC_XMapWindow, FUNC_XRaiseWindow, FUNC_XLowerWindow, FUNC_XSendEvent, FUNC_XNextEvent, FUNC_XLoadFont, FUNC_XChangeProperty} E_Funcs;

void enhancer_load_config();
int enhancer_checkconfig_default(int FuncID, const char *FuncName, const char *Str1, const char *Str2, int Int1, int Int2);
int enhancer_checkconfig_xid_function(int FuncID, const char *FuncName, long real, long effective, long saved);
int enhancer_checkconfig_chfile_function(int FuncID, const char *FuncName, const char *Path, const char *Dest, int val1, int val2);
int enhancer_checkconfig_with_redirect(int FuncID, const char *FuncName, const char *Str1, const char *Str2, int Int1, int Int2, char **Redirect);
int enhancer_checkconfig_program_arg(const char *Arg, char **Redirect);
TEnhancerConfig *enhancer_checkconfig_open_function(int FuncID, const char *FuncName, const char *Path, int Int1, int Int2, char **Redirect);
int enhancer_checkconfig_socket_function(int FuncID, const char *FuncName, TSockInfo *SockInfo);
int enhancer_checkconfig_exec_function(int FuncID, const char *FuncName, const char *Path, char **Redirect, char **TrustedPath);
void enhancer_config_destroy(TEnhancerConfig *Config);

#endif
