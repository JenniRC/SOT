/*
    gcc -c -Wall -Wshadow  client.c && gcc -o client client.o && ./client
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
    
#define fifo    "/tmp/fifo"

int
main(void)
{
    int n, fd;
    char buf[1024];

    /* Open the FIFO for writing. It was created by the server*/
    if ((fd = open(fifo, O_WRONLY)) < 0) {
        perror("open");
        exit(1);
    }

   
    printf("Iniciado cliente ... \n");
    while(1){
    while ((n = read(0, buf, sizeof(buf))) > 0){
        if(strcmp(buf,"bye\n") == 0){
            write(fd, buf, n);
            close(fd);
            exit(0);
        }
    write(fd, buf, n);
    }
}
    exit(0);
}
