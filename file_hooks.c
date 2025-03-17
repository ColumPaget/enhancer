#include "common.h"
#include "actions.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include "file_shred.h"


int (*enhancer_real_open)(const char *path, int oflag, ...)=NULL;
int (*enhancer_real_open64)(const char *path, int oflag, ...)=NULL;
int (*enhancer_real_openat)(int pos, const char *path, int oflag, ...)=NULL;
FILE *(*enhancer_real_fopen)(const char *path, const char *args)=NULL;
int (*enhancer_real_read)(int fd, char *Buffer, int len)=NULL;
int (*enhancer_real_write)(int fd, const void *Buffer, size_t len)=NULL;
int (*enhancer_real_close)(int fd)=NULL;
int (*enhancer_real_unlink)(const char *path)=NULL;
int (*enhancer_real_unlinkat)(int dirfd, const char *path, int flags)=NULL;
int (*enhancer_real_rename)(const char *from, const char *to)=NULL;
int (*enhancer_real_renameat)(int fromfd, const char *from, int tofd, const char *to)=NULL;
int (*enhancer_real_renameat2)(int fromfd, const char *from, int tofd, const char *to, int flags)=NULL;
int (*enhancer_real_fsync)(int fd)=NULL;
int (*enhancer_real_fdatasync)(int fd)=NULL;

typedef struct
{
    char *path;
    int mode;
    int fd;
} TCachedFD;


TCachedFD *FD_Cache=NULL;
int MaxFD=-1;
int OpenFDs=0;



int FindCachedFD(const char *path, int oflag)
{
    TCachedFD *p_curr;
    int i;

    if (! FD_Cache) return(-1);

    p_curr=FD_Cache;
    for (i=0; i <= MaxFD; i++)
    {
        p_curr=&FD_Cache[i];
        if ( (p_curr->fd > -1) && strvalid(p_curr->path) && (p_curr->mode==oflag) && (strcmp(p_curr->path, path)==0)) return(p_curr->fd);
    }

    return(-1);
}


const char *CachedFDGetPath(int fd)
{
    if (FD_Cache && (fd <= MaxFD) && FD_Cache[fd].path) return(FD_Cache[fd].path);
    return("");
}


void CloseCachedFD(int fd)
{
    TCachedFD *p_curr;
    int i;

    if (FD_Cache && (fd <= MaxFD))
    {
        p_curr=&FD_Cache[fd];
        if (p_curr->fd == fd)
        {
            p_curr->fd=-1;
            destroy(p_curr->path);
            p_curr->path=NULL;
            OpenFDs--;
        }
    }

}



void AddCachedFD(const char *path, int oflag, int fd)
{
    int i;

    if (MaxFD < fd)
    {
        if (MaxFD < 0) MaxFD=0;
        FD_Cache=realloc(FD_Cache, (fd+1) * sizeof(TCachedFD));
        for (i=MaxFD; i <=fd; i++)
        {
            FD_Cache[i].fd=-1;
            FD_Cache[i].path=NULL;
        }
        MaxFD=fd;
    }

    FD_Cache[fd].path=strdup(path);
    FD_Cache[fd].mode=oflag;
    FD_Cache[fd].fd=fd;

    OpenFDs++;
}


static void file_backup(const char *inpath)
{
    char *outpath=NULL;

    outpath=(char *) realloc(outpath, strlen(inpath) * 2);
    snprintf(outpath, strlen(inpath) *2, "%s.backup", inpath);
    enhancer_copyfile(inpath, outpath);

    destroy(outpath);
}



void FileOpenApplyConfig(int Flags, const char *path, int oflag, int fd)
{
    int val;

    AddCachedFD(path, oflag, fd);

    if (Flags & FLAG_LOCK)
    {
        val=LOCK_SH;
        if (oflag & O_WRONLY) val=LOCK_EX;
        if (oflag & O_RDWR) val=LOCK_EX;
        flock(fd, val);
    }

    if (Flags & FLAG_FADVISE_SEQU) posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (Flags & FLAG_FADVISE_RAND) posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM);
}



//this function does things that are common to all open functions
static int open_common(const char *funcname, int *fd, const char *path, int *oflags, mode_t *mode, char **redirect)
{
    const char *p_path;
    char *full_path=NULL;
    TEnhancerConfig *Conf;
    int Flags=0;

    if (*path != '/')
    {
        full_path=realloc(full_path, PATH_MAX);
        getcwd(full_path, PATH_MAX);
        full_path=enhancer_strncat(full_path, "/", 1);
        full_path=enhancer_strncat(full_path, path, 0);
    }
    else full_path=enhancer_strcpy(full_path, path);

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();
    p_path=enhancer_ConvertPathToChroot(full_path);

    *redirect=enhancer_strcpy(*redirect, full_path);

    Conf=enhancer_checkconfig_open_function(FUNC_OPEN, funcname, full_path, *oflags, 0, redirect);

//we will use 'redirect' as the path from here on in
    destroy(full_path);

    if (Conf)
    {
        Flags=Conf->Flags;

        if (Flags & FLAG_BACKUP) file_backup(*redirect);

        if (Flags & FLAG_NOSYNC) *oflags=*oflags ^= (O_SYNC | O_DSYNC);

//	if (Flags & FLAG_CMOD) *mode=Conf->ActInt;

        enhancer_config_destroy(Conf);

        if (Flags & FLAG_DENY) return(Flags);

        if (Flags & FLAG_CACHE_FD)
        {
            *fd=FindCachedFD(*redirect, *oflags);
            lseek(*fd, SEEK_SET, 0);
        }

        if ((Flags & FLAG_CREATE)) // && (! (*oflags & O_CREAT)))
        {
            *oflags |= O_CREAT;
            *mode=0600;
        }
    }

    return(Flags);
}

