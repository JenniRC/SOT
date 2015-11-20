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
typedef struct Test Test ; 
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
/*Construye el nombre del fichero.out, para guardar la salida de la ejecucción( 0->Out 1->ok)*/
char *
built_newname(char *s1,int f){
	char * aux;
	char *token;
    char *saveptr1;
    token = strtok_r(s1,".",&saveptr1);

    switch(f){
	case 0:
	    aux = malloc (strlen(token)+strlen(OUTPUT)+1);
		sprintf(aux,"%s%s",token,OUTPUT);
		break;
	case 1:
	 	aux = malloc (strlen(token)+strlen(OKTEST)+1);
		sprintf(aux,"%s%s",token,OKTEST);
		break;
	}	
	return aux;
	free(aux);
}
/*Return 1 if .tst !.out !.ok*/
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
	aux = malloc (n1+n2+2);
	sprintf(aux, "%s/%s", path, s1);
	return aux;
	//free(aux);
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
/* Parent process creates all needed pipes at the beginning .1 Pipe for 2 command (number of command - 1)*/
void 
to_fork_pipes(Command cmdlist [],int nc,int fo) {

	int fd[2*(nc-1)];
	int i, pid;
	dup2(fo,2); // connect fo to stderr
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
				dup2(back,0); // connect to stdin
			}
			if(j == (nc-1)){// Last command
				dup2(fo,1); // connect fo to stdout
			}
			if(j!=0){// NOT First Command
				dup2(fd[(j-1)*2],0); // get imput from previous command
			}
			if(j != (nc-1)){// NOT last command to process
				dup2(fd[j*2+1],1); // connect output to next command
			}
			// close pipes;
			int q;
            for(q = 0; q < 2*(nc-1); q++){
                close(fd[q]);
            }
			to_give(&cmdlist[j]);
           	execv(cmdlist[j].command,cmdlist[j].cmdarg);
			err(1,"Execute command failed"); // if execv returns, we get an error
		default:
			;
			//wait(NULL); (I dont want to wait any child)
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
		//	printf("line  %d %s",nc ,line);	
			mytoken(line,&cmdlist[nc]," ");
			nc++;
		}
	}
	to_proccess(cmdlist,nc,fo);
	fclose(fi);
	close(1);
}
void
read_write(int fd,int fd2){
	int nr;
	char  buffer[1024];
  for(;;){
    nr = read(fd, buffer, sizeof buffer );
      if(nr == 0)           break;
      if(nr < 0)            err(1, "bad reading");
      if(nr > 0){
      		write(fd2,buffer,nr);
      }
	}
}
int 
comparate(char *filename,char *fileout){
	FILE *fp1, *fp2;
	int ch1, ch2;
 	fp1 = fopen(filename, "r");
	fp2 = fopen(fileout, "r");
	int eq;
 	if (fp1 == NULL)	printf("Cannot open %s for reading ", filename);
	if (fp2 == NULL)	printf("Cannot open %s for reading ", fileout);
    
    do{
        ch1 = getc(fp1);
        ch2 = getc(fp2);
    }while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2));
	
	if (ch1 == ch2)		eq = 1;
	if (ch1 != ch2)		eq = -1;
   
    fclose(fp1);
    fclose(fp2);
    return eq;
}
int
check_fook(char * filename,int fook,int newfd,char * fileout){
	int cp;
	if(fook==0){
		fook=createFile(filename); //No existía, lo creo y copio lo que hay en .out al .ok
		printf("-----------------%s\n",filename );
		read_write(newfd,fook);
		cp = 0;
	}
	if(fook>0){ //Si existe, asíque hay que comparar
		cp=comparate(filename,fileout);
	}
	return cp;
}
int 
main(int argc, char *argv[]){
	char *pwd = getenv("PWD");
	DIR *d = opendir(pwd);
	struct dirent *de;
	char *buf = malloc(sizeof (PATH_MAX));

	if(d == NULL) 		err(1, "dir");
	if(chdir(pwd)<0)    err(1, "error con chdir");   
    
	if(argc==2 && (strcmp(argv[1],"-c") == 0)){ /*OPCION*/
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
	int pid,np,fook;
	char * fileok;
	char *fileout;
	np=0;//numero de procesos creados//
    while((de = readdir(d)) != NULL) {
		buf = built_path(de->d_name,pwd);
		if ((is_file(buf) && (get_format(de->d_name,0)==1)) ){
			if((myFile = fopen(buf, "r"))== NULL) 	err(1, "bad filename");
   			fileout = built_path(built_newname(de->d_name,0),pwd);
   			newfd = createFile(built_newname(de->d_name,0));
   			fileok = built_path(built_newname(de->d_name,1),pwd);
   			if((fook = open(fileok,O_RDONLY)) < 0){
   				fook=0;
   			} 	
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
						wait(NULL);
						int sucess=check_fook(fileok,fook,newfd,fileout); 
						if(sucess<0){
							printf("NOT EQUAL\n");
						}				
 				}	
 				printf("nptotal %d\n",np );
			}
			close(newfd);
			close(fook);
		}
	free(buf);
	closedir(d);
}
return 0;
}
