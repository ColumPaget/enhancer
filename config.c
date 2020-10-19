#include "common.h"
#include "config.h"
#include "actions.h"
#include <pwd.h>
#include <grp.h>
#include "exit.h"

extern char *enhancer_prog_name;



typedef enum {MATCH_ALL, MATCH_PATH, MATCH_BASENAME, MATCH_FAMILY, MATCH_PROTO, MATCH_PEER, MATCH_PORT, MATCH_USER, MATCH_GROUP, MATCH_FD, MATCH_ARG, MATCH_CHROOTED} EMatchTypes;


#define OP_EQ  0
#define OP_NOT 1
#define OP_GT  2
#define OP_LT  3


char *EnhancerFuncNames[]={"all","main", "onexit", "arg", "open", "close", "read", "write", "uname", "socket", "connect", "bind", "listen", "accept", "gethostip", "sprintf", "fork", "exec", "system", "sysexec", "unlink", "setuid", "setgid", "chown", "chmod", "chdir", "chroot", "time","settime","mprotect", "fsync", "fdatasync", "select", "XMapWindow","XRaiseWindow", "XLowerWindow", "XSendEvent", "XLoadFont", "XChangeProperty", NULL};


char *EnhancerTokNames[]={"deny","allow","die","abort","setvar","setbasename","log","syslog","syslogcrit","echo", "debug", "send", "exec", "die-on-fail", "collect", "sleep", "usleep", "deny-links","deny-symlinks","redirect","fallback","chrooted","if-chrooted","path","basename","peer","port","user","group","family","fd", "arg", "keepalive", "localnet", "reuseport", "tcp-qack", "tcp-nodelay", "ttl", "freebind", "cmod", "lock", "fdcache","create", "shred", "searchpath", "xstayabove", "xstaybelow", "xiconized", "xunmanaged", "xfullscreen", "xtransparent", "xnormal","pidfile","lockfile", "xtermtitle","backup", "nosync", "fsync", "fdatasync", "writejail", "unshare", "setenv", "getip", "cd", "chroot", "copyclone", "linkclone", "ipmap", "fadv_seq", "fadv_rand", "fadv_nocache", "qlen", "sanitise", "die-on-taint", "deny-on-taint", NULL};

typedef enum {TOK_DENY, TOK_ALLOW, TOK_DIE, TOK_ABORT, TOK_SETVAR, TOK_SETBASENAME, TOK_LOG, TOK_SYSLOG, TOK_SYSLOGCRIT, TOK_ECHO, TOK_DEBUG, TOK_SEND, TOK_EXEC, TOK_FAILDIE, TOK_COLLECT, TOK_SLEEP, TOK_USLEEP, TOK_DENYLINKS, TOK_DENYSYMLINKS, TOK_REDIRECT, TOK_FALLBACK, TOK_CHROOTED, TOK_CHROOTED2, TOK_PATH, TOK_BASENAME, TOK_PEER, TOK_PORT, TOK_USER, TOK_GROUP, TOK_FAMILY, TOK_FD, TOK_ARG, TOK_KEEPALIVE, TOK_LOCALNET, TOK_REUSEPORT, TOK_TCP_QACK, TOK_TCP_NODELAY, TOK_TTL, TOK_FREEBIND, TOK_CMOD, TOK_LOCK, TOK_FDCACHE, TOK_CREATE, TOK_SHRED, TOK_SEARCHPATH, TOK_X11_STAY_ABOVE, TOK_X11_STAY_BELOW, TOK_X11_ICONIZED, TOK_X11_UNMANAGED, TOK_X11_FULLSCREEN, TOK_X11_TRANSPARENT, TOK_X11_NORMAL, TOK_PIDFILE, TOK_LOCKFILE, TOK_XTERM_TITLE, TOK_BACKUP, TOK_NOSYNC, TOK_FSYNC, TOK_FDATASYNC, TOK_WRITEJAIL, TOK_UNSHARE, TOK_SETENV, TOK_GETIP, TOK_CHDIR, TOK_CHROOT, TOK_COPY_CLONE, TOK_LINK_CLONE, TOK_IPMAP, TOK_FADV_SEQU, TOK_FADV_RAND, TOK_FADV_NOCACHE, TOK_QLEN, TOK_SANITISE, TOK_DIE_ON_TAINT, TOK_DENY_ON_TAINT} TActions;

char *EnhancerFamilyNames[]={"unix","raw","netlink","net","ip4","ip6",NULL};
typedef enum {FAMILY_UNIX, FAMILY_RAW, FAMILY_NETLINK, FAMILY_NET, FAMILY_IP4, FAMILY_IP6} E_NETFAM;


TEnhancerFuncConfig *FuncConfigs=NULL;

