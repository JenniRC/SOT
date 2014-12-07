/*
	gcc -c -Wall -Wshadow  logger.c && gcc -o logger  logger.o && ./logger
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#define fifo    "/tmp/fifo"
#define timeout 5


char*
date(){    
    
    time_t ttime =time(0);
    char* c_time;
 
    /* Convert to local time format. */
    c_time = ctime(&ttime);
 
    if (c_time != NULL){
    	return (c_time);/* Print to stdout. */
    }
}
static void
hndlr(int no){
	alarm(5);
	fprintf(stderr,"%s",date());
	fprintf(stderr, "time out,no events\n");
}

int
main(int argc,char* argv[]){
	char buf[1024];
	int fd, nr;
	/* Remove any previous FIFO*/
    unlink(fifo);
    
	if(mkfifo(fifo, 0666) < 0){//  rw rw --
		err(1, "mkfifo");
	}else{
		fprintf(stderr,"%s",date());
		fprintf(stderr, "waiting for a client...\n");
	}

	fd = open(fifo, O_RDONLY);
		if(fd < 0) {
			err(1, "open");
		}
		if(fd>0){
			fprintf(stderr,"%s",date());
			fprintf(stderr, "ready to read events...\n");
		}
		
	signal(SIGALRM, hndlr);//para el handler
	alarm(5);
	while(1){
    while ((nr = read(fd, buf, sizeof(buf))) > 0){
 		alarm(5);
		if(strcmp(buf,"end\n") == 0){
			write(2, buf, nr);
			close(fd);
			exit(0);
		}
		fprintf(stderr,"%s",date());
		write(2, buf, nr);
	}
}
exit(0);
}
