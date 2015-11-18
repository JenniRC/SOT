/* 
	gcc -c -Wall -Wshadow  cunit.c && gcc -o cunit cunit.o && ./cunit
	cat < idefix | sort | uniq > fo
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
/*Retorna 0 si no es .tst, retorna 1 sí es .tst*/
int
get_format(char *name){
	int len = 0;
	char *txt;

	txt = strstr(name, FORMAT);

	if(txt)
		len = strlen(txt);

	return ((txt) && (len == 4));
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
FILE*
counterfiles(){
	char *pwd = getenv("PWD");
	DIR *d = opendir(pwd);
	struct dirent *de;
	char *buf = malloc(sizeof (PATH_MAX));
	//int newfd;
	FILE * myFile;

	if(d == NULL) {
		err(1, "dir");
	}
	if(chdir(pwd)<0){
       err(1, "error con chdir");   
    }
    while((de = readdir(d)) != NULL) {
		buf= built_path(de->d_name,pwd);
		if ((is_file(buf) && (get_format(de->d_name)==1)) ){
			if((myFile = fopen(buf, "r"))== NULL) 	err(1, "bad filename");
   				printf("Abierto %s\n",buf );
   				//newfd = createFile(built_newname(de->d_name));
		}
	}
	free(buf);
	closedir(d);
	return myFile;
}

Command*
mytoken(char *cadena,Command *cmd,char *sep){
    char *token;
    char *saveptr1;
    int j=0;
    token = strtok_r(cadena,sep,&saveptr1);
    cmd->command = token;   		
    while(token){
    	if (strcmp(token," ") !=0)
       		cmd->cmdarg[j]=token;
       		quit_end(cmd->cmdarg[j]);
    	j++;
       	token = strtok_r(saveptr1,sep,&saveptr1);/*si es " " al final, lo cuenta como 1,asi que cuidado ahí*/
   	}
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
		rutaux = mytoken(path,rutaux,":");
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
	if ((getcwd(cwd, sizeof(cwd)) != NULL)&&(rutadefinitiva=execruta(cwd,mycmd->command,1))!=NULL){
  			mycmd->command=rutadefinitiva;
	}else{
	/*Si no son comandos ejecutables en el directorio actual,son en la variable path*/
		char *path2=(char*) malloc(1024*sizeof(char));	
   		strcpy(path2, varpath);
		if((rutadefinitiva=execruta(path2,mycmd->command,-1))!=NULL){
			mycmd->command=rutadefinitiva;
			}
	}
	return mycmd;
}
void 
to_fork_pipes(Command *cmd,int fo,int nc,int fi) {
	int   p[2];
	pid_t pid;
	int   fd_in = fi;
	int i;
	for(i = 0; i < nc ; i++){
    	pipe(p);
    	if ((pid = fork()) == -1){
         	exit(EXIT_FAILURE);
        
        }else if (pid == 0){
        	dup2(fd_in, 0); //change the input according to the old one 
          	if (i != nc-1){
          		dup2(p[1], 1);
          	}else if(i == (nc-1)){
				dup2(fo,1);
			}           
          		//if(towait==1 && fi ==0 ){ // No hay que esperar
	    		int back = open("/dev/null",  O_RDONLY);
               	if(back < 0)
                   	fprintf(stderr,"Cannot open /dev/null\n");
               	dup2(back, 0);
            	close(back);
        	//}
        	close(p[0]);
          	Command* caux=(Command*)malloc(256*sizeof(Command));
			caux = mytoken(cmd->cmdarg[i],caux," ");
			caux = to_give(caux);
           	execv(caux->command,caux->cmdarg);
			printf("Execute command failed\n");
          	exit(EXIT_FAILURE);
        }else{
          wait(NULL);
          close(p[1]);
          fd_in = p[0]; //save the input for the next command
        }
    }
}
void
to_proccess(Command cmdlist []){
	printf("entro\n" );
	printf("comparo %s , %s\n",cmdlist[0].command,cmdlist[0].cmdarg[1] );
	//printf("comando en memoria %p comando :%s\n",cmdlist[0]->command,cmdlist[0]->command);
		/*if (strcmp(cmdlist[0].command,"cd") == 0){ //SOLO PUEDE ESTAR EN EL 0. HAGO EL CAMBIO, Y PROCESO A PARTIR E AHÍ
			printf("comparo999999999%s\n",cmdlist[0].command );
			Command* mycmd=(Command*)malloc(512*sizeof(Command));
			char cwd[1024];
			//mycmd->command=mylist[0].command;
			//mycmd->cmdarg=mylist[0].cmdarg;
			mycd(mycmd);
			if (getcwd(cwd, sizeof(cwd)) != NULL)	printf("directorio actural%s",cwd);
		
			//printf("%s\n",mylist[0]->command );
		}
		printf("salgo\n" );
*/
}
 //grvoluntarios@gmail.com Con el nombre y el turno
/*void
init (Command  mylist[],char * line,int i){
	Command * mycmd=(Command*)malloc(sizeof(Command));

	mycmd=mytoken(line,mycmd," ");
	memcpy(&mylist[i], &mycmd, sizeof (Command));
}
*/
int 
main(int argc, char *argv[]){

	char myline [MAX];
	char *line=NULL;
	FILE* fi = counterfiles();
	Command cmdlist [7] ; 
	Command * mycmd=(Command*)malloc(sizeof(Command));
	int i=0;
	memset(cmdlist,0,sizeof(Command[7])) ;
while(feof(fi)!=1){
	line = fgets(myline,sizeof(myline),fi);
		if(line != NULL){
			printf("line  %d %s",i ,line);	
			//init(cmdlist,line,i);
			mycmd=mytoken(line,mycmd," ");
			cmdlist[i]=*mycmd;
			//memcpy(&cmdlist[i], &mycmd, sizeof (Command));
			//memcpy(&cmdlist[i].command, &mycmd->command, sizeof (Command->command));
			//memcpy(cmdlist[i].cmdarg, mycmd->cmdarg, sizeof (mycmd->cmdarg));
			printf("	comando en list %d %s  %s\n",i,cmdlist[i].command,cmdlist[i].cmdarg[1] );
		//	printf("	comando %p en memoria \n",&mycmd->command);
		//	printf("	comando en list/memoria %d %p\n",i,&cmdlist[i].command );
			i++;

		}
	}
	int c;
	for(c=0;c<7;c++){
		if(cmdlist[c].command!=NULL) printf("c:%d%s\n",c,cmdlist[c].command );
	}
	
	//free(mycmd);
	to_proccess(cmdlist);
	printf("final fichero\n" );
	fclose(fi);

return 0;
}
