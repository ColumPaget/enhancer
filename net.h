#ifndef ENHANCER_NET_H
#define ENHANCER_NET_H

#include "common.h"

int net_get_salen(struct sockaddr *sa);
struct sockaddr *net_sockaddr_from_url(const char *URL);
char *net_lookuphost(char *RetStr, const char *Hostname);
int net_connect(const char *URL);
void net_send(const char *URL, const char *Str);

#endif

