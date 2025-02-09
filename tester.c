#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

main()
{
    char Buffer[10];
    char *Tempstr=NULL;
    int fd, i;
    time_t When;

    Tempstr=realloc(Tempstr,101);
    memset(Tempstr, 0, 101);

    strcpy(Tempstr,"Twas Brillig and the slivey tooves\n");
//strcpy(Buffer,"Twas Brillig and the slivey tooves\n");

    When=time(NULL);
    printf("WHEN: %s\n", ctime(&When));
    fd=open("passwd",O_RDONLY);
    read(fd, Tempstr, 100);
    printf("READ: %s\n", Tempstr);
    close(fd);

    /*
    for (i=0; i < 100; i ++)
    {
    	fd=open("hosts", O_RDONLY);
    	sleep(5);
    }
    */

rename("/home/colum/Downloads/test.file", "/home/colum/Downloads/test.done");

    system("noexist ; /bin/echo hello");
   // execl("/bin/diff", "--help", NULL);
   // execl("/bin/echo", "hello", "|", "md5sum", NULL);
}
