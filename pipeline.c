/*
	gcc -c -Wall -Wshadow  pipeline.c && gcc -o pipeline  pipeline.o && ./pipeline fo < cheri.gz
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h> /* PATH_MAX */


int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}

char *
built_path(char *s1, char *path){
	char * aux;
	int n1, n2;

	n1 = strlen(s1);
	n2 = strlen(path);
	aux = malloc (n1+n2+2);
	sprintf(aux, "%s/%s", path, s1);
	return aux;
}

int
my_pipe_file(int fi,char** cmd,int fo,int nc){ 
	int nr;
	// Parent process creates all needed pipes at the beginning
	// 1 Pipe for 2 command (number of command - 1)
	int fd[2*(nc-1)];
	int i;
	for( i = 0; i < (nc-1); i++ ){
    	if( pipe(fd + i*2) < 0 ){
		printf("Pipe creation failed\n");
		return 1;    
		}
	}

	// Loop for all commands to execute -
	nr = -1;
	int j=0;
	for(j=0; j < nc; j++){

		// create the child
		int pid;
		if((pid = fork()) < 0)
		{
			printf("fork failed\n");
			return 2;
		}

		if (pid == 0)	
		{
			if(j == 0){
				// First command
				dup2(fi,0); // connect fi to stdin
			}
			if(j == (nc-1)){
				// Last command
				dup2(fo,1); // connect fo to stdout
			}
			if(j!=0){
				// NOT First Command
				dup2(fd[(j-1)*2],0); // get imput from previous command
			}
			if(j != (nc-1)){
				// NOT last command to process
				dup2(fd[j*2+1],1); // connect output to next command
			}
			// close pipes;
			int q;
            for(q = 0; q < 2*(nc-1); q++){
                    close(fd[q]);
            }

			// execute command
			int k = j+5;
			execlp(cmd[k], cmd[k], NULL);
			printf("Execute command failed"); // if execlp returns, we get an error
		}
	}
	return nr;
} 

int
my_pipe_no_file(char** cmd,int nc){ 
	int nr;

	// Parent process creates all needed pipes at the beginning
	// 1 Pipe for 2 command (number of command - 1)
	int fd[2*(nc-1)];
	int i;
	for( i = 0; i < (nc-1); i++ ){
    	if( pipe(fd + i*2) < 0 ){
		printf("Pipe creation failed\n");
		return 1;    
		}
	}

	// Loop for all commands to execute -
	nr = -1;
	int j=0;
	for(j=0; j < nc; j++){

		// create the child
		int pid;
		if((pid = fork()) < 0)
		{
			printf("fork failed\n");
			return 2;
		}

		if (pid == 0)	
		{
			if(j!=0){
				// NOT First Command
				dup2(fd[(j-1)*2],0); // get imput from previous command
			}
			if(j != (nc-1)){
				// NOT last command to process
				dup2(fd[j*2+1],1); // connect output to next command
			}
			// close pipes;
			int q;
            for(q = 0; q < 2*(nc-1); q++){
                    close(fd[q]);
            }

			// execute command
			int k = j+1;
			execlp(cmd[k], cmd[k], NULL);
			printf("Execute command failed\n"); // if execlp returns, we get an error
		}
	}
	return nr;
} 

/*Si existe el fichero retorna su fd, sino retorna -1*/
int 
test_file(char* dir,char* argv){
	DIR *d = opendir(dir);
	struct dirent *de;
	int fd;

	if(d == NULL) {
		err(1,"%s" ,dir);
	}
	char *buf =malloc(sizeof (PATH_MAX));
	fd =-1;
	while((de = readdir(d)) != NULL) {
		buf = built_path(de->d_name,dir);
			if (is_file(buf) && (strcmp(de->d_name,argv)==0)) {
				fd = open(buf, O_RDWR);
				break;
			}
	}
	return fd;
free(buf);
closedir(d);

}

int
main(int argc,char* argv[]){

if (argc == 1){
	printf("Not enough args\n");
}

if (argc > 1){
    int nc;
	if ((test_file(".",argv[2])) == -1 ){ //Si != -1 file existe
		nc = (argc-1); 
		my_pipe_no_file(argv,nc);
	}	
	
	else if ((test_file(".",argv[4])) == -1 ){ //Si != -1 file existe 
		nc = (argc-1); 
		my_pipe_no_file(argv,nc);
	}
	else{
	// Hasta aquí hemos comprobado que existen los ficheros
	int fi = test_file(".",argv[2]);
	int fo = test_file(".",argv[4]);
	nc = (argc-5); // number of command to execute 
	my_pipe_file(fi,argv,fo,nc);
	
	//Contamos el numero de comando a ejecutar = o
	//el primero con fi, el ultimo con fo en salida
	close(fi);
	close(fo);
	}
}
	exit(0);

}