int open64(const char *ipath, int oflags, ...)
{
    int fd=-1, Flags;
    va_list args;
    char *path=NULL;
    mode_t mode=0;

    if (oflags & O_CREAT)
    {
        va_start(args, oflags);
        mode=va_arg(args, mode_t);
        va_end(args);
    }

    Flags=open_common("open64", &fd, ipath, &oflags, &mode, &path);
    if ((Flags & FLAG_CACHE_FD) && (fd > -1))
    {
        destroy(path);
        return(fd);
    }

    fd=enhancer_real_open64(path, oflags, mode);
    if (fd > -1) FileOpenApplyConfig(Flags, path, oflags, fd);

    destroy(path);
    return(fd);
}

int open(const char *ipath, int oflags, ...)
{
    int fd=-1, Flags;
    va_list args;
    char *path=NULL;
    mode_t mode=0;

    if (oflags & O_CREAT)
    {
        va_start(args, oflags);
        mode=va_arg(args, mode_t);
        va_end(args);
    }

#ifdef O_LARGEFILE
    oflags |= O_LARGEFILE;
#endif

    Flags=open_common("open", &fd, ipath, &oflags, &mode, &path);
    if ((Flags & FLAG_CACHE_FD) && (fd > -1))
    {
        destroy(path);
        return(fd);
    }

    fd=enhancer_real_open(path, oflags, mode);
    if (fd > -1) FileOpenApplyConfig(Flags, path, oflags, fd);

    destroy(path);
    return(fd);
}

int openat(int atfd, const char *ipath, int oflags, ...)
{
    va_list args;
    mode_t mode=0;

    if (oflags & O_CREAT)
    {
        va_start(args, oflags);
        mode=va_arg(args, mode_t);
        va_end(args);
    }

    return(open(ipath, oflags, mode));
}




FILE *fopen(const char *ipath, const char *args)
{
    FILE *f=NULL;
    const char *ptr;
    char *path=NULL;
    int Flags, oflags=0, fd=-1, mode=0600;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=open_common("open", &fd, ipath, &oflags, &mode, &path);

    for (ptr=args; *ptr !='\0'; ptr++)
    {
        switch (*ptr)
        {
        case 'r':
            oflags |= O_RDONLY;
            break;
        case 'w':
            oflags |= O_WRONLY|O_CREAT|O_TRUNC;
            break;
        case 'a':
            oflags |= O_WRONLY|O_CREAT;
            break;
        case '+':
            oflags &= ~ (O_WRONLY | O_RDONLY);
            oflags |= O_RDWR;
            break;
        }
    }

    if ((Flags & FLAG_CACHE_FD) && (fd > -1)); // use cached fd
    else fd=enhancer_real_open(path, oflags, mode);

    if (fd > -1) FileOpenApplyConfig(Flags, path, oflags, fd);

    if (fd > -1) f=fdopen(fd, args);

    destroy(path);
    return(f);
}

/*
FILE *fopen64(const char *ipath, const char *args)
{
    return(fopen(ipath, args));
}
*/


int close(int fd)
{
    int Flags;
    const char *path="";

    if (fd == -1) return(0);
    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    path=CachedFDGetPath(fd);
    Flags=enhancer_checkconfig_default(FUNC_CLOSE, "close", path, NULL, 0, 0);

    if (Flags & FLAG_FDATASYNC)
    {
        if (enhancer_real_fdatasync) enhancer_real_fdatasync(fd);
    }

    if (Flags & FLAG_FSYNC)
    {
        if (enhancer_real_fsync) enhancer_real_fsync(fd);
    }

    if (Flags & FLAG_FADVISE_NOCACHE) posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

    CloseCachedFD(fd);

    return(enhancer_real_close(fd));
}


/* THIS FUNCTION NOT IMPLEMENTED YET
int write(int fd, const void *Data, size_t len)
{
char *redirect=NULL;
int Flags, result;

if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();
Flags=enhancer_checkconfig_with_redirect(FUNC_WRITE, "write", Data, NULL, fd, len, &redirect);

result=enhancer_real_write(fd, Data, len);
destroy(redirect);

return(result);
}
*/


int unlink(const char *path)
{
    int Flags, result;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=enhancer_checkconfig_default(FUNC_UNLINK, "unlink", path, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);
    if (Flags & FLAG_SHRED) ShredFileAt(AT_FDCWD, path);

    result=enhancer_real_unlink(path);
    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("unlink");

    return(result);
}


