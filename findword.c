/*
	gcc -c -Wall -Wshadow findword.c
	gcc -o 8.findword findword.o
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h> /* PATH_MAX */

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
search_in_dir(char* dir,char* argv){
	DIR *d = opendir(dir);
	struct dirent *de;
	FILE *myFile;

	//if(d == NULL) {
	//	err(1, dir);
	//}
	char *buf =malloc(sizeof (PATH_MAX));
	while((de = readdir(d)) != NULL) {
			buf= built_path(de->d_name,dir);
			if (is_file(buf)){
				myFile = fopen(buf, "r");
				char firstword [20];
				fscanf(myFile,"%s ",firstword);
					if(strcmp(firstword,argv)==0){
						printf("%s\n", buf);
					}
			}else{
				if(is_dir(buf)){
					if((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..")!=0)){
						search_in_dir(buf,argv);
					}
				}else{
					printf("\t%s I does not a valid file or directory\n", buf);
				}
			}
	}
	free(buf);
	closedir(d);
}

int
main(int argc,char*argv[])
{
	if(argc==2){
		search_in_dir(".",argv[1]);
	}
	int i;
	if (argc > 2){
		for (i = 1; i < argc - 1; i++){
			search_in_dir(argv[i+1],argv[1]);
			//printf("entro %d veces con %s \n", i,argv[i+1]);
		}	
	}
	exit(0);
}
