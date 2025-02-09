#include "actions.h"
#include "common.h"
#include "config.h"
#include "vars.h"
#include "iplist.h"
#define _GNU_SOURCE
#include <sched.h>
#include <syslog.h>
#include <wait.h>
#include <sys/file.h>
#include <sys/mman.h>
#include "net.h"

char *EnhancerChrootDir=NULL;



const char *enhancer_ConvertPathToChroot(const char *Path)
{
    if (! EnhancerChrootDir) return(Path);
    if (strcmp(Path,EnhancerChrootDir)==0) return(Path+strlen(EnhancerChrootDir));

    return(Path);
}



char *enhancer_FileSearch(char *RetStr, const char *FilePath, const char *SearchPath)
{
    const char *ptr, *fname;
    char *Dir=NULL;

    if (access(FilePath, F_OK)==0) return(enhancer_strcpy(RetStr, FilePath));

    fname=strrchr(FilePath, '/');
    if (! fname) fname=FilePath;

    ptr=enhancer_strtok(SearchPath, ",:", &Dir);
    while (ptr)
    {
        RetStr=enhancer_strcpy(RetStr, Dir);
        RetStr=enhancer_strncat(RetStr, "/", 0);
        RetStr=enhancer_strncat(RetStr, fname, 0);
        if (access(RetStr, F_OK)==0) break;
        ptr=enhancer_strtok(ptr, ",:", &Dir);
    }

    destroy(Dir);
    return(RetStr);
}




static void enhancer_action_log(int fd, int Type, const char *Fmt,  const char *FuncName, const char *Str1, const char *Str2)
{
    char *LogStr=NULL;

    if (! strvalid(Fmt)) return;

    LogStr=enhancer_format_str(LogStr, Fmt, FuncName, Str1, Str2);

    switch (Type)
    {
    case ACT_SYSLOG:
        syslog(LOG_INFO,"%s",LogStr);
        break;
    case ACT_SYSLOG_CRIT:
        syslog(LOG_CRIT,"%s",LogStr);
        break;

    case ACT_LOG:
        if (fd == -1) return;
    case ACT_ECHO:
    case ACT_DEBUG:
        LogStr=enhancer_strncat(LogStr, "\n", 1);
        if (enhancer_real_write) enhancer_real_write(fd, LogStr, strlen(LogStr));
        posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
        break;
    }
    destroy(LogStr);
}


void enhancer_action_send(const char *Arg, const char *FuncName, const char *Str1, const char *Str2)
{
    char *Tempstr=NULL, *URL=NULL, *Str=NULL;
    const char *ptr;

    ptr=enhancer_strtok(Arg, "|", &Tempstr);
    URL=enhancer_format_str(URL, Tempstr, FuncName, Str1, Str2);
    ptr=enhancer_strtok(ptr, "|", &Tempstr);
    Str=enhancer_format_str(Str, Tempstr, FuncName, Str1, Str2);
    Str=enhancer_strncat(Str, "\n", 1);
    net_send(URL, Str);

    destroy(Tempstr);
    destroy(URL);
    destroy(Str);
}


void enhancer_action_unshare(const char *Arg)
{
#ifndef HAVE_UNSHARE
    char *Token=NULL, *HostName=NULL;
    const char *ptr;
    int Flags=0;
    int uid, gid, fd;

    ptr=enhancer_strtok(Arg, ",", &Token);
    while (ptr)
    {
        if (strcasecmp(Token, "user")==0) Flags |= CLONE_NEWUSER;
        else if (strcasecmp(Token, "net")==0) Flags |= CLONE_NEWNET;
        else if (strcasecmp(Token, "uts")==0) Flags |= CLONE_NEWUTS;
        else if (strcasecmp(Token, "ipc")==0) Flags |= CLONE_NEWIPC;
        else if (strcasecmp(Token, "pid")==0) Flags |= CLONE_NEWPID;
        else if (strcasecmp(Token, "mount")==0) Flags |= CLONE_NEWNS;
        else if (strncasecmp(Token, "host=", 5)==0) HostName=enhancer_strcpy(HostName, Token+5);
        ptr=enhancer_strtok(ptr, ",", &Token);
    }

    uid=getuid();
    gid=getgid();
    if (Flags > 0)
    {
        unshare(Flags);
        if (strvalid(HostName)) sethostname(HostName, strlen(HostName));
        if (Flags & CLONE_NEWUSER)
        {
            Token=realloc(Token, 1024);
            snprintf(Token, 1000, "/proc/%d/setgroups", getpid());
            fd=enhancer_real_open(Token, O_WRONLY | O_TRUNC);
            if (fd)
            {
                snprintf(Token, 1000, "deny\n");
                write(fd, Token, strlen(Token));
                close(fd);
            }

            snprintf(Token, 1000, "/proc/%d/uid_map", getpid());
            fd=enhancer_real_open(Token, O_WRONLY | O_TRUNC);
            if (fd)
            {
                snprintf(Token, 1000, "%d %d 1\n", uid, uid);
                write(fd, Token, strlen(Token));
                close(fd);
            }

            snprintf(Token, 1000, "/proc/%d/gid_map", getpid());
            fd=enhancer_real_open(Token, O_WRONLY | O_TRUNC);
            if (fd)
            {
                snprintf(Token, 1000, "%d %d 1\n", gid, gid);
                write(fd, Token, strlen(Token));
                close(fd);
            }

        }
    }

    destroy(Token);
    destroy(HostName);

#endif

}



