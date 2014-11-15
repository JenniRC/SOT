/*
	gcc -c -Wall -Wshadow  apply.c && gcc -o apply  apply.o && ./apply ls
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


#define FORMAT ".txt"

int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}
/*Retorna 0 si no es .txt, retorna 1 sí es .txt*/
int
get_format(char *name){
	int len = 0;
	char *rubbish;

	rubbish = strstr(name, FORMAT);

	if(rubbish)
		len = strlen(rubbish);

	return ((rubbish) && (len == 4));
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
static run(char *cmd, char *argv[],char * afile, int ofd){
	int pid, sts, fd;

	pid = fork();

	switch(pid){
	
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		fd = open(afile, O_RDWR);
		dup2(fd, 0);
		dup2(ofd, 1);
		close(fd);
		execv(cmd, argv);
		//err(1, "exec %s failed", cmd);
	default:
		
		while(wait(&sts) == 1)
			;
		return sts;
	}
}

void
search_in_dir(char* dir,char* argv[],int fd){
	DIR *d = opendir(dir);
	struct dirent *de;
	char *path =malloc(sizeof (PATH_MAX));
	path=built_path(argv[0],"/bin");
	
	if(d == NULL) {
		err(1,"%s" ,dir);
	}
	char *buf =malloc(sizeof (PATH_MAX));
	while((de = readdir(d)) != NULL) {
		buf= built_path(de->d_name,dir);
			if (is_file(buf)){
				if(get_format(de->d_name)==1){
					if(run(path,argv,de->d_name,fd) != 0) {
						path=built_path(argv[0],"/usr/bin");
					
						if(run(path,argv,de->d_name,fd) != 0) {
							path=built_path(argv[0],"/usr/local/bin");
					
								if(run(path,argv,de->d_name,fd) != 0) {
									err(1,"This command does not exist\n");
								}
						}		
					}
				}
			}
	}
	free(buf);
	free(path);
	closedir(d);
}

int
main(int argc,char* argv[]){

if (argc == 1){
	printf("Not enough args\n");
	exit(0);
}
	int i;
for (i = 0; i < argc-1; i++)
{
	argv[i]=argv[i+1];
}
argv[argc-1]=NULL;
int fd;
	if (argc > 1){
		fd = creat("apply.output", 0660); //  rw rw --
			if (fd < 0) {
				err(1, "apply.output");
			}
	}

	search_in_dir(".",argv,fd);

	exit(0);
}
