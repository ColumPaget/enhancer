#include "common.h"
#include "file_shred.h"
#include <time.h>

static void ShredFile1Pass(int fd, struct stat *Stat)
{
    int i, bytes=0;
    char *Tempstr=NULL;

    Tempstr=(char *) calloc(1, 1024);
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


