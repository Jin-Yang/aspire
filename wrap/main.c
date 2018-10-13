#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
        if (putenv((char *)"LD_PRELOAD=./libawrap.so")) {
                printf("add env LD_PRELOAD failed, %s\n", strerror(errno));
                return -1;
        }

        printf("entering main process---\n");

        //execl("/bin/ls", "ls", "-l", NULL);
        execl("/bin/bash", "bash", "/tmp/foobar/testwrap.sh", NULL);

        //char *argv[] = {"ls","-l",NULL};
        //execvp("ls", argv);

        //char *argv[] = {"bash", "/tmp/foobar/testwrap.sh", NULL};
        //execvp("bash", argv);

        //const char *envp[] = {"AA=11", "BB=22", NULL};
        //execle("/bin/bash", "bash", "-c", "echo $AA", NULL, envp);

        //execl("/bin/ls", "ls", "-l", NULL);

        execlp("ls", "ls", "-l", NULL);

        printf("exiting main process ----\n");
        return 0;
}

