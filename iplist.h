#ifndef ENHANCER_IPLIST_H
#define ENHANCER_IPLIST_H

void enhancer_iplist_add(const char *ip_addr, const char *name);
const char *enhancer_iplist_get(const char *ip_addr);
char *enhancer_map_ip(char *RetStr, const char *name);

#endif