static const char *enhancer_parse_config_item(const char *ConfigStr, TEnhancerConfig *Conf, int FuncID);



void enhancer_config_destroy(TEnhancerConfig *Config)
{
int i;

for (i=0; i < Config->NoOfActions; i++) destroy(Config->Actions[i].StrArg);

destroy(Config->Matches);
destroy(Config->Actions);
destroy(Config);
}


static TEnhancerConfig *enhancer_create_config_item(int FuncID)
{
TEnhancerFuncConfig *FuncConfig;
TEnhancerConfig *Config;

FuncConfig=& FuncConfigs[FuncID];
if ((FuncConfig->NoOfConfigs % 10) ==0) FuncConfig->Items=(TEnhancerConfig *) realloc(FuncConfig->Items, (FuncConfig->NoOfConfigs + 10) * sizeof(TEnhancerConfig));
Config=& FuncConfig->Items[FuncConfig->NoOfConfigs];
FuncConfig->NoOfConfigs++;
memset(Config, 0, sizeof(TEnhancerConfig));
Config->Type=FuncID;

return(Config);
}


static int enhancer_flag_matches_func(int func, int ConfID)
{
switch (ConfID)
{
case TOK_X11_NORMAL:
case TOK_X11_STAY_ABOVE:
case TOK_X11_STAY_BELOW:
case TOK_X11_ICONIZED:
case TOK_X11_FULLSCREEN:
case TOK_X11_UNMANAGED:
case TOK_X11_TRANSPARENT:
if (func != FUNC_XMapWindow) return(FALSE);
break;

case TOK_FSYNC:
if ((func != FUNC_CLOSE) && (func != FUNC_FDATASYNC)) return(FALSE);
break;

case TOK_FDATASYNC:
if ((func != FUNC_CLOSE) && (func != FUNC_FSYNC)) return(FALSE);
break;

case TOK_LOCK:
case TOK_FDCACHE: 
case TOK_CREATE:
//case TOK_FADVISE_SEQU:
//case TOK_FADVISE_RAND:
case TOK_NOSYNC:
if (func != FUNC_OPEN) return(FALSE);
break;

case TOK_FADV_NOCACHE:
if (func != FUNC_CLOSE) return(FALSE);
break;

case TOK_REUSEPORT:
case TOK_FREEBIND:
if (func != FUNC_BIND) return(FALSE);
break;

case TOK_TCP_NODELAY:
case TOK_KEEPALIVE:
case TOK_LOCALNET:
switch (func)
{
	case FUNC_CONNECT:
	case FUNC_BIND:
	case FUNC_ACCEPT:
	return(TRUE);
	break;

	default:
	return(FALSE);
	break;
}
break;


case TOK_SHRED:
if (func != FUNC_UNLINK) return(TRUE);
return(FALSE);
break;


case TOK_BACKUP:
if ((func != FUNC_CLOSE) && (func != FUNC_OPEN)) return(TRUE);
break;
}

/*
#define TOK_TCP_QACK 131072
#define TOK_DENY_SYMLINKS 262144
#define TOK_DENY_LINKS 524288
*/

return(TRUE);
}



static void enhancer_set_config(TEnhancerConfig *Config, int ConfID, const char *ConfName, int GlobalFlag, int Flag)
{
if (enhancer_flag_matches_func(Config->Type, ConfID))
{
Config->GlobalFlags |= GlobalFlag;
Config->Flags |= Flag;
}
else fprintf(stderr, "WARN: %s doesn't apply to function %s\n", EnhancerFuncNames[Config->Type]);

}



static void enhancer_add_match(TEnhancerConfig *Config, int Type, int Op, int MatchInt, const char *MatchArg)
{
TConfigItem *Match;
struct passwd *pwent;
struct group *grent;


Config->NoOfMatches++;
Config->Matches=realloc(Config->Matches, Config->NoOfMatches * sizeof(TConfigItem));
Match=&Config->Matches[Config->NoOfMatches - 1];
Match->Type=Type;
Match->Op=Op;

switch (Type)
{
case MATCH_USER:
 pwent=getpwnam(MatchArg);
 if (pwent) Match->IntArg=pwent->pw_uid;
break;

case MATCH_GROUP:
 grent=getgrnam(MatchArg);
 if (grent) Match->IntArg=grent->gr_gid;
break;

case MATCH_FAMILY:
	if (MatchArg) Match->IntArg=enhancer_match_token_from_list(MatchArg, EnhancerFamilyNames);
break;

case MATCH_PORT:
case MATCH_FD:
	if (MatchArg) Match->IntArg=atoi(MatchArg);
break;

default:
	Match->IntArg=MatchInt;
	Match->StrArg=enhancer_strcpy(NULL, MatchArg);
break;
}
}


