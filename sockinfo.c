#include "sockinfo.h"
#include "config.h"
#include "net.h"


const char *sockinfo_family_name(TSockInfo *SockInfo)
{
    const char *sock_family="";
    int size, val;

    switch (SockInfo->family)
    {
    case  AF_PACKET :
        sock_family="raw";
        break;

    case  AF_UNSPEC :
    case  AF_UNIX   :
        sock_family="unix";
        break;

    case  AF_NETLINK  :
        sock_family="netlink";
        break;

    case  AF_INET   :
        if (SockInfo->socket > -1)
        {
            size=sizeof(val);
            getsockopt(SockInfo->socket, SOL_SOCKET, SO_TYPE, &val, &size);
            if (val==SOCK_STREAM) sock_family="tcp";
            else if (val==SOCK_DGRAM) sock_family="udp";
            else sock_family="ip4";
        }
        else sock_family="ip4";
        break;

    case  AF_INET6  :
        if (SockInfo->socket > -1)
        {
            size=sizeof(val);
            getsockopt(SockInfo->socket, SOL_SOCKET, SO_TYPE, &val, &size);
            if (val==SOCK_STREAM) sock_family="tcp6";
            else if (val==SOCK_DGRAM) sock_family="udp6";
            else sock_family="ip6";
        }
        else sock_family="ip6";
        break;

    case  AF_IPX  :
        sock_family="ipx";
        break;
    case  AF_X25  :
        sock_family="x25";
        break;
    case  AF_IRDA  :
        sock_family="irda";
        break;
    case  AF_BLUETOOTH  :
        sock_family="bluetooth";
        break;

    default:
        sock_family="other";
        break;

        /*
        case  AF_FILE   :
        case  AF_APPLETALK  :
        case  AF_NETROM :
        case  AF_BRIDGE :
        case  AF_ATMPVC :
        case  AF_X25    :
        case  AF_ROSE   :
        case  AF_DECnet :
        case  AF_NETBEUI  :
        case  AF_SECURITY :
        case  AF_KEY    :
        case  AF_ASH    :
        case  AF_ECONET :
        case  AF_ATMSVC :
        case  AF_RDS    :
        case  AF_SNA    :
        case  AF_PPPOX  :
        case  AF_WANPIPE  :
        case  AF_LLC    :
        case  AF_CAN    :
        case  AF_TIPC   :
        case  AF_IUCV   :
        case  AF_RXRPC  :
        case  AF_ISDN   :
        case  AF_PHONET :
        case  AF_IEEE802154 :
        case  AF_CAIF   :
        case  AF_ALG    :
        case  AF_NFC    :
        case  AF_VSOCK  :
        */
    }

    return(sock_family);
}


TSockInfo *enhancer_createSockInfo(int FuncID, int socket, const struct sockaddr *sa, int salen)
{
    struct sockaddr_in6 *sa_in6;
    struct sockaddr_in *sa_in;
    struct sockaddr_un *sa_un;
    struct ucred SockCreds;
    TSockInfo *sockinfo;

    if (! sa) return(NULL);

    sockinfo=(TSockInfo *) calloc(1, sizeof(TSockInfo));
    sockinfo->socket=socket;
    sockinfo->sa=sa;
    sockinfo->salen=salen;
    sockinfo->pid=-1;
    sockinfo->uid=-1;
    sockinfo->gid=-1;
    if (sockinfo->sa) sockinfo->family=sockinfo->sa->sa_family;

    switch (sockinfo->family)
    {
    case AF_INET:
        sa_in=(struct sockaddr_in *) sa;
        sockinfo->port=ntohs(sa_in->sin_port);
        sockinfo->address=(char *) realloc(sockinfo->address, INET6_ADDRSTRLEN+1);
        inet_ntop(AF_INET,&sa_in->sin_addr, sockinfo->address,INET_ADDRSTRLEN);
        break;

    case AF_INET6:
        sa_in6=(struct sockaddr_in6 *) sa;
        sockinfo->port=ntohs(sa_in6->sin6_port);
        sockinfo->address=(char *) realloc(sockinfo->address, INET6_ADDRSTRLEN+1);
        inet_ntop(AF_INET6,&sa_in6->sin6_addr, sockinfo->address,INET6_ADDRSTRLEN);
        enhancer_strrep(sockinfo->address, ':', '.');
        break;

    case AF_UNIX:
        if (sockinfo->sa &&
                ((FuncID == FUNC_CONNECT) || (FuncID == FUNC_BIND) || (FuncID == FUNC_ACCEPT) )
           )
        {
            sa_un=(struct sockaddr_un *) sockinfo->sa;
            if (strvalid(sa_un->sun_path)) sockinfo->address=enhancer_strcpy(sockinfo->address, sa_un->sun_path);
            if (sockinfo->socket > -1)
            {
                salen=sizeof(struct ucred);
                getsockopt(sockinfo->socket, SOL_SOCKET, SO_PEERCRED, & SockCreds, &salen);
                sockinfo->pid=SockCreds.pid;
                sockinfo->uid=SockCreds.uid;
                sockinfo->gid=SockCreds.gid;
            }
        }
        break;
    }

    return(sockinfo);
}


void enhancer_destroySockInfo(TSockInfo *SI)
{
    if (SI)
    {
        destroy(SI->address);
        destroy(SI->redirect);
        free(SI);
    }
}
