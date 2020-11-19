
#ifndef ENHANCER_COMMON_H
#define ENHANCER_COMMON_H

#define _GNU_SOURCE
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>


#define TRUE 1
#define FALSE 0

#define ENHANCER_STATE_GOTFUNCS 1
#define ENHANCER_STATE_INITDONE 2
#define ENHANCER_STATE_CONFIGDONE 4
#define ENHANCER_STATE_HAS_ATEXIT 8
#define ENHANCER_STATE_NO_DESCEND 16
#define ENHANCER_STATE_CHROOTED 2147483648

#define destroy(item) ((item != NULL) ? free(item) : 0)
#define strvalid(str) ( ((str !=NULL) && (*str != '\0')) ? 1 : 0)


typedef struct
{
int Type;
int Op;
int IntArg;
char *StrArg;
} TConfigItem;

typedef struct
{
  int Type;
  int GlobalFlags;
  int Flags;
	int NoOfMatches;
	TConfigItem *Matches;
  int NoOfActions;
  TConfigItem *Actions;
} TEnhancerConfig;

typedef struct
{
int NoOfConfigs;
TEnhancerConfig *Items;
} TEnhancerFuncConfig;


extern unsigned int enhancer_flags;
extern int enhancer_log_fd;
extern int enhancer_argc;
extern char **enhancer_argv;

void __attribute__ ((constructor)) enhancer_init(void);

int StrListMatch(const char *MatchStr, const char *List);
extern void enhancer_fail_die(const char *FuncName);
extern int (*enhancer_real_open)(const char *path, int oflag, ...);
extern int (*enhancer_real_open64)(const char *path, int oflag, ...);
extern int (*enhancer_real_close)(int fd);
extern int (*enhancer_real_write)(int fd, const void *Buffer, size_t len);
extern int (*enhancer_real_uname)(struct utsname *);
extern int (*enhancer_real_system)(const char *command);
extern int (*enhancer_real_socket)(int domain, int type, int protocol);
extern int (*enhancer_real_connect)(int socket, const struct sockaddr *address, socklen_t address_len);
extern int (*enhancer_real_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern FILE *(*enhancer_real_fopen)(const char *path, const char *args);
extern int (*enhancer_real_sprintf)(char *str, const char *format, ...);
extern int (*enhancer_real_snprintf)(char *str, size_t len, const char *format, ...);
extern char * (*enhancer_real_strcpy)(char *dest, const char *src);
extern char * (*enhancer_real_strncpy)(char *dest, const char *src, size_t len);
extern void *(*enhancer_real_memcpy)(void *dest, const void *src, size_t len);
extern int (*enhancer_real_execl)(const char *path, const char *arg, ...);
extern int (*enhancer_real_execlp)(const char *file, const char *arg, ...);
//extern int (*enhancer_real_execle)(const char *path, const char *arg, ..., char * const envp[]); 
extern int (*enhancer_real_execv)(const char *path, char *const argv[]);
extern int (*enhancer_real_execvp)(const char *file, char *const argv[]);
extern int (*enhancer_real_execvpe)(const char *file, char *const argv[], char *const envp[]);
extern int (*enhancer_real_chdir) (const char *path);
extern int (*enhancer_real_chroot) (const char *file);

void enhancer_init();
char *fread_until(char *RetBuff, FILE *f);

char *enhancer_strncat(char *dest, const char *src, int slen);
char *enhancer_strcpy(char *dest, const char *src);
void enhancer_strrep(char *Str, char inchar, char outchar);
const char *enhancer_istrtok(const char *Str, const char *Dividers, char **Tok);
const char *enhancer_strtok(const char *Str, const char *Dividers, char **Tok);
const char *enhancer_spacetok(const char *Str, char **Tok);
void enhancer_mkdir_path(const char *path, int mode);
char *enhancer_read_file(char *RetBuff, const char *Path);
void enhancer_copyfile(const char *src, const char *dst);
int enhancer_match_token_from_list(const char *Token, char *List[]);
char *enhancer_format_str(char *RetStr, const char *Fmt, const char *FuncName, const char *Str1, const char *Str2);


#endif
