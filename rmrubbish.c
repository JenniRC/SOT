/*
	gcc -c -Wall -Wshadow  rmrubbish.c
	gcc -o 8. rmrubbish  rmrubbish.o
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

#define FORMAT ".rubbish"

int
static is_dir(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFDIR);
}

int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}
/*Retorna 0 si no es .rubbish, retorna 1 sí es .rubbish*/
int
get_format(char *name){
	int len = 0;
	char *rubbish;

	rubbish = strstr(name, FORMAT);

	if(rubbish)
		len = strlen(rubbish);

	return ((rubbish) && (len == 8));
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

void
search_in_dir(char* dir, int pid){
	DIR *d = opendir(dir);
	struct dirent *de;
	int counter=0;
	if(d == NULL) {
		err(1,"%s" ,dir);
	}
	char *buf =malloc(sizeof (PATH_MAX));
	while((de = readdir(d)) != NULL) {
			buf= built_path(de->d_name,dir);
				if (is_file(buf)){
					if(get_format(de->d_name)==1){
						if (unlink(buf)<0){
							printf("%d : error removing %s\n", pid ,de->d_name);
						}else {
							counter++;
							printf("%d : %s ok \n",pid ,de->d_name );
						}
					}
				}else{
					if(is_dir(buf)){
						if((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..")!=0)){
							search_in_dir(buf,pid);
						}
					}else{
						printf("\t%s It in not a valid file or directory\n", buf);
				}
				if ((counter==0) && (strcmp(de->d_name, ".") == 0))
					printf("%d :  no files to remove in :%s\n", pid ,dir);
			}
	}
	free(buf);
	closedir(d);
}

int
main(int argc,char*argv[])
{
	int i,sts,pid,np;
	np=0;//numero de procesos creados//
	if (argc >= 1){
		for (i = 1; i < argc ; i++){
			printf("%d\n", i);
			pid=fork();
			switch(pid){
				case -1:
					err(1, "fork failed");
					break;
				case 0: // esto es el hijo
					search_in_dir(argv[i], getpid());
					break;
				default: 
					while(wait(&sts) != pid){
						if (sts == 1){
							printf("%d\n",sts );
							np++;//Sumo porque ha ido bien todo//
						}
					}
			}	
		}
	if (np == (argc))
		err(1,"All processes were successful ");
	if (np!=(argc))	
		err(1,"%d processes failed dsvfx",(argc-np));
	}
	exit(0);
}
