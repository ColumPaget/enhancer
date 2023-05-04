#include "common.h"
#include "actions.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>

int (*enhancer_real_chown) (const char *file, uid_t owner, gid_t group)=NULL;
int (*enhancer_real_fchown) (int fd, uid_t owner, gid_t group)=NULL;
int (*enhancer_real_chmod) (const char *file, mode_t mode)=NULL;
int (*enhancer_real_fchmod) (int fd,  mode_t mode)=NULL;
int (*enhancer_real_chdir) (const char *path)=NULL;
int (*enhancer_real_fchdir) (int fd)=NULL;
int (*enhancer_real_chroot) (const char *path)=NULL;
int (*enhancer_real_fchroot) (int fd)=NULL;

int fchown (int fd, uid_t owner, gid_t group)
{
    int result, Flags;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHOWN, "fchown", NULL, NULL, owner, group);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);

    fstat(fd, &Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchown(fd,owner,group);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("fchown");
    return(result);
}


int chown (const char *path, uid_t owner, gid_t group)
{
    int result, Flags, fd;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHOWN, "chown", path, NULL, owner, group);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);

    fd=open(path,O_RDWR);
    fstat(fd,&Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchown(fd,owner,group);

    close(fd);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("chown");
    return(result);
}


int fchmod (int fd, mode_t mode)
{
    int result, Flags;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHMOD, "fchmod", NULL, NULL, mode, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);

    fstat(fd,&Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchmod(fd,mode);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("fchmod");
    return(result);
}


int chmod(const char *path, mode_t mode)
{
    int result, Flags, fd;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHOWN, "chmod", path, NULL, mode, 0);
    fd=open(path,O_RDWR);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);

    fstat(fd,&Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchmod(fd,mode);

    close(fd);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("chmod");

    return(result);
}


int chdir(const char *path)
{
    int result, Flags, fd;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHDIR, "chdir", path, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);

    fd=open(path, O_PATH);
    fstat(fd,&Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchdir(fd);

    close(fd);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("chdir");

    return(result);
}


int fchdir(int fd)
{
    int result, Flags;
    struct stat Stat;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHDIR, "fchdir", NULL, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);

    fstat(fd,&Stat);

    if ((Flags & FLAG_DENY_LINKS) && (Stat.st_nlink > 1)) result=-1;
    else if ((Flags & FLAG_DENY_SYMLINKS) && (S_ISLNK(Stat.st_mode))) result=-1;
    else result=enhancer_real_fchdir(fd);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("fchdir");

    return(result);
}

int chroot(const char *path)
{
    int result, Flags, fd;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHROOT, "chroot", path, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);

    fd=open(path,O_RDWR);
    result=enhancer_real_fchroot(fd);
    if (result==0) enhancer_flags |= ENHANCER_STATE_CHROOTED;
    close(fd);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("chroot");

    return(result);
}


int fchroot(int fd)
{
    int result, Flags;

    Flags=enhancer_checkconfig_chfile_function(FUNC_CHROOT, "fchroot", NULL, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);


    result=enhancer_real_fchroot(fd);
    if (result==0) enhancer_flags |= ENHANCER_STATE_CHROOTED;

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("fchroot");

    return(result);
}



void enhancer_filesys_hooks()
{
    if (! enhancer_real_chown) enhancer_real_chown= dlsym(RTLD_NEXT, "chown");
    if (! enhancer_real_fchown) enhancer_real_fchown= dlsym(RTLD_NEXT, "fchown");
    if (! enhancer_real_chmod) enhancer_real_chmod= dlsym(RTLD_NEXT, "chmod");
    if (! enhancer_real_fchmod) enhancer_real_fchmod= dlsym(RTLD_NEXT, "fchmod");
    if (! enhancer_real_chdir) enhancer_real_chdir= dlsym(RTLD_NEXT, "chdir");
    if (! enhancer_real_fchdir) enhancer_real_fchdir= dlsym(RTLD_NEXT, "fchdir");
    if (! enhancer_real_chroot) enhancer_real_chroot= dlsym(RTLD_NEXT, "chroot");


    /*
    if (! enhancer_real_lchmod) enhancer_real_setuid= dlsym(RTLD_NEXT, "lchown");
    if (! enhancer_real_fchownat) enhancer_real_setuid= dlsym(RTLD_NEXT, "fchownat");
    */
    /*
    if (! enhancer_real_lchown) enhancer_real_setuid= dlsym(RTLD_NEXT, "lchown");
    if (! enhancer_real_fchownat) enhancer_real_setuid= dlsym(RTLD_NEXT, "fchownat");
    */
}