void enhancer_action_setenv(const char *Env, const char *FuncName, const char *Str1, const char *Str2)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    int result;

    Tempstr=enhancer_format_str(Tempstr, Env, FuncName, Str1, Str2);
    ptr=enhancer_strtok(Tempstr, "=", &Name);
    if (ptr)
    {
        ptr++;
        result=setenv(Name, ptr, TRUE);
    }

    destroy(Tempstr);
    destroy(Name);
    destroy(Value);
}




void enhancer_action_clone(int Type, const char *Src)
{
    const char *ptr;
    char *Tempstr=NULL;

    ptr=strrchr(Src, '/');
    Tempstr=enhancer_strncat(Tempstr, Src, ptr-Src);
    if (strvalid(Tempstr))
    {
        ptr=Tempstr;
        if (*ptr=='/') ptr++;
        enhancer_mkdir_path(ptr, 0700);
    }

    switch (Type)
    {
    case ACT_LINK_CLONE:
        ptr=Src;
        if (*ptr=='/') ptr++;
        link(Src, ptr);
        break;

    case ACT_COPY_CLONE:
        ptr=Src;
        if (*ptr=='/') ptr++;
        enhancer_copyfile(Src, ptr);
        break;

    }
    destroy(Tempstr);
}

int enhancer_actions(TEnhancerConfig *Conf, const char *FuncName, const char *Str1, const char *Str2, char **Redirect)
{
    char *Tempstr=NULL, *Value=NULL;
    const char *ptr;
    int i, fd, result;
    TConfigItem *Act;
    struct stat FStat;
    int Flags=0;

    for (i=0; i < Conf->NoOfActions; i++)
    {
        Act=&Conf->Actions[i];

        switch( Act->Type )
        {
        case ACT_LOG:
        case ACT_SYSLOG:
        case ACT_SYSLOG_CRIT:
            enhancer_action_log(enhancer_log_fd, Act->Type, Act->StrArg,  FuncName, Str1, Str2);
            break;

        case ACT_ECHO:
            enhancer_action_log(1, Act->Type, Act->StrArg,  FuncName, Str1, Str2);
            break;

        case ACT_DEBUG:
            enhancer_action_log(2, Act->Type, Act->StrArg,  FuncName, Str1, Str2);
            break;

        case ACT_SETVAR:
            enhancer_func_setvar(Act->StrArg, FuncName, Str1, Str2);
            break;

        case ACT_SETENV:
            enhancer_action_setenv(Act->StrArg, FuncName, Str1, Str2);
            break;

        case ACT_SETBASENAME:
            Tempstr=enhancer_strcpy(Tempstr, Str1);
            enhancer_func_setvar(Act->StrArg, FuncName, basename(Tempstr), Str2);
            break;

        case ACT_SEND:
            enhancer_action_send(Act->StrArg, FuncName, Str1, Str2);
            break;

        case ACT_SLEEP:
            sleep(Act->IntArg);
            break;
        case ACT_USLEEP:
            usleep(Act->IntArg);
            break;

        case ACT_PIDFILE:
        case ACT_LOCKFILE:
            Tempstr=enhancer_format_str(Tempstr, Act->StrArg, "", "", "");
            fd=enhancer_real_open(Tempstr, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            if (fd > -1)
            {
                if (Act->Type==ACT_LOCKFILE) flock(fd, LOCK_EX);
                Tempstr=enhancer_format_str(Tempstr, "%p", "", "", "");
                enhancer_real_write(fd, Tempstr, strlen(Tempstr));
                enhancer_real_close(fd);
            }
            break;


        case ACT_XTERM_TITLE:
            Tempstr=enhancer_format_str(Tempstr, Act->StrArg, FuncName, Str1, Str2);
            enhancer_real_write(1, "\x1b]2;", 4);
            enhancer_real_write(1, Tempstr, strlen(Tempstr));
            enhancer_real_write(1, "\007", 1);
            break;

        case ACT_UNSHARE:
            enhancer_action_unshare(Act->StrArg);
            break;

        case ACT_EXEC:
            enhancer_real_system(Act->StrArg);
            break;

        case ACT_REDIRECT:
            if (Redirect)
            {
                Flags |= FLAG_REDIRECT;
                Tempstr=enhancer_strcpy(Tempstr, *Redirect);
                *Redirect=enhancer_format_str(*Redirect, Act->StrArg, FuncName, Tempstr, Str2);
            }
            break;

        case ACT_FALLBACK:
            if (Redirect)
            {
                Flags |= FLAG_REDIRECT;
                *Redirect=enhancer_strncat(*Redirect, ",", 1);
                Tempstr=enhancer_format_str(Tempstr, Act->StrArg, FuncName, Str1, Str2);
                *Redirect=enhancer_strncat(*Redirect, Tempstr, 0);
            }
            break;

        case ACT_SEARCHPATH:
            if (Redirect)
            {
                Flags |= FLAG_REDIRECT;
                Tempstr=enhancer_strcpy(Tempstr, *Redirect);
                *Redirect=enhancer_FileSearch(*Redirect, Tempstr, Act->StrArg);
            }
            break;

        case ACT_WRITEJAIL:
            result=stat(*Redirect, &FStat);
            if ( Redirect && ((result==-1) || S_ISREG(FStat.st_mode)) )
            {
                Flags |= FLAG_REDIRECT;
                Tempstr=enhancer_strcpy(Tempstr, *Redirect);
                *Redirect=enhancer_format_str(*Redirect, Act->StrArg, FuncName, Tempstr, Str2);
                *Redirect=enhancer_strncat(*Redirect, "/", 1);
                mkdir(*Redirect, 0700);
                Tempstr=enhancer_strcpy(Tempstr, Str1);
                enhancer_strrep(Tempstr, '/', '_');
                *Redirect=enhancer_strncat(*Redirect, Tempstr, 0);
                if (access(*Redirect, F_OK) !=0)
                {
                    enhancer_copyfile(Str1, *Redirect);
                }
            }
            break;

        case ACT_IPMAP:
            if (Redirect)
            {
                Flags |= FLAG_REDIRECT;
                Tempstr=enhancer_strcpy(Tempstr, *Redirect);
                *Redirect=enhancer_map_ip(*Redirect, Tempstr);
            }
            break;

        case ACT_GETHOSTIP:
            Value=net_lookuphost(Value, Act->StrArg);
            Tempstr=enhancer_strcpy(Tempstr, "ip:");
            Tempstr=enhancer_strncat(Tempstr, Act->StrArg, 0);
            enhancer_setvar(Tempstr, Value);
            break;

        case ACT_CHDIR:
            enhancer_real_chdir(Act->StrArg);
            break;

        case ACT_CHROOT:
            Tempstr=realloc(Tempstr, PATH_MAX +10);
            getcwd(Tempstr, PATH_MAX);
            EnhancerChrootDir=enhancer_strcpy(EnhancerChrootDir, Tempstr);
            if (enhancer_real_chroot) enhancer_real_chroot(".");
            break;

        case ACT_COPY_CLONE:
        case ACT_LINK_CLONE:
            enhancer_action_clone(Act->Type, Act->StrArg);
            break;

        case ACT_MLOCKALL:
            mlockall(MCL_FUTURE);
            break;

        case ACT_MLOCKCURR:
            mlockall(MCL_CURRENT);
            break;

        case ACT_NO_DESCEND:
            enhancer_flags |= ENHANCER_STATE_NO_DESCEND;
            break;
        }
    }



    if (Conf->GlobalFlags & FLAG_COLLECT) waitpid(-1, NULL, WNOHANG);

		//ACT_ABORT is handled here
    if (Conf->GlobalFlags & FLAG_ABORT) abort();

		//ACT_DIE is handled here
    if (Conf->GlobalFlags & FLAG_DIE) exit(1);


    destroy(Tempstr);
    destroy(Value);

    return(Flags);
}

