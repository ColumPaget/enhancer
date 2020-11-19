#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include "common.h"
#include "config.h"
#include "hooks.h"
#include <linux/limits.h>
#include <time.h>
#include <fnmatch.h>
#include <sys/sendfile.h>
#include "time_hooks.h"
#include "vars.h"
#include "exit.h"

unsigned int enhancer_flags=0;
int enhancer_log_fd=-1;

int enhancer_argc;
char **enhancer_argv;




void enhancer_fail_die(const char *FuncName)
{
  fprintf(stderr, "FATAL: %s failed!\n", FuncName);
  syslog(LOG_CRIT, "FATAL: %s failed!\n", FuncName);
  exit(1);
}


int StrListMatch(const char *MatchStr, const char *List)
{
const char *ptr;
int result=FALSE;
char *Tok=NULL;

if (! strvalid(MatchStr)) return(FALSE);
ptr=enhancer_strtok(List, ",", &Tok);
while (ptr)
{
  if (fnmatch(Tok, MatchStr, 0)==0)
  {
    result=TRUE;
    break;
  }
  ptr=enhancer_strtok(ptr, ",", &Tok);
}

destroy(Tok);

return(result);
}



char *enhancer_strcpy_dequote(char *dest, const char *src)
{
	int dpos=0, i, slen;
	const char *ptr;

	slen=strlen(src);

	dest=(char *) realloc(dest, slen+10);
	for (ptr=src; *ptr != '\0'; ptr++)
	{
		if (*ptr != '\\')
		{
		dest[dpos]=*ptr;
		dpos++;
		}
	}

	dest[dpos]='\0';

	return(dest);
}


char *enhancer_strncat(char *dest, const char *src, int slen)
{
	int dlen;

	if (dest) dlen=strlen(dest);
	else dlen=0;

	if (slen < 1) 
	{
		if (src==NULL) slen=0;
		else slen=strlen(src);
	}

	dest=(char *) realloc(dest, dlen+slen+10);
	if (slen > 0) memcpy(dest+dlen,src,slen);
	dest[dlen+slen]='\0';

	return(dest);
}



char *enhancer_strcpy(char *dest, const char *src)
{
if (dest) *dest='\0';
return(enhancer_strncat(dest, src, 0));
}


const char *enhancer_istrtok(const char *Str, const char *Dividers, char **Tok)
{
const char *sptr=NULL, *eptr=NULL, *rptr;

if (*Tok) **Tok='\0';
if ((! Str) || (*Str=='\0')) return(NULL);


sptr=Str;

if (strchr(Dividers, *sptr)) 
{
	eptr=sptr+1;
	rptr=eptr;
}
else
{
switch (*sptr)
{
	case '"':
	eptr=sptr+1;
	while ((*eptr !='\0') && (*eptr != *sptr)) 
	{
		//don't treat a quoted character as an end-quote
		if (*eptr == '\\') eptr++;

		//iterate ptr, but not if we're at end of string
		if (*eptr != '\0') eptr++;
	}
	sptr++;
	rptr=eptr+1;
	break;

	case '\n':
	eptr=sptr+1;
	rptr=eptr;
	break;

	default:
	eptr=sptr;
	while (*eptr != '\0') 
	{
		if (*eptr=='\\') eptr++;
		else if (strchr(Dividers, *eptr)) break;
		else if (*eptr=='\n') break;

		if (*eptr != '\0') eptr++;
	}
	rptr=eptr;
	break;
}
}


if (eptr > sptr)
{
	*Tok=enhancer_strcpy(*Tok, "");
	*Tok=enhancer_strncat(*Tok, sptr, eptr-sptr);
}

return(rptr);
}


const char *enhancer_strtok(const char *Str, const char *Dividers, char **Tok)
{
const char *sptr;

if (! Str) return(NULL);
sptr=Str;
while ((*sptr !='\0') && strchr(Dividers,*sptr)) sptr++;
return(enhancer_istrtok(sptr, Dividers, Tok));
}

