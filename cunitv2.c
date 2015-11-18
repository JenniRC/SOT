/* 
	gcc -c -Wall -Wshadow  cunit.c && gcc -o cunit cunit.o && ./cunit
	gcc -c -Wall -Wshadow  cunit.c && gcc -o cunit cunit.o && ./cunit -c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

#define MAX 256
#define FORMAT ".tst"
#define OUTPUT ".out"
#define OKTEST ".ok"
/* Estructura para los comandos */	
typedef struct Command Command ; 
struct Command {
    char * command ;
    char * cmdarg[10] ; 
}; 
/*Cambia el \n por \0 (final de linea / final de string)*/
void
quit_end(char * str){
    int lon = strlen(str);
    if(str[lon-1]=='\n'){
        str[lon-1]='\0';        
    }
}
int
static is_file(char *d_name){
	struct stat st;
	stat(d_name, &st);
	return ((st.st_mode & S_IFMT)==S_IFREG);
}
/*Construye el nombre del fichero.out, para guardar la salida de la ejecucción*/
char *
built_newname(char *s1){
	char * aux;
	int n1;
	char *token;
    char *saveptr1;
	n1 = strlen(OUTPUT);
    token = strtok_r(s1,".",&saveptr1);
    aux = malloc (strlen(token)+n1+2);	
	sprintf(aux,"%s%s",token,OUTPUT);

	return aux;
}
int
flag(char *line,char sep){
	int flag=0;
	char *flg;
	if ((flg = strchr(line,sep))) {
		flag=1;
	
			if(sep=='&'){
				flg[0]='\0';
			}
	}

	return flag;
}
/*Retorna 0 si no es .tst, retorna 1 sí es .tst !.out !.ok*/
int
get_format(char *name,int f){
	int len = 0;
	char *txt;
	int t;
	switch(f){

	case 0:
		txt = strstr(name, FORMAT);
		if(txt)
		len = strlen(txt);
		t = ((txt) && (len == 4));
		break;
	case 1:
		txt = strstr(name, OUTPUT);
		if(txt)
		len = strlen(txt);
		t = ((txt) && (len == 4));
		break;
	case 2:
		txt = strstr(name, OKTEST);
		if(txt)
		len = strlen(txt);
		t = ((txt) && (len == 3));
	}
	return t;
}
int 
createFile(char *s1){
	
	int fd = creat(s1, 0660);
	if (fd < 0)		err(1, "Impossible create a new File");
	
	return fd;
}
/*Para cambiar de directorio -este es el built in que había que hacer-. Si tengo arg en
	el comando cd, voy a ese path, si no, voy a $HOME */