/*
 
	case ACT_LOG:
	case ACT_SYSLOG:
	case ACT_SYSLOG_CRIT:
	case ACT_SEND:
	case ACT_ECHO:
	case ACT_DEBUG:
	case ACT_SLEEP:
	case ACT_USLEEP:
	case ACT_PIDFILE:
	case ACT_SETVAR:
	case ACT_SETENV:
	case ACT_SETBASENAME:
	case ACT_EXEC:
	case ACT_XTERM_TITLE:
	case ACT_UNSHARE:
	case ACT_GETHOSTIP:
	case ACT_CHDIR:
	case ACT_CHROOT:
	case ACT_COPY_CLONE:
	case ACT_LINK_CLONE:
	case ACT_REDIRECT:
	case ACT_FALLBACK:
	case ACT_SEARCHPATH:
	case ACT_CMOD:
	case ACT_TTL:
	case ACT_WRITEJAIL: 
	case ACT_IPMAP:
	break;

 */

static int enhancer_action_matches_func(int func, int action)
{
switch (func)
{
case FUNC_ALL:
break;

case FUNC_MAIN:
case FUNC_ONEXIT:
case FUNC_CLOSE:
	switch (action) 
	{
		case ACT_REDIRECT:
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
		case ACT_CMOD:
		case ACT_TTL:
		case ACT_WRITEJAIL: 
		case ACT_IPMAP:
			return(FALSE);
		break;
	
		default:
			return(TRUE);
			break;
	}
break;

case FUNC_OPEN:
	switch (action)
	{
	case ACT_TTL:
	case ACT_IPMAP:
		return(FALSE);
	break;

	default:
		return(TRUE);
	break;
	}
break;

case FUNC_BIND:
case FUNC_CONNECT:
	switch (action)
	{
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

case FUNC_ACCEPT:
	switch (action)
	{
		case ACT_REDIRECT:
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

case FUNC_SYSTEM:
case FUNC_EXEC:
case FUNC_XLoadFont:
	switch (action)
	{
		case ACT_TTL:
		case ACT_IPMAP:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

case FUNC_GETHOSTIP:
	switch (action)
	{
		case ACT_TTL:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

case FUNC_UNAME:
case FUNC_TIME:
case FUNC_SETTIME:
case FUNC_SETUID:
case FUNC_SETGID:
case FUNC_CHOWN:
case FUNC_CHMOD:
case FUNC_CHDIR:
case FUNC_CHROOT:
case FUNC_MPROTECT:
case FUNC_XChangeProperty:
	switch (action)
	{
		case ACT_TTL:
		case ACT_IPMAP:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

case FUNC_FORK:
case FUNC_UNLINK:
case FUNC_XMapWindow:
case FUNC_XRaiseWindow:
case FUNC_XLowerWindow:
	switch (action)
	{
		case ACT_TTL:
		case ACT_IPMAP:
		case ACT_CMOD:
		case ACT_WRITEJAIL: 
		case ACT_REDIRECT:
		case ACT_FALLBACK:
		case ACT_SEARCHPATH:
			return(FALSE);
		break;
	
		default:
			return(TRUE);
		break;
	}
break;

/*
case FUNC_READ:
case FUNC_WRITE:
case FUNC_SPRINTF:
case FUNC_XSendEvent:
case FUNC_PROGRAM_ARG:
case FUNC_SOCKET:
break;
*/
}

return(TRUE);
}



static void enhancer_add_action(TEnhancerConfig *Config, int Action, const char *ActName, int IntArg, const char *StrArg)
{
TConfigItem *Act;

if (enhancer_action_matches_func(Config->Type, Action))
{
Config->NoOfActions++;
Config->Actions=realloc(Config->Actions, Config->NoOfActions * sizeof(TConfigItem));
Act=&Config->Actions[Config->NoOfActions - 1];
Act->Type=Action;
Act->IntArg=IntArg;
if (StrArg) Act->StrArg=enhancer_strcpy(NULL, StrArg);
//as we used realloc we must set StrArg to NULL if we don't set
//it to a value
else Act->StrArg=NULL;
}
else fprintf(stderr,"WARNING: Action %s is unsuitable for function %s. Not adding\n", ActName, EnhancerFuncNames[Config->Type]);

}

static const char *enhancer_config_tok(const char *Config, char **Tok)
{
const char *dividers="# \t\r!=<>\r\n";
const char *ptr;

if (! Config) return(NULL);
ptr=Config;
while (*ptr==' ') ptr++;
ptr=enhancer_istrtok(ptr, dividers, Tok);

return(ptr);
}



static int enhancer_combine_flags(int CurrFlags, int NewFlags)
{
int Flags;

Flags=CurrFlags;

if (NewFlags & FLAG_DENY) Flags &= ~FLAG_ALLOW;
else if (NewFlags & FLAG_ALLOW) Flags &= ~FLAG_DENY;
Flags |= NewFlags;

//mixing FLAG_DENY and FLAG_ALLOW gets you FLAG_DENY
if ((Flags & FLAG_DENY) && (Flags & FLAG_ALLOW)) Flags &= ~FLAG_ALLOW;

return(Flags);
}


static void enhancer_combine_config(TEnhancerConfig *Combined, TEnhancerConfig *New)
{
TConfigItem *Act;
int i;

Combined->GlobalFlags |= New->GlobalFlags;
Combined->Flags=enhancer_combine_flags(Combined->Flags, New->Flags);
for (i=0; i < New->NoOfActions; i++)
{
Act=&New->Actions[i];
enhancer_add_action(Combined, Act->Type, "", Act->IntArg, Act->StrArg);
}

}


static int ConfigStrMatch(TConfigItem *Config, const char *MatchStr)
{
int result;
char *Item=NULL;
const char *ptr, *p_MatchStr;

	switch(Config->Type)
	{
		case MATCH_BASENAME:
		p_MatchStr=basename(MatchStr);
		break;


		case MATCH_PROTO:
		ptr=enhancer_strtok(MatchStr, ":", &Item);
		ptr=enhancer_strtok(ptr, ":", &Item);
		p_MatchStr=Item;
		break;


		case MATCH_PEER:
		ptr=enhancer_strtok(MatchStr, ":", &Item);
		ptr=enhancer_strtok(ptr, ":", &Item);
		p_MatchStr=Item;
		break;

		case MATCH_PORT:
		ptr=enhancer_strtok(MatchStr, ":", &Item);
		ptr=enhancer_strtok(ptr, ":", &Item);
		ptr=enhancer_strtok(ptr, ":", &Item);
		p_MatchStr=Item;
		break;

		default:
		p_MatchStr=MatchStr;
		break;
	}

	result=StrListMatch(p_MatchStr, Config->StrArg);
	destroy(Item);

	if (Config->Op==OP_NOT) result = !result;

	if (result) return(TRUE);
	return(FALSE);
}


int enhancer_config_matches(TEnhancerConfig *Config, int MatchInt, const char *MatchStr)
{
TConfigItem *Match;
int i, result;

if (Config->NoOfMatches==0) return(TRUE);

for (i=0; i < Config->NoOfMatches; i++)
{
	Match=&Config->Matches[i];

	switch (Match->Type)
	{
		case MATCH_ALL: return(TRUE);

		case MATCH_ARG:
		case MATCH_PATH:
		case MATCH_PEER:
		case MATCH_PORT:
		case MATCH_BASENAME:
		if (ConfigStrMatch(Match, MatchStr)) return(TRUE);
		break;

		case MATCH_CHROOTED:
			if (enhancer_flags & ENHANCER_STATE_CHROOTED) return(TRUE);
		break;

		case MATCH_FAMILY:
			switch (Match->IntArg)
			{
				case FAMILY_UNIX: if (MatchInt==AF_UNIX) return(TRUE); break;
				case FAMILY_NETLINK: if (MatchInt==AF_NETLINK) return(TRUE); break;
				case FAMILY_NET: if ((MatchInt==AF_INET) || (MatchInt==AF_INET6)) return(TRUE); break;
				case FAMILY_IP4: if (MatchInt==AF_INET) return(TRUE); break;
				case FAMILY_IP6: if (MatchInt==AF_INET6) return(TRUE); break;
			}
		break;

		case MATCH_USER:
			if (Match->IntArg == getuid()) return(TRUE);
		break;

		case MATCH_GROUP:
			if (Match->IntArg == getgid()) return(TRUE);
		break;

		case MATCH_FD:
			if (Match->IntArg == MatchInt) return(TRUE);
		break;
	}
}

	return(FALSE);
}

static void enhancer_get_functype_config(int FuncID, TEnhancerConfig *Combined, const char *MatchStr, int MatchInt)
{
TEnhancerConfig *Config;
int i;

for (i=0; i < FuncConfigs[FuncID].NoOfConfigs; i++)
{
	Config=&FuncConfigs[FuncID].Items[i];
	if (enhancer_config_matches(Config, MatchInt, MatchStr)) 
	{
			enhancer_combine_config(Combined, Config);
	}
}
}


static TEnhancerConfig *enhancer_getconfig(int FuncID, const char *Str1, const char *Str2, int Int1, int Int2)
{
TEnhancerConfig *Combined;


if (! FuncConfigs) return(NULL);

Combined=(TEnhancerConfig *) calloc(1,sizeof(TEnhancerConfig));

enhancer_get_functype_config(0, Combined, Str1, Int1);
enhancer_get_functype_config(FuncID, Combined, Str1, Int1);

//if no flags are set, then there are no actions to carry out
if (
		(! (Combined->Flags | Combined->GlobalFlags)) &&
		(Combined->NoOfActions < 1)
	)
{
	destroy(Combined);
	return(NULL);
}

return(Combined);
}





int enhancer_checkconfig_default(int FuncID, const char *FuncName, const char *Str1, const char *Str2, int Int1, int Int2)
{
TEnhancerConfig *Conf=NULL;
int Flags=0;

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) Conf=enhancer_getconfig(FuncID, Str1, Str2, Int1, Int2);

if (Conf)
{
	enhancer_actions(Conf, FuncName, Str1, Str2, NULL);
	Flags=Conf->Flags;
	enhancer_config_destroy(Conf);
}

return(Flags);
}



int enhancer_checkconfig_xid_function(int FuncID, const char *FuncName, long real, long effective, long saved)
{
char *Tempstr=NULL;
int result;

Tempstr=realloc(Tempstr, 110);
snprintf(Tempstr, 100, "%ld", real);
result=enhancer_checkconfig_default(FuncID, FuncName, Tempstr, NULL, real, effective);
destroy(Tempstr);
return(result);
}

int enhancer_checkconfig_chfile_function(int FuncID, const char *FuncName, const char *Path, const char *Dest, int val1, int val2)
{
return(enhancer_checkconfig_default(FuncID, FuncName, Path, Dest, 0, 0));
}



TEnhancerConfig *enhancer_checkconfig_open_function(int FuncID, const char *FuncName, const char *Path, int Int1, int Int2, char **Redirect)
{
char *p_open_flags;
TEnhancerConfig *Conf=NULL;

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) Conf=enhancer_getconfig(FuncID, Path, "", Int1, Int2);

if (Conf)
{
	if (Int1 & O_APPEND) p_open_flags="append";
	else if (Int1 & O_WRONLY) p_open_flags="write";
	else if (Int1 & O_RDWR) p_open_flags="read/write";
	else p_open_flags="read";

	enhancer_actions(Conf, FuncName, Path, p_open_flags, Redirect);

}

return(Conf);
}


int enhancer_checkconfig_with_redirect(int FuncID, const char *FuncName, const char *Str1, const char *Str2, int Int1, int Int2, char **Redirect)
{
TEnhancerConfig *Conf;
int Flags=0;

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) Conf=enhancer_getconfig(FuncID, Str1, Str2, Int1, Int2);

if (Conf)
{
Flags=Conf->Flags;
enhancer_actions(Conf, FuncName, Str1, Str2, Redirect);
enhancer_config_destroy(Conf);
}

return(Flags);
}


int enhancer_checkconfig_program_arg(const char *Arg, char **Redirect)
{
return(enhancer_checkconfig_with_redirect(FUNC_PROGRAM_ARG, "arg", Arg, "", 0, 0, Redirect));
}


int enhancer_checkconfig_socket_function(int FuncID, const char *FuncName, TSockInfo *SockInfo)
{
char *Tempstr=NULL;
const char *sock_family;
TEnhancerConfig *Conf=NULL;
TConfigItem *Act;
int Flags=0, i;

sock_family=sockinfo_family_name(SockInfo);

Tempstr=(char *) calloc(1025,1);
if (strvalid(SockInfo->address)) snprintf(Tempstr,1024, "%s:%s:%d", sock_family, SockInfo->address, SockInfo->port);

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) Conf=enhancer_getconfig(FuncID, Tempstr, "", SockInfo->family, SockInfo->port);

if (Conf)
{

	Flags=Conf->Flags;
	Flags |= enhancer_actions(Conf, FuncName, Tempstr, sock_family, &SockInfo->redirect);


  for (i=0; i < Conf->NoOfActions; i++)
  { 
    Act=&Conf->Actions[i];
    switch (Act->Type)
    { 
      case ACT_TTL: SockInfo->ttl=Act->IntArg; break;
    }
  }
	enhancer_config_destroy(Conf);
}

destroy(Tempstr);

return(Flags);
}


int enhancer_checkconfig_exec_function(int FuncID, const char *FuncName, const char *Path, char **Redirect, char **TrustedPath)
{
TEnhancerConfig *Conf=NULL;
int Flags=0;

if (enhancer_flags & ENHANCER_STATE_CONFIGDONE) Conf=enhancer_getconfig(FuncID, Path, "", 0, 0);

if (Conf)
{
  enhancer_actions(Conf, FuncName, Path, NULL, Redirect);
	Flags=Conf->Flags;
	enhancer_config_destroy(Conf);
}
return(Flags);
}


static const char *enhancer_parse_match(const char *Input, TEnhancerConfig *Conf, int FuncID, int Type)
{
char *Tempstr=NULL;
const char *ptr;
int Op=OP_EQ;

if (! Input) return(NULL);

ptr=enhancer_config_tok(Input, &Tempstr);
if (strvalid (Tempstr))
{
if (strcmp(Tempstr, "!")==0) 
{
	Op=OP_NOT;
	ptr=enhancer_config_tok(ptr, &Tempstr);
}
if (strcmp(Tempstr, "=")==0) ptr=enhancer_config_tok(ptr, &Tempstr);

enhancer_add_match(Conf, Type, Op, 0, Tempstr);
}

destroy(Tempstr);
return(ptr);
}


static const char *enhancer_parse_config_item(const char *ConfigStr, TEnhancerConfig *Conf, int FuncID)
{
char *Tempstr=NULL, *Name=NULL;
const char *ptr;
int val;

ptr=enhancer_config_tok(ConfigStr, &Name);
while (ptr)
{

if (strcmp(Name,"\n")==0) break;

	val=enhancer_match_token_from_list(Name, EnhancerTokNames);
	switch (val)
	{
		case TOK_PATH:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_PATH);
		break;

		case TOK_BASENAME:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_BASENAME);
		break;

		case TOK_PEER:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_PEER);
		break;

		case TOK_USER:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_USER);
		break;

		case TOK_GROUP:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_GROUP);
		break;

		case TOK_FAMILY:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_FAMILY);
		break;

		case TOK_ARG:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_ARG);
		break;

		case TOK_FD:
			ptr=enhancer_parse_match(ptr, Conf, FuncID, MATCH_FD);
		break;

		case TOK_CHROOTED:
		break;

		case TOK_DENY:
			enhancer_set_config(Conf, val, Name, 0, FLAG_DENY);
		break;

		case TOK_ALLOW:
			enhancer_set_config(Conf, val, Name, 0, FLAG_ALLOW);
		break;

		case TOK_FAILDIE:
			enhancer_set_config(Conf, val, Name, FLAG_FAILDIE, 0);
		break;

		case TOK_DIE:
			enhancer_set_config(Conf, val, Name, FLAG_DIE, 0);
		break;

		case TOK_ABORT:
			enhancer_set_config(Conf, val, Name, FLAG_ABORT, 0);
		break;

		case TOK_COLLECT:
			enhancer_set_config(Conf, val, Name, FLAG_COLLECT, 0);
		break;

		case TOK_BACKUP:
			enhancer_set_config(Conf, val, Name, 0, FLAG_BACKUP);
		break;

		case TOK_KEEPALIVE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_NET_KEEPALIVE);
		break;

		case TOK_LOCALNET:
			enhancer_set_config(Conf, val, Name, 0, FLAG_NET_DONTROUTE);
		break;

		case TOK_REUSEPORT:
			enhancer_set_config(Conf, val, Name, 0, FLAG_NET_REUSEPORT);
		break;

		case TOK_LOCK:
			enhancer_set_config(Conf, val, Name, 0, FLAG_LOCK);
		break;

		case TOK_FDCACHE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_CACHE_FD);
		break;

		case TOK_CREATE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_CREATE);
		break;

		case TOK_SHRED:
			enhancer_set_config(Conf, val, Name, 0, FLAG_SHRED);
		break;

		case TOK_NOSYNC:
			enhancer_set_config(Conf, val, Name, 0, FLAG_NOSYNC);
		break;

		case TOK_FSYNC:
			enhancer_set_config(Conf, val, Name, 0, FLAG_FSYNC);
		break;

		case TOK_FDATASYNC:
			enhancer_set_config(Conf, val, Name, 0, FLAG_FDATASYNC);
		break;

		case TOK_TCP_QACK:
			enhancer_set_config(Conf, val, Name, 0, FLAG_TCP_QACK);
		break;

		case TOK_TCP_NODELAY:
			enhancer_set_config(Conf, val, Name, 0, FLAG_TCP_NODELAY);
		break;

		case TOK_FREEBIND:
			enhancer_set_config(Conf, val, Name, 0, FLAG_NET_FREEBIND);
		break;

		case TOK_FADV_SEQU:
			enhancer_set_config(Conf, val, Name, 0, FLAG_FADVISE_SEQU);
		break;

		case TOK_FADV_RAND:
			enhancer_set_config(Conf, val, Name, 0, FLAG_FADVISE_RAND);
		break;

		case TOK_FADV_NOCACHE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_FADVISE_NOCACHE);
		break;

		case TOK_X11_STAY_ABOVE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_STAY_ABOVE);
		break;

		case TOK_X11_STAY_BELOW:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_STAY_BELOW);
		break;

		case TOK_X11_ICONIZED:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_ICONIZED);
		break;

		case TOK_X11_FULLSCREEN:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_FULLSCREEN);
		break;

		case TOK_X11_UNMANAGED:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_UNMANAGED);
		break;

		case TOK_X11_NORMAL:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_NORMAL);
		break;

		case TOK_X11_TRANSPARENT:
			enhancer_set_config(Conf, val, Name, 0, FLAG_X_TRANSPARENT);
		break;

		case TOK_SANITISE:
			enhancer_set_config(Conf, val, Name, 0, FLAG_SANITISE);
		break;

		case TOK_DIE_ON_TAINT:
			enhancer_set_config(Conf, val, Name, 0, FLAG_DIE_ON_TAINT);
		break;

		case TOK_DENY_ON_TAINT:
			enhancer_set_config(Conf, val, Name, 0, FLAG_DENY_ON_TAINT);
		break;

		case TOK_LOG:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_LOG, Name, 0, Tempstr);
		break;

		case TOK_SYSLOG:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SYSLOG, Name, 0, Tempstr);
		break;

		case TOK_SYSLOGCRIT:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SYSLOG_CRIT, Name, 0, Tempstr);
		break;

		case TOK_ECHO:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_ECHO, Name, 0, Tempstr);
		break;

		case TOK_SEND:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SEND, Name, 0, Tempstr);
		break;

		case TOK_DEBUG:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_DEBUG, Name, 0, Tempstr);
		break;

		case TOK_EXEC:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_EXEC, Name, 0, Tempstr);
		break;

		case TOK_UNSHARE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_UNSHARE, Name, 0, Tempstr);
		break;

		case TOK_CMOD:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_CMOD, Name, atoi(Tempstr), NULL);
		break;
	
		case TOK_TTL:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_TTL, Name, atoi(Tempstr), NULL);
		break;

		case TOK_SLEEP:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SLEEP, Name, atoi(Tempstr), NULL);
		break;

		case TOK_USLEEP:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_USLEEP, Name, atoi(Tempstr), NULL);
		break;


		case TOK_SEARCHPATH:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SEARCHPATH, Name, 0, Tempstr);
		break;

		case TOK_REDIRECT:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_REDIRECT, Name, 0, Tempstr);
		break;

		case TOK_WRITEJAIL:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_WRITEJAIL, Name, 0, Tempstr);
		break;

		case TOK_FALLBACK:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_FALLBACK, Name, 0, Tempstr);
		break;

		case TOK_PIDFILE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_PIDFILE, Name, 0, Tempstr);
		break;

		case TOK_LOCKFILE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_LOCKFILE, Name, 0, Tempstr);
		break;

		case TOK_COPY_CLONE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_COPY_CLONE, Name, 0, Tempstr);
		break;

		case TOK_LINK_CLONE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_LINK_CLONE, Name, 0, Tempstr);
		break;

		case TOK_CHDIR:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_CHDIR, Name, 0, Tempstr);
		break;

		case TOK_CHROOT:
			enhancer_add_action(Conf, ACT_CHROOT, Name, 0, NULL);
		break;

		case TOK_XTERM_TITLE:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_XTERM_TITLE, Name, 0, Tempstr);
		break;

		case TOK_SETVAR:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SETVAR, Name, 0, Tempstr);
		break;

		case TOK_SETENV:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SETENV, Name, 0, Tempstr);
		break;

		case TOK_GETIP:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_GETHOSTIP, Name, 0, Tempstr);
		break;

		case TOK_SETBASENAME:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_SETBASENAME, Name, 0, Tempstr);
		break;

		case TOK_IPMAP:
			enhancer_add_action(Conf, ACT_IPMAP, Name, 0, NULL);
		break;

		case TOK_QLEN:
			ptr=enhancer_spacetok(ptr, &Tempstr);
			enhancer_add_action(Conf, ACT_REDIRECT, Name, 0, Tempstr);
		break;

		default:
			fprintf(stderr, "WARN: enhancer config: unrecognized token %s\n", Name);
		break;
	}

