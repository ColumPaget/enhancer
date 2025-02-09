#include "common.h"
#include "config.h"
#include "actions.h"
#include "net.h"
#include "socks.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "sockinfo.h"

int (*enhancer_real_socket)(int domain, int type, int protocol)=NULL;
int (*enhancer_real_connect)(int socket, const struct sockaddr *address, socklen_t address_len)=NULL;
int (*enhancer_real_bind)(int socket, const struct sockaddr *address, socklen_t address_len)=NULL;
int (*enhancer_real_listen)(int sockfd, int qlen)=NULL;
int (*enhancer_real_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen)=NULL;
int (*enhancer_real_getaddrinfo)(const char *name, const char *service, const struct addrinfo *hints, struct addrinfo **res)=NULL;
struct hostent *(*enhancer_real_gethostbyname)(const char *name)=NULL;



/*
int socket(int family, int type, int protocol)
{
int Flags;
TSockInfo sockinfo;

memset(&sockinfo, 0, sizeof(TSockInfo));
sockinfo.socket=-1;
sockinfo.family=family;

Flags=enhancer_process_socket_function(FUNC_SOCKET, "socket", &sockinfo);

if (Flags & FLAG_DENY) return(-1);


return(enhancer_real_socket(family, type, protocol));
}
*/


int connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    int Flags=0, result=-1, val;
    int local_errno;
    TSockInfo *sockinfo;

    sockinfo=enhancer_createSockInfo(FUNC_CONNECT, socket, address, address_len);
    Flags=enhancer_checkconfig_socket_function(FUNC_CONNECT, "connect", sockinfo);

    if (! (Flags & FLAG_DENY))
    {
#ifdef SO_DONTROUTE
        if (Flags & FLAG_NET_DONTROUTE)
        {
            val= 1;
            setsockopt(socket, SOL_SOCKET, SO_DONTROUTE, &val, sizeof(val));
        }
#endif

#ifdef TCP_NODELAY
        if (Flags & FLAG_TCP_NODELAY)
        {
            val= 1;
            setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
        }
#endif

#ifdef TCP_QUICKACK
        if (Flags & FLAG_TCP_QACK)
        {
            val= 1;
            setsockopt(socket, IPPROTO_TCP, TCP_QUICKACK, &val, sizeof(val));
        }
#endif


        if (sockinfo->ttl > 0)
        {
            val=sockinfo->ttl & 0xFF;
            setsockopt(socket, IPPROTO_IP, IP_TTL, &val, sizeof(val));
        }

        if (Flags & FLAG_REDIRECT)
        {
            close(socket);
            if (strncmp(sockinfo->redirect, "socks:", 6)==0) socket=socks_connect(sockinfo->redirect, sockinfo->address, sockinfo->port);
            else socket=net_connect(sockinfo->redirect);
            if (socket==-1) result=-1;
            else result=0;
        }
        else result=enhancer_real_connect(socket, address, address_len);
        local_errno=errno;

        //did 'connect' work?
        if (result != -1)
        {
#ifdef SO_KEEPALIVE
            if (Flags & FLAG_NET_KEEPALIVE)
            {
                val= 1;
                setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
            }
#endif
        }
    }
    else local_errno=ECONNREFUSED;

    enhancer_destroySockInfo(sockinfo);

    errno=local_errno;
    return(result);
}



int bind(int fd, const struct sockaddr *address, socklen_t address_len)
{
    int Flags, result=-1, val;
    TSockInfo *sockinfo;
    struct sockaddr *redirect_sa;

    sockinfo=enhancer_createSockInfo(FUNC_BIND, fd, address, address_len);
    Flags=enhancer_checkconfig_socket_function(FUNC_BIND, "bind", sockinfo);

    if (Flags & FLAG_PRETEND) result=0;

    if (! (Flags & (FLAG_DENY | FLAG_PRETEND)) )
    {
#ifdef SO_REUSEPORT
        if (Flags & FLAG_NET_REUSEPORT)
        {
            val= 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
        }
#endif

#ifdef SO_FREEBIND
        if (Flags & FLAG_NET_FREEBIND)
        {
            val= 1;
            setsockopt(fd, IPPROTO_IP, IP_FREEBIND, &val, sizeof(val));
        }
#endif

        if (Flags & FLAG_REDIRECT)
        {
            redirect_sa=net_sockaddr_from_url(sockinfo->redirect);
            if (redirect_sa)
            {
                close(fd);
                if (redirect_sa->sa_family==AF_UNIX) fd=socket(AF_UNIX, SOCK_STREAM, 0);
                else fd=socket(redirect_sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
                address_len=net_get_salen(redirect_sa);
                result=enhancer_real_bind(fd, redirect_sa, address_len);
                free(redirect_sa);
            }
        }
        else result=enhancer_real_bind(fd, address, address_len);

#ifdef SO_DONTROUTE
        if (Flags & FLAG_NET_DONTROUTE)
        {
            val= 1;
            setsockopt(fd, SOL_SOCKET, SO_DONTROUTE, &val, sizeof(val));
        }
#endif

#ifdef TCP_NODELAY
        if (Flags & FLAG_TCP_NODELAY)
        {
            val= 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
        }
#endif

#ifdef TCP_QUICKACK
        if (Flags & FLAG_TCP_QACK)
        {
            val= 1;
            setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &val, sizeof(val));
        }
#endif

    }

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("bind");


    enhancer_destroySockInfo(sockinfo);

    return(result);
}



