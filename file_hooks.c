#include "common.h"
#include "actions.h"
#include "config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>

int (*enhancer_real_open)(const char *path, int oflag, ...)=NULL;
int (*enhancer_real_open64)(const char *path, int oflag, ...)=NULL;
int (*enhancer_real_openat)(int pos, const char *path, int oflag, ...)=NULL;
FILE *(*enhancer_real_fopen)(const char *path, const char *args)=NULL;
int (*enhancer_real_read)(int fd, char *Buffer, int len)=NULL;
int (*enhancer_real_write)(int fd, const void *Buffer, size_t len)=NULL;
int (*enhancer_real_close)(int fd)=NULL;
int (*enhancer_real_unlink)(const char *path)=NULL;
int (*enhancer_real_unlinkat)(int dirfd, const char *path, int flags)=NULL;
int (*enhancer_real_fsync)(int fd)=NULL;
int (*enhancer_real_fdatasync)(int fd)=NULL;
int (*enhancer_real_chown) (const char *file, uid_t owner, gid_t group)=NULL;
int (*enhancer_real_fchown) (int fd, uid_t owner, gid_t group)=NULL;
int (*enhancer_real_chmod) (const char *file, mode_t mode)=NULL;
int (*enhancer_real_fchmod) (int fd,  mode_t mode)=NULL;
int (*enhancer_real_chdir) (const char *path)=NULL;
int (*enhancer_real_fchdir) (int fd)=NULL;
int (*enhancer_real_chroot) (const char *path)=NULL;
int (*enhancer_real_fchroot) (int fd)=NULL;

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


static void ShredFile1Pass(int fd, struct stat *Stat)
{
int i, bytes=0;
char *Tempstr=NULL;

Tempstr=calloc(1, 1024);
srand(time(NULL) + clock());
for (i=0; i < 1024; i++) Tempstr[i]=rand() % 0xFF;

lseek(fd, SEEK_SET, 0);
while (bytes < Stat->st_size)
{
bytes+=enhancer_real_write(fd, Tempstr, 1024);
}

destroy(Tempstr);
}


void ShredFileAt(int dirfd, const char *path)
{
struct stat Stat;
int fd;

	fstatat(dirfd, path, &Stat, 0);
	if (! S_ISLNK(Stat.st_mode) && (Stat.st_nlink == 1))
	{
		fd=enhancer_real_openat(dirfd, path, O_WRONLY);
		if (fd > -1)
		{
			ShredFile1Pass(fd, &Stat);
			ShredFile1Pass(fd, &Stat);
			ShredFile1Pass(fd, &Stat);
		}
		close(fd);
	}
	else syslog(LOG_WARNING, "shredfile failed for %s. File is symlink or has linkcount > 1", path);
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
p_path=EnhancerConvertPathToChroot(full_path);

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
	case 'r': oflags |= O_RDONLY; break;
	case 'w': oflags |= O_WRONLY|O_CREAT|O_TRUNC; break;
	case 'a': oflags |= O_WRONLY|O_CREAT; break;
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



int unlink(const char *path)
{
int Flags;

if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

Flags=enhancer_checkconfig_default(FUNC_UNLINK, "unlink", path, NULL, 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);
if (Flags & FLAG_SHRED) ShredFileAt(AT_FDCWD, path);

return(enhancer_real_unlink(path));
}


int unlinkat(int dirfd, const char *path, int flags)
{
int Flags;

if (! (enhancer_flags & ENHANCER_STATE_INITDONE)) enhancer_init();

Flags=enhancer_checkconfig_default(FUNC_UNLINK, "unlink", path, NULL, 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);
if (Flags & FLAG_SHRED) ShredFileAt(dirfd, path);
return(enhancer_real_unlinkat(dirfd, path, flags));
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




int fchown (int fd, uid_t owner, gid_t group)
{
int result, Flags;
struct stat Stat;

Flags=enhancer_checkconfig_chfile_function(FUNC_CHOWN, "fchown", NULL, NULL, owner, group);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_PRETEND) return(0);

fstat(fd,&Stat);

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
if (! enhancer_real_fsync) enhancer_real_fsync = dlsym(RTLD_NEXT, "fsync");
if (! enhancer_real_fdatasync) enhancer_real_fsync = dlsym(RTLD_NEXT, "fdatasync");
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