void
mycd(Command *cmd){
	if (cmd->cmdarg[1] == NULL){
		chdir(getenv("HOME"));
		printf("CD A HOME\n");

	}else{
		if(chdir(cmd->cmdarg[1])<0) fprintf(stderr,"error cd $%s \n",cmd->cmdarg[1]); 
			printf("CD A %s\n",cmd->cmdarg[1]);
	}
}
/* Construye un path de una ruta para buscar con access si esta el ejecutable*/
char *
built_path(char *s1, char *path){
	char * aux;
	int n1, n2;

	n1 = strlen(s1);
	n2 = strlen(path);
	aux = malloc (n1+n2+1);
	sprintf(aux, "%s/%s", path, s1);
	return aux;
	free(aux);
}
/*Tokeniza una cadena en struct de comandos*/
Command*
mytoken(char *cadena,Command *cmd,char *sep){
    char *token;
    char *saveptr1;
    int j=0;
    char *aux=(char*) malloc(1024*sizeof(char));
    strcpy(aux,cadena);
    token = strtok_r(aux,sep,&saveptr1);
    cmd->command = token;
    while(token!=NULL){
    	cmd->cmdarg[j]=token;
    	quit_end(cmd->cmdarg[j]);/////////////
    	j++;
       	token = strtok_r(saveptr1,sep,&saveptr1);/*si es " " al final, lo cuenta como 1,asi que cuidado ahí*/
   	}
   	cmd->cmdarg[j]=NULL;
    return cmd;
}
/*Para hacer la sustitucion en variables de entorno,devuelvo otra linea con el nombre de las variables cambiadas*/
char *
to_sust(char *cadena){
	char *line=(char*) malloc(1024*sizeof(char));	
   	strcpy(line, cadena);
   	int i=0;
   	char *line_aux=(char*) malloc(1024*sizeof(char));	
   	Command* var=(Command*)malloc(512*sizeof(Command));
	var = mytoken(line,var," ");
	while(var->cmdarg[i] != NULL){
		if ((strchr(var->cmdarg[i],'$'))!=NULL){
			Command* aux=(Command*)malloc(512*sizeof(Command));
			aux = mytoken(var->cmdarg[i],aux,"$");
			quit_end(aux->cmdarg[0]);
        	if(!getenv(aux->cmdarg[0])){
            	fprintf(stderr,"error: var %s does not exist\n",aux->cmdarg[i]);
            	break;
      		}else{
      			var->cmdarg[i]=getenv(aux->cmdarg[0]);
       			free(aux);
      		}
   		}
		sprintf(line_aux,"%s %s", line_aux,var->cmdarg[i]); 
		i++;
	}
	printf("%s\n",line_aux);
	return line_aux;
	free(line);
	free(var);
	free(line_aux);
}
//me devolverá la ruta dentro de PATH en la que está el ejecutable,para luego llamar a exec
char*
execruta(char *path, char * cmd,int j){
	int i=0;
	char *aux=NULL;
	if (j>0){
		aux = built_path(cmd,path);
			if (access(aux,X_OK)!=0){
				return NULL;
			}
	}
	if(j<0){
		Command* rutaux =(Command*)malloc(256*sizeof(Command));
		mytoken(path,rutaux,":");
		while(rutaux->cmdarg[i] != NULL){
			aux = built_path(cmd,rutaux->cmdarg[i]);
			
			if(access(aux,X_OK)==0){
				break;	
			}else{
				i++;
				aux=NULL;	
			}
		}
	} 	
	return aux;
}
Command*
to_give(Command * mycmd){
	char *varpath = getenv("PATH");
	char cwd[1024];
	char *rutadefinitiva =(char*) malloc(1024*sizeof(char));
	char *path2=(char*) malloc(1024*sizeof(char));	
	if ((getcwd(cwd, sizeof(cwd)) != NULL)&&(rutadefinitiva=execruta(cwd,mycmd->command,1))!=NULL){
  			mycmd->command=rutadefinitiva;
	}else{
	/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
   		strcpy(path2, varpath);
		if((rutadefinitiva=execruta(path2,mycmd->command,-1))!=NULL){
			mycmd->command=rutadefinitiva;
		}
	}
	return mycmd;
	free(varpath);
	free(path2);
}
void 
to_fork_pipes(Command cmdlist [],int nc,int fo) {
	// Parent process creates all needed pipes at the beginning
	// 1 Pipe for 2 command (number of command - 1)
	int fd[2*(nc-1)];
	int i, pid;
	//int fi = 0;
	for( i = 0; i < (nc-1); i++ ){
    	if( pipe(fd + i*2) < 0 ) err (1,"Pipe creation failed\n");
	}
	// Loop for all commands to execute -
	int j=0;
	for(j=0; j < nc; j++){
		// create the child
		pid = fork();
 			
		switch(pid){
		
		case -1:
			err(1,"fork failed\n");
		
		case 0: 
			if(j == 0){				// First command
				int back = open("/dev/null",  O_RDONLY);
               	if(back < 0)	err(1,"Cannot open /dev/null\n");
            	
				dup2(back,0); // connect fi to stdin
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
			to_give(&cmdlist[j]);
           	execv(cmdlist[j].command,cmdlist[j].cmdarg);
			err(1,"Execute command failed"); // if execv returns, we get an error
		
		default:
		;
			//wait(NULL);
	}
	}
}
void
to_proccess(Command cmdlist [],int nc,int fo){

	if (strcmp(cmdlist[0].command,"cd") == 0){ //SOLO PUEDE ESTAR EN EL 0. HAGO EL CAMBIO, Y PROCESO A PARTIR E AHÍ
		char cwd[1024];
		mycd(&cmdlist[0]);
		if (getcwd(cwd, sizeof(cwd)) != NULL)	printf("directorio actual%s\n",cwd);
			cmdlist++;
			nc--;
			//printf("%d\n",nc );
			//printf("%s\n",cmdlist[0].command );
		to_fork_pipes(cmdlist,nc,fo);
	}
	if (strcmp(cmdlist[0].command,"cd") != 0){  //NO ES CD EL PRIMERO
		to_fork_pipes(cmdlist,nc,fo);
	}
}
void
readfile(FILE* fi,int fo){
	char myline [MAX];
	char *line=NULL;
	Command cmdlist [MAX] ; 
	int nc=0;
	while(feof(fi)!=1){
		line = fgets(myline,sizeof(myline),fi);
		if(line != NULL){
			printf("line  %d %s",nc ,line);	
			mytoken(line,&cmdlist[nc]," ");
			nc++;
		}
	}
	to_proccess(cmdlist,nc,fo);
	printf("final fichero\n" );
	fclose(fi);
	close(1);
}
 //grvoluntarios@gmail.com Con el nombre y el turno
int 
main(int argc, char *argv[]){
	char *pwd = getenv("PWD");
	DIR *d = opendir(pwd);
	struct dirent *de;
	char *buf = malloc(sizeof (PATH_MAX));
	if(d == NULL) 		err(1, "dir");
	if(chdir(pwd)<0)    err(1, "error con chdir");   
    
    /*OPCION*/
	if(argc==2 && (strcmp(argv[1],"-c") == 0)){
	    while((de = readdir(d)) != NULL) {
		buf= built_path(de->d_name,pwd);
			if ((is_file(buf) && (get_format(de->d_name,1)==1)) )
		   		unlink(buf);
			if ((is_file(buf) && (get_format(de->d_name,2)==1)) )
				unlink(buf);   
			}
	
	}else{

	int newfd;
	FILE * myFile;
	int sts,pid,np;
	np=0;//numero de procesos creados//
    while((de = readdir(d)) != NULL) {
		buf= built_path(de->d_name,pwd);
		if ((is_file(buf) && (get_format(de->d_name,0)==1)) ){
			if((myFile = fopen(buf, "r"))== NULL) 	err(1, "bad filename");
   				printf("Abierto %s\n",buf );
   				newfd=createFile(built_newname(de->d_name));
				pid=fork();
				switch(pid){
					case -1:
						err(1, "fork failed");
						break;
					case 0: // esto es el hijo
						printf("%d\n",newfd );
						readfile(myFile,newfd);
						break;
					default: 
					np++;
						while(wait(&sts) != pid){
							if (sts == 1)	printf("%d\n",sts );
							close(newfd);
						}
			
 				}	
			}
		}
		printf("nptotal %d\n",np );
	}

	free(buf);
	closedir(d);
return 0;
}