const char *enhancer_spacetok(const char *Str, char **Tok)
{
return(enhancer_strtok(Str, " \t\r", Tok));
}

void enhancer_strrep(char *Str, char inchar, char outchar)
{
char *ptr;

for (ptr=Str; *ptr !='\0'; ptr++) 
{
	if (*ptr==inchar) *ptr=outchar;
}
}


int enhancer_match_token_from_list(const char *Token, char *List[])
{
int i;

for (i=0; List[i] !=NULL; i++)
{
	if (strcmp(Token,List[i])==0) return(i);
}

return(-1);
}


char *enhancer_read_file(char *RetBuff, const char *Path)
{
struct stat Stat;
int fd, result;

result=stat(Path,&Stat);
if (result==-1) 
{
	free(RetBuff);
	return(NULL);
}

fd=enhancer_real_open(Path,O_RDONLY);
if (fd == -1)
{
	free(RetBuff);
	return(NULL);
}

result=Stat.st_size;

//files in /proc and /sys show up as having zero length, but contain data if we read them
if (result==0) result=4096;
RetBuff=realloc(RetBuff,result+1);
result=read(fd,RetBuff,result);
RetBuff[result]='\0';

enhancer_real_close(fd);

return(RetBuff);
}



void enhancer_set_default_vars()
{
char *Tempstr=NULL;
struct passwd *pwd;
uid_t uid;

Tempstr=(char *) calloc(255,1);
snprintf(Tempstr,10,"%d",getpid());
enhancer_setvar("pid",Tempstr);

uid=getuid();
snprintf(Tempstr,10,"%d",uid);
enhancer_setvar("uid",Tempstr);

snprintf(Tempstr,10,"%d",getgid());
enhancer_setvar("gid",Tempstr);

if (enhancer_argc > 0) enhancer_setvar("progname",basename(enhancer_argv[0]));
/*
pwd=getpwuid(uid);
if (pwd)
{
	enhancer_setvar("user",pwd->pw_name);
	enhancer_setvar("home",pwd->pw_dir);
}
*/

free(Tempstr);
}

const char *enhancer_format_addvar(const char *Input, char **RetStr, int *pos)
{
const char *start, *ptr, *p_var;
char *Name=NULL;
int len, val;

start=Input+2;
ptr=strchr(start, ')');
if (! ptr) return(Input+1);

p_var="";
Name=enhancer_strncat(Name, start, ptr-start);

if (strncmp(Name, "argv[", 5)==0)
{
	val=strtol(Name+5, NULL, 10);
	if ((val > -1 ) && (val < enhancer_argc)) p_var=enhancer_argv[val];
}
else p_var=enhancer_getvar(Name);

len=strlen(p_var);
strncat(*RetStr, p_var, len);
*pos+=len;

destroy(Name);
return(ptr);
}