ptr=enhancer_config_tok(ptr, &Name);
}

destroy(Tempstr);
destroy(Name);

return(ptr);
}


static const char *enhancer_add_config(const char *ConfigStr, const char *Function)
{
int FuncID;
char *Tempstr=NULL;
const char *ptr;
TEnhancerConfig *Conf;

ptr=ConfigStr;
FuncID=enhancer_match_token_from_list(Function, EnhancerFuncNames);
if ((FuncID==-1) && (strcmp(Function, "*")==0) ) FuncID=FUNC_ALL;

if (FuncID > -1) 
{
	Conf=enhancer_create_config_item(FuncID);
	ptr=enhancer_parse_config_item(ptr, Conf, FuncID);
	if (FuncID==FUNC_ONEXIT) enhancer_flags |= ENHANCER_STATE_HAS_ATEXIT;
}

destroy(Tempstr);

return(ptr);
}


//Read in everyting between a program name and } and then pass it to
//enhancer_add_config
static void enhancer_read_prog_config(const char *Config)
{
char *Tempstr=NULL;
const char *ptr, *dividers="#{} \r\n";

ptr=enhancer_config_tok(Config, &Tempstr);
while (ptr)
{
	if (Tempstr && strlen(Tempstr))
	{
		if (strcmp(Tempstr,"{")==0) /* do nothing  */;
		else if (strcmp(Tempstr,"\n")==0) /* do nothing  */;
		else if (strcmp(Tempstr,"#")==0) 
		{
			while ((*ptr !='\n') && (*ptr != '\0')) ptr++;
		}
		else if (strcmp(Tempstr,"}")==0) break;
 		else 
		{
				ptr=enhancer_add_config(ptr, Tempstr);
		}
	}
ptr=enhancer_config_tok(ptr, &Tempstr);
}

destroy(Tempstr);
}