int unlinkat(int dirfd, const char *path, int flags)
{
    int Flags, result;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=enhancer_checkconfig_default(FUNC_UNLINK, "unlink", path, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_PRETEND) return(0);
    if (Flags & FLAG_SHRED) ShredFileAt(dirfd, path);

    result=enhancer_real_unlinkat(dirfd, path, flags);
    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("unlinkat");

    return(result);
}


int rename(const char *from, const char *to)
{
    int Flags, result;
    char *redirect=NULL;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=enhancer_checkconfig_with_redirect(FUNC_RENAME, "rename", from, to, 0, 0, &redirect);
//fprintf(stderr, "RENAME: %d %s\n", Flags, redirect);

    if (Flags & FLAG_DENY)
    {
        destroy(redirect);
        return(-1);
    }

    if (Flags & FLAG_PRETEND)
    {
        destroy(redirect);
        return(0);
    }

		if (strvalid(redirect)) result=enhancer_real_rename(from, redirect);
    else result=enhancer_real_rename(from, to);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("rename");

    destroy(redirect);
    return(result);
}


int renameat(int fromdirfd, const char *from, int todirfd, const char *to)
{
    int Flags, result;
    char *redirect=NULL;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=enhancer_checkconfig_with_redirect(FUNC_RENAME, "rename", from, to, 0, 0, &redirect);
    if (Flags & FLAG_DENY)
    {
        destroy(redirect);
        return(-1);
    }

    if (Flags & FLAG_PRETEND)
    {
        destroy(redirect);
        return(0);
    }


    result=enhancer_real_renameat(fromdirfd, from, todirfd, to);
    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("renameat");

    destroy(redirect);
    return(result);
}


int renameat2(int fromdirfd, const char *from, int todirfd, const char *to, unsigned int flags)
{
    int Flags, result;
    char *redirect=NULL;

    if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

    Flags=enhancer_checkconfig_with_redirect(FUNC_RENAME, "rename", from, to, 0, 0, &redirect);

    if (Flags & FLAG_DENY)
    {
        destroy(redirect);
        return(-1);
    }

    if (Flags & FLAG_PRETEND)
    {
        destroy(redirect);
        return(0);
    }

    result=enhancer_real_renameat2(fromdirfd, from, todirfd, to, flags);
    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("renameat2");

    destroy(redirect);
    return(result);
}


int fsync(int fd)
{
    int Flags;

    Flags=enhancer_checkconfig_default(FUNC_UNLINK, "fsync", NULL, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_NOSYNC) return(0);
    if (Flags & FLAG_PRETEND) return(0);
    if (Flags & FLAG_FDATASYNC)
    {
        if (enhancer_real_fdatasync) return(enhancer_real_fdatasync(fd));
    }
    return(enhancer_real_fsync(fd));
}


int fdatasync(int fd)
{
    int Flags;

    Flags=enhancer_checkconfig_default(FUNC_UNLINK, "fsync", NULL, NULL, 0, 0);
    if (Flags & FLAG_DENY) return(-1);
    if (Flags & FLAG_NOSYNC) return(0);
    if (Flags & FLAG_PRETEND) return(0);
    if (Flags & FLAG_FSYNC)
    {
        if (enhancer_real_fsync) return(enhancer_real_fsync(fd));
    }
    if (enhancer_real_fdatasync) return(enhancer_real_fdatasync(fd));
    return(0);
}




void enhancer_file_hooks()
{
    if (! enhancer_real_open64) enhancer_real_open64 = dlsym(RTLD_NEXT, "open64");
    if (! enhancer_real_open) enhancer_real_open = dlsym(RTLD_NEXT, "open");
    if (! enhancer_real_openat) enhancer_real_openat = dlsym(RTLD_NEXT, "openat");
    if (! enhancer_real_fopen) enhancer_real_fopen = dlsym(RTLD_NEXT, "fopen");
    if (! enhancer_real_read) enhancer_real_read = dlsym(RTLD_NEXT, "read");
    if (! enhancer_real_write) enhancer_real_write = dlsym(RTLD_NEXT, "write");
    if (! enhancer_real_close) enhancer_real_close = dlsym(RTLD_NEXT, "close");
    if (! enhancer_real_unlink) enhancer_real_unlink = dlsym(RTLD_NEXT, "unlink");
    if (! enhancer_real_unlinkat) enhancer_real_unlinkat = dlsym(RTLD_NEXT, "unlinkat");
    if (! enhancer_real_rename) enhancer_real_rename = dlsym(RTLD_NEXT, "rename");
    if (! enhancer_real_renameat) enhancer_real_renameat = dlsym(RTLD_NEXT, "renameat");
    if (! enhancer_real_renameat2) enhancer_real_renameat2 = dlsym(RTLD_NEXT, "renameat2");
    if (! enhancer_real_fsync) enhancer_real_fsync = dlsym(RTLD_NEXT, "fsync");
    if (! enhancer_real_fdatasync) enhancer_real_fsync = dlsym(RTLD_NEXT, "fdatasync");
}