char *enhancer_format_str(char *RetStr, const char *Fmt, const char *FuncName, const char *Str1, const char *Str2)
{
char *ValStr=NULL, *Tempstr=NULL;
const char *ptr, *p_tmp;
int LogLen=1000, len;
struct tm *TM;
time_t Now;
int pos=0;


//+3 to give a bit of extra space for '\n' and '\0' and one for luck
RetStr=(char *) realloc(RetStr, LogLen+3);
memset(RetStr, 0, LogLen+3);
if (! Fmt) return(RetStr);

for (ptr=Fmt; *ptr != '\0'; ptr++)
{
	if (*ptr == '%')
	{
		ptr++;
		switch (*ptr)
		{
		case 'f': 
			len=strlen(FuncName);
			if ((pos + len) < (LogLen)) 
			{
				strcat(RetStr+pos, FuncName);	
				pos+=len;
			}
		break; 	

		case '1':
			if (strvalid(Str1))
			{
			len=strlen(Str1);
			if ((pos + len) < (LogLen))
			{
				strcat(RetStr+pos, Str1);	
				pos+=len;
			}
			}
		break; 	

		case '2':
			if (strvalid(Str2))
			{
			len=strlen(Str2);
			if ((pos + len) < (LogLen))
			{
				strcat(RetStr+pos, Str2);	
				pos+=len;
			}
			}
		break;

		case 'n':
			p_tmp=basename(enhancer_argv[0]);
			strcat(RetStr, p_tmp);	
			pos+=strlen(p_tmp);
		break;

		case 'N':
			p_tmp=enhancer_argv[0];
			strcat(RetStr, p_tmp);	
			pos+=strlen(p_tmp);
		break;


		case 'p':
			ValStr=realloc(ValStr, 110);
			snprintf(ValStr, 100, "%d", getpid());
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;

		case 'd':
			ValStr=realloc(ValStr, PATH_MAX);
			getcwd(ValStr, PATH_MAX);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;

		case 'T':
			Now=enhancer_gettime();
			TM=localtime(&Now);
			ValStr=realloc(ValStr, 110);
			strftime(ValStr, 100, "%H:%M:%S", TM);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;

		case 't':
			Now=enhancer_gettime();
			TM=localtime(&Now);
			ValStr=realloc(ValStr, 110);
			ptr++;
			Tempstr=realloc(Tempstr, 110);
			snprintf(Tempstr, 100, "%%%c", *ptr);
			strftime(ValStr, 100, Tempstr, TM);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;


		case 'D':
			ValStr=realloc(ValStr, 110);
			Now=enhancer_gettime();
			TM=localtime(&Now);
			strftime(ValStr, 100, "%Y\%m\%d", TM);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;

		case 'h':
			ValStr=realloc(ValStr, 110);
			gethostname(ValStr, 100);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;

		case 'H':
			ValStr=realloc(ValStr, 110);
			gethostname(ValStr, 100);
			Tempstr=realloc(Tempstr, 110);
			getdomainname(Tempstr, 100);
			ValStr=enhancer_strncat(ValStr, ".", 1);
			ValStr=enhancer_strncat(ValStr, Tempstr, 0);
			strcat(RetStr, ValStr);	
			pos+=strlen(ValStr);
		break;


		default: RetStr[pos]=*ptr; pos++; break;
		}
	}
	else if (strncmp(ptr, "$(", 2)==0) ptr=enhancer_format_addvar(ptr, &RetStr, &pos);
	else 
	{
		RetStr[pos]=*ptr; 
		pos++; 
	}
}

destroy(Tempstr);
destroy(ValStr);

return(RetStr);
}


void enhancer_copyfile(const char *src, const char *dst)
{
int infd, outfd;

infd=enhancer_real_open(src, O_RDONLY);
if (infd > -1)
{
outfd=enhancer_real_open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0600);
if (outfd > -1)
{

	while (sendfile(outfd, infd, NULL, 4096) > 0);
	close(outfd);
}
close(infd);
}

}


void enhancer_mkdir_path(const char *path, int mode)
{
char *Tempstr=NULL;
const char *ptr;

ptr=strchr(path, '/');
while (ptr)
{
ptr++;
Tempstr=enhancer_strncat(Tempstr, path, ptr-path);
mkdir(Tempstr, mode);
ptr=strchr(ptr, '/');
}
mkdir(path, mode);

destroy(Tempstr);
}


//void __attribute__ ((constructor)) enhancer_init(void)
void enhancer_init(void)
{
char *ptr=NULL;

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) return;

if (! enhancer_flags & ENHANCER_STATE_GOTFUNCS) enhancer_get_real_functions();

if (enhancer_argc > 0) ptr=basename(enhancer_argv[0]);
if (ptr && strlen(ptr)) 
{
	enhancer_flags |= ENHANCER_STATE_CONFIGDONE;
	enhancer_set_default_vars();
	enhancer_load_config();
}

if (enhancer_flags & ENHANCER_STATE_HAS_ATEXIT) atexit(enhancer_atexit);


if (enhancer_flags & ENHANCER_STATE_INITDONE) return;

memset(&enhancer_settings,0,sizeof(enhancer_settings));
enhancer_settings.strcpy_max=1024;


enhancer_flags |= ENHANCER_STATE_INITDONE;
}
