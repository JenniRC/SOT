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
static myls(){

	DIR *d = opendir(".");
	struct dirent *de;

	if(d == NULL) {
		err(1, ".");
	}
	while((de = readdir(d)) != NULL) {
		printf("%s\n", de->d_name);
	}
	
	closedir(d);
	return 1;

}

int
static is_dir(char *d_name){
	struct stat st;
	
	int numero;
	numero = stat(d_name, &st);
	//printf("%s numero=%i esdir? %d\n", d_name, numero, st.st_mode & S_IFDIR);

	return st.st_mode & S_IFDIR;
}

int
static is_file(char *d_name){
	struct stat st;
		
	int numero;
	numero = stat(d_name, &st);
	//printf("%s numero=%i esfil? %d\n", d_name, numero, st.st_mode & S_IFREG);
  //(st.st_mode & S_IFMT) == S_IFREG
	return st.st_mode & S_IFREG;
}

int
main(int argc,char*argv[])
{

	int i=0;
	FILE *myFile;
//	myls();

	if (argc == 2){
		DIR *d = opendir(".");
		struct dirent *de;

		if(d == NULL) {
			err(1, ".");
		}
		while((de = readdir(d)) != NULL) {
			char * pointeur = &de->d_name[0];
			if (is_file(pointeur)== S_IFREG){

				myFile = fopen(de->d_name, "r");
				char firstword [20];
				fscanf(myFile,"%s ",firstword);
					if(strcmp(firstword,argv[1])==0){
						char *buf;
						buf = malloc(sizeof (PATH_MAX));
						getcwd(buf,PATH_MAX);
						printf("%s/%s\n", buf,de->d_name);
					}
			}
		}
		
		closedir(d);
	}
		if (argc > 2){

		DIR *d = opendir(argv[2]);
		myls();
		struct dirent *de;

		if(d == NULL) {
			err(1, argv[2]);
		}
		while((de = readdir(d)) != NULL) {
			char * pointeur = &de->d_name[0];
			if (is_file(pointeur)== S_IFREG){

				myFile = fopen(de->d_name, "r");
				char firstword [20];
				fscanf(myFile,"%s ",firstword);
					if(strcmp(firstword,argv[1])==0){
						char *buf;
						buf = malloc(sizeof (PATH_MAX));
						getcwd(buf,PATH_MAX);
						printf("%s/%s\n", buf,de->d_name);
					}
			}
		}
		closedir(d);
	}
	exit(0);

}
