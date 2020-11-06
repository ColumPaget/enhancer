#ifndef ENHANCER_SOCKINFO_H
#define ENHANCER_SOCKINFO_H

#include "common.h"

typedef struct
{
int socket;
const struct sockaddr *sa;
socklen_t salen;
int family;
char *address;
int port;
char *redirect;
int ttl;
pid_t pid;
uid_t uid;
gid_t gid;
} TSockInfo;


const char *sockinfo_family_name(TSockInfo *SockInfo);
TSockInfo *enhancer_createSockInfo(int FuncID, int socket, const struct sockaddr *sa, int salen);
void enhancer_destroySockInfo(TSockInfo *SI);

#endif
