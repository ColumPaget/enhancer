#include "socks.h"
#include "iplist.h"
#include "net.h"


static int socks_request(int fd, const char *Auth, const char *DestHost, int DestPort)
{
struct sockaddr_in sa;
char *Tempstr=NULL;
char *ptr;
int val;
int UseHostname=FALSE;

if (fd == -1) return(0);

if (! inet_aton(DestHost, &sa.sin_addr)) 
{
	inet_aton("0.0.0.1", &sa.sin_addr);
	UseHostname=TRUE;
}

Tempstr=calloc(strlen(Auth) + strlen(DestHost) + 100, sizeof(char));
ptr=Tempstr;
* (uint8_t *) ptr= 0x04; //socks version
ptr++;
* (uint8_t *) ptr= 0x01; // cmd=1, establish tcp connection
ptr++;
* (uint16_t *) ptr=htons((uint16_t) DestPort); //destination Port
ptr+=2;

val=sa.sin_addr.s_addr;
memcpy(ptr, &val, 4);
ptr+=4;

val=strlen(Auth);
if (val > 0)
{
	memcpy(ptr, Auth, val);
	ptr+=val;
}

*ptr='\0';
ptr++;

if (UseHostname)
{
	val=strlen(DestHost);
	memcpy(ptr, DestHost, val);
	ptr+=val;
	*ptr='\0';
	ptr++;
}

enhancer_real_write(fd, Tempstr, ptr-Tempstr);
destroy(Tempstr);

return(1);
}




static int socks_reply(int fd)
{
char *Tempstr=NULL;
int retval=0;

Tempstr=calloc(20, sizeof(char));
if (read(fd, Tempstr, 8) ==8)
{
	if (Tempstr[1]==0x5A) retval=1;
}

destroy(Tempstr);
return (retval);
}



int socks_connect(const char *ProxyURL, const char *DestHost, int DestPort)
{
char *Auth=NULL;
const char *ptr;
int fd;

if (! DestHost) return(-1);
Auth=enhancer_strcpy(Auth, "");
if (strchr(ProxyURL, '@'))
{
	ptr=enhancer_strtok(ProxyURL, ":", &Auth);
	ptr=enhancer_strtok(ptr, "@", &Auth);
}

fd=net_connect(ProxyURL);
if (fd > -1)
{
	ptr=enhancer_iplist_get(DestHost);
	if (! strvalid(ptr)) ptr=DestHost;
	
	if (
				(! socks_request(fd, Auth, ptr, DestPort)) ||
				(! socks_reply(fd))
		 )
	{
		close(fd);
		fd=-1;
	}
}

destroy(Auth);
return(fd);
}