int listen(int socket, int qlen)
{
    char *Redirect=NULL;
    int result;
    int Flags;

    Flags=enhancer_checkconfig_with_redirect(FUNC_LISTEN, "listen", "", "", qlen, 0, &Redirect);
    if (strvalid(Redirect)) qlen=atoi(Redirect);

    if (enhancer_real_listen) result=enhancer_real_listen(socket, qlen);

    if ((result !=0) && (Flags & FLAG_FAILDIE)) enhancer_fail_die("listen");

    destroy(Redirect);
    return(result);
}


int accept(int socket, struct sockaddr *address, socklen_t *address_len)
{
    int Flags, fd=-1, val;
    TSockInfo *sockinfo;

    fd=enhancer_real_accept(socket, address, address_len);

    sockinfo=enhancer_createSockInfo(FUNC_ACCEPT, fd, address, *address_len);
    Flags=enhancer_checkconfig_socket_function(FUNC_ACCEPT, "accept", sockinfo);

    if (Flags & FLAG_DENY)
    {
        enhancer_destroySockInfo(sockinfo);
        close(fd);
        return(-1);
    }

#ifdef SO_DONTROUTE
    if (Flags & FLAG_NET_DONTROUTE)
    {
        val= 1;
        setsockopt(socket, SOL_SOCKET, SO_DONTROUTE, &val, sizeof(val));
    }
#endif

#ifdef TCP_NODELAY
    if (Flags & FLAG_TCP_NODELAY)
    {
        val= 1;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    }
#endif

#ifdef TCP_QUICKACK
    if (Flags & FLAG_TCP_QACK)
    {
        val= 1;
        setsockopt(socket, IPPROTO_TCP, TCP_QUICKACK, &val, sizeof(val));
    }
#endif


    enhancer_destroySockInfo(sockinfo);
    return(fd);
}



struct hostent *gethostbyname(const char *name)
{
    int Flags;
    char *Redirect=NULL;
    struct hostent *hostent;

    Redirect=enhancer_strcpy(Redirect, name);
    Flags=enhancer_checkconfig_with_redirect(FUNC_GETHOSTIP, "gethostbyname", name, "", 0, 0, &Redirect);
    hostent=enhancer_real_gethostbyname(Redirect);

    destroy(Redirect);

    return(hostent);
}


int getaddrinfo(const char *name, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    int result, Flags;
    char *Redirect=NULL;

    Redirect=enhancer_strcpy(Redirect, name);
    Flags=enhancer_checkconfig_with_redirect(FUNC_GETHOSTIP, "getaddrinfo", name, "", 0, 0, &Redirect);

    result=enhancer_real_getaddrinfo(Redirect, service, hints, res);

    destroy(Redirect);
    return(result);
}

void enhancer_socket_hooks()
{
    if (! enhancer_real_socket) enhancer_real_socket = dlsym(RTLD_NEXT, "socket");
    if (! enhancer_real_connect) enhancer_real_connect = dlsym(RTLD_NEXT, "connect");
    if (! enhancer_real_bind) enhancer_real_bind = dlsym(RTLD_NEXT, "bind");
    if (! enhancer_real_listen) enhancer_real_listen = dlsym(RTLD_NEXT, "listen");
    if (! enhancer_real_accept) enhancer_real_accept = dlsym(RTLD_NEXT, "accept");
    if (! enhancer_real_gethostbyname) enhancer_real_gethostbyname = dlsym(RTLD_NEXT, "gethostbyname");
    if (! enhancer_real_getaddrinfo) enhancer_real_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
}
