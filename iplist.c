#include "common.h"
#include "iplist.h"
#include "vars.h"

TVarList *IPList=NULL;
int ip_maps=0;

void enhancer_iplist_add(const char *ip_addr, const char *name)
{
if (! IPList) IPList=enhancer_varlist_create();
enhancer_setvarlist(IPList, ip_addr, name);
}

const char *enhancer_iplist_get(const char *ip_addr)
{
if (! IPList) return("");
return(enhancer_getvarlist(IPList, ip_addr));
}


char *enhancer_map_ip(const char *RetStr, const char *name)
{
uint32_t ip_nbo;

ip_maps++;

RetStr=realloc(RetStr, 20);
ip_nbo=htonl(ip_maps);
inet_ntop(AF_INET, &ip_nbo, RetStr, 20);

fprintf(stderr, "ipmap: %d %s\n", ip_maps, RetStr);
enhancer_iplist_add(RetStr, name);

return(RetStr);
}
