#include "net.h"
#include <netdb.h>

int net_get_salen(struct sockaddr *sa)
{
    int len=0;

    if (! sa) return(0);
    if (sa->sa_family==AF_UNIX) len=sizeof(struct sockaddr_un);
    else len=sizeof(struct sockaddr_in);

    return(len);
}


struct sockaddr *net_sockaddr_from_url(const char *URL)
{
    struct sockaddr *sa=NULL;
    struct sockaddr_un *unix_sa;
    struct sockaddr_in *ip_sa;
    const char *ptr, *p_tmp;
    char *Host=NULL, *Token=NULL;
    int Port;

//get the protocol (e.g. tcp:)
    ptr=enhancer_strtok(URL, ":", &Token);

//some protocols (e.g. socks) can include 'username:password@' after the protocol
//so we keen to skip past that
    p_tmp=strchr(ptr, '@');
    if (p_tmp) ptr=p_tmp;


    if (strcmp(Token, "unix")==0)
    {
        if (*ptr==':') ptr++;
        sa=(struct sockaddr *) calloc(1, sizeof(struct sockaddr_un));
        unix_sa=(struct sockaddr_un *) sa;
        unix_sa->sun_family=AF_UNIX;
        strncpy(unix_sa->sun_path, ptr, sizeof(unix_sa->sun_path) -1);
    }
    else
    {
        ptr=enhancer_strtok(ptr, ":", &Host);
        ptr=enhancer_strtok(ptr, ":", &Token);
        Port=atoi(Token);

        sa=(struct sockaddr *) calloc(1, sizeof(struct sockaddr_in));
        ip_sa=(struct sockaddr_in *) sa;
        if (strcmp(Token, "tcp6")==0)
        {
            ip_sa->sin_family=AF_INET6;
            enhancer_strrep(Host, '.', ':');
        }
        else ip_sa->sin_family=AF_INET;
        ip_sa->sin_port=htons(Port);
        inet_pton(ip_sa->sin_family, Host, &(ip_sa->sin_addr));
    }

    destroy(Token);
    destroy(Host);
    return(sa);
}


int net_connect(const char *URL)
{
    int fd=-1, salen, result;
    struct sockaddr *sa;

    sa=net_sockaddr_from_url(URL);
    salen=net_get_salen(sa);
    if (sa->sa_family==AF_UNIX) fd=socket(AF_UNIX, SOCK_STREAM, 0);
    else fd=socket(sa->sa_family, SOCK_STREAM, IPPROTO_TCP);

    result=enhancer_real_connect(fd, (struct sockaddr *) sa, salen);
    if (result != 0)
    {
        close(fd);
        fd=-1;
    }
    free(sa);

    return(fd);
}

void net_send(const char *URL, const char *Str)
{
    int fd;

    fd=net_connect(URL);
    write(fd, Str, strlen(Str));
    close(fd);
}


char *net_lookuphost(char *RetStr, const char *Hostname)
{
    struct addrinfo *ret=NULL, *next=NULL;
    char *Tempstr=NULL;

    getaddrinfo(Hostname, NULL, NULL, &ret);
    if (ret)
    {
        Tempstr=calloc(1024, sizeof(char));
        next=ret;
        while (next)
        {
            inet_ntop(next->ai_family, &( ((struct sockaddr_in *) next->ai_addr)->sin_addr), Tempstr, next->ai_addrlen);

            RetStr=enhancer_strncat(RetStr, Tempstr, 0);
            RetStr=enhancer_strncat(RetStr, ",", 1);

            next=next->ai_next;
        }
    }


    if (ret) freeaddrinfo(ret);
    destroy(Tempstr);

    return(RetStr);
}
