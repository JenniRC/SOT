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

/*Types*/

int
static myls(DIR *d){

	d = opendir(".");
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
	printf("%s numero=%i esdir? %d\n", d_name, numero, st.st_mode & S_IFDIR);

	return st.st_mode & S_IFDIR;
}

int
static is_file(char *d_name){
	struct stat st;
	
	int numero;
	numero = stat(d_name, &st);
	printf("%s numero=%i esfil? %d\n", d_name, numero, st.st_mode & S_IFREG);

	return st.st_mode & S_IFREG;
}

int 
main(int argc,char*argv[])
{
	int a= is_file(argv[2]); //como a=0 no es file
	printf("a es =%d\n",a );
    int b = is_dir(argv[2]);// como b!=0 es directorio
	printf("b es =%d\n",b );
	int i=0;
	//myls();
	while (i<argc){
		printf("Arg num %d : %s\n",i,argv[i]);
		i++;
		}
	/*palabra a buscar es argv[1]
		directorios son argv[2],argv[3]...
	*/
	exit(0);
}