static char *enhancer_read_config(char *RetStr, const char *Dir, const char *FileName)
{
char *Tempstr=NULL;

Tempstr=enhancer_strcpy(Tempstr, Dir);
Tempstr=enhancer_strncat(Tempstr, FileName, 0);
RetStr=enhancer_read_file(RetStr, Tempstr);

destroy(Tempstr);

return(RetStr);
}


static int enhancer_load_prog_config(const char *ProgName)
{
char *SetupData=NULL, *Tempstr=NULL, *Token=NULL;
const char *ptr;
int Found=0;

//first have config files been specified in environment variables?
Tempstr=enhancer_strcpy(Tempstr, getenv("ENHANCER_CONFIG_DIR"));
if (strvalid(Tempstr)) 
{
	SetupData=enhancer_read_config(SetupData, Tempstr, ProgName);
	if (! strvalid(SetupData)) 
	{
		syslog(LOG_WARNING, "ERROR: cant load config from %s/%s", Tempstr, ProgName);
		//as we had an explicitly set config file, quit
		//exit(1);
	}
}


if (! strvalid(SetupData))
{
	Tempstr=enhancer_strcpy(Tempstr, getenv("ENHANCER_CONFIG_FILE"));
	if (strvalid(Tempstr))
	{
	SetupData=enhancer_read_config(SetupData, Tempstr, "");
	if (! strvalid(SetupData)) 
	{
		syslog(LOG_WARNING, "ERROR: cant load config from %s", Tempstr);
		//as we had an explicitly set config file, quit
		//exit(1);
	}
	}
}

if (! strvalid(SetupData))
{
	Tempstr=enhancer_strcpy(Tempstr, getenv("HOME"));
	Tempstr=enhancer_strncat(Tempstr, "/.enhancer/",0);
	SetupData=enhancer_read_config(SetupData, Tempstr, ProgName);
	if (! strvalid(SetupData)) 
	{
		Tempstr=enhancer_strcpy(Tempstr, getenv("HOME"));
		SetupData=enhancer_read_config(SetupData, Tempstr, ".enhancer.conf");
	}
}

if (! strvalid(SetupData))
{
	SetupData=enhancer_read_config(SetupData, "/etc/enhancer.d/", ProgName);
	if (! strvalid(SetupData)) 
	{
		SetupData=enhancer_read_config(SetupData, "/etc/", "enhancer.conf");
	}
}

ptr=SetupData;
while (ptr)
{
	ptr=enhancer_spacetok(ptr, &Token);
	if (Token)	
	{
		if (strcmp(Token,"program")==0)
		{
			ptr=enhancer_spacetok(ptr, &Token);
			if (StrListMatch(ProgName, Token)) 
			{
				enhancer_read_prog_config(ptr);
				Found=1;
			}
		}
		else if (strcmp(Token,"logfile")==0)
		{
			//isspace is crashing for some reason
			while (strchr(" 	", *ptr)) ptr++;
			ptr=enhancer_spacetok(ptr, &Token);
			Tempstr=enhancer_format_str(Tempstr, Token, "", "", "");
			enhancer_log_fd=enhancer_real_open(Tempstr, O_CREAT | O_TRUNC | O_WRONLY, 0600);
		}
	}
}



destroy(SetupData);
destroy(Tempstr);
destroy(Token);

return(Found);
}



void enhancer_load_config()
{
int NoOfFuncs;

for (NoOfFuncs=0; EnhancerFuncNames[NoOfFuncs] !=NULL; NoOfFuncs++) /* do nothing, just counting funcs*/;
FuncConfigs=(TEnhancerFuncConfig *) calloc(NoOfFuncs+1, sizeof(TEnhancerFuncConfig));
//reads a whole file into memory

if (! enhancer_load_prog_config(basename(enhancer_argv[0]))) 
{
	enhancer_load_prog_config("*");
}
}
